import time
import uuid
from pathlib import Path

import aiosqlite
from config import settings
from models import Printer, PrinterCreate, PrinterUpdate, Spool, SpoolCreate, SpoolUpdate

SCHEMA = """
-- Spools table
CREATE TABLE IF NOT EXISTS spools (
    id TEXT PRIMARY KEY,
    spool_number INTEGER UNIQUE,
    tag_id TEXT UNIQUE,
    material TEXT NOT NULL,
    subtype TEXT,
    color_name TEXT,
    rgba TEXT,
    brand TEXT,
    label_weight INTEGER DEFAULT 1000,
    core_weight INTEGER DEFAULT 250,
    weight_new INTEGER,
    weight_current INTEGER,
    weight_used REAL DEFAULT 0,
    slicer_filament TEXT,
    slicer_filament_name TEXT,
    location TEXT,
    note TEXT,
    added_time INTEGER,
    encode_time INTEGER,
    added_full INTEGER DEFAULT 0,
    consumed_since_add REAL DEFAULT 0,
    consumed_since_weight REAL DEFAULT 0,
    data_origin TEXT,
    tag_type TEXT,
    ext_has_k INTEGER DEFAULT 0,
    archived_at INTEGER,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Printers table
CREATE TABLE IF NOT EXISTS printers (
    serial TEXT PRIMARY KEY,
    name TEXT,
    model TEXT,
    ip_address TEXT,
    access_code TEXT,
    last_seen INTEGER,
    config TEXT,
    auto_connect INTEGER DEFAULT 0,
    nozzle_count INTEGER DEFAULT 1
);

-- K-Profiles table
CREATE TABLE IF NOT EXISTS k_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spool_id TEXT REFERENCES spools(id) ON DELETE CASCADE,
    printer_serial TEXT REFERENCES printers(serial) ON DELETE CASCADE,
    extruder INTEGER,
    nozzle_diameter TEXT,
    nozzle_type TEXT,
    k_value TEXT,
    name TEXT,
    cali_idx INTEGER,
    setting_id TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Usage history table
CREATE TABLE IF NOT EXISTS usage_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spool_id TEXT REFERENCES spools(id) ON DELETE CASCADE,
    printer_serial TEXT,
    print_name TEXT,
    weight_used REAL,
    timestamp INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Spool-to-AMS slot assignments (persistent mapping)
CREATE TABLE IF NOT EXISTS spool_assignments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spool_id TEXT REFERENCES spools(id) ON DELETE CASCADE,
    printer_serial TEXT NOT NULL,
    ams_id INTEGER NOT NULL,
    tray_id INTEGER NOT NULL,
    assigned_at INTEGER DEFAULT (strftime('%s', 'now')),
    UNIQUE(printer_serial, ams_id, tray_id)
);

-- Settings table (key-value store for app settings)
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Spool catalog (empty spool weights by brand/type)
CREATE TABLE IF NOT EXISTS spool_catalog (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    weight INTEGER NOT NULL,
    is_default INTEGER DEFAULT 1,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);
CREATE UNIQUE INDEX IF NOT EXISTS idx_spool_catalog_name ON spool_catalog(name);

-- Color catalog (filament colors by manufacturer/material)
CREATE TABLE IF NOT EXISTS color_catalog (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    manufacturer TEXT NOT NULL,
    color_name TEXT NOT NULL,
    hex_color TEXT NOT NULL,
    material TEXT,
    is_default INTEGER DEFAULT 1,
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);
CREATE UNIQUE INDEX IF NOT EXISTS idx_color_catalog_unique ON color_catalog(manufacturer, color_name, material);

-- AMS sensor history (for humidity/temperature graphs)
CREATE TABLE IF NOT EXISTS ams_sensor_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    printer_serial TEXT NOT NULL REFERENCES printers(serial) ON DELETE CASCADE,
    ams_id INTEGER NOT NULL,
    humidity REAL,
    humidity_raw REAL,
    temperature REAL,
    recorded_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Indexes
CREATE INDEX IF NOT EXISTS idx_spools_tag_id ON spools(tag_id);
CREATE INDEX IF NOT EXISTS idx_spools_material ON spools(material);
CREATE INDEX IF NOT EXISTS idx_k_profiles_spool ON k_profiles(spool_id);
CREATE INDEX IF NOT EXISTS idx_usage_history_spool ON usage_history(spool_id);
CREATE INDEX IF NOT EXISTS idx_spool_assignments_slot ON spool_assignments(printer_serial, ams_id, tray_id);
CREATE INDEX IF NOT EXISTS idx_ams_sensor_history_lookup ON ams_sensor_history(printer_serial, ams_id, recorded_at);
"""

# Default spool catalog data (name, weight in grams)
DEFAULT_SPOOL_CATALOG = [
    ("3D FilaPrint - Cardboard", 210),
    ("3D FilaPrint - Plastic", 238),
    ("3D Fuel - Plastic", 264),
    ("3D Power - Plastic", 220),
    ("3D Solutech - Plastic", 173),
    ("3DE - Cardboard", 136),
    ("3DE - Plastic", 181),
    ("3DHOJOR - Cardboard", 157),
    ("3DJake - Cardboard", 209),
    ("3DJake - Plastic", 232),
    ("3DJake 250g - Plastic", 91),
    ("3DJake ecoPLA - Plastic", 210),
    ("3DXTech - Plastic", 258),
    ("Acccreate - Plastic", 161),
    ("Amazon Basics - Plastic", 234),
    ("Amolen - Plastic", 150),
    ("AMZ3D - Plastic", 233),
    ("Anycubic - Cardboard", 125),
    ("Anycubic - Plastic", 127),
    ("Atomic Filament - Plastic", 272),
    ("Aurapol - Plastic", 220),
    ("Azure Film - Plastic", 163),
    ("Bambu Lab - Plastic High Temp", 216),
    ("Bambu Lab - Plastic Low Temp", 250),
    ("Bambu Lab - Plastic White", 253),
    ("BQ - Plastic", 218),
    ("Colorfabb - Plastic", 236),
    ("Colorfabb 750g - Cardboard", 152),
    ("Colorfabb 750g - Plastic", 254),
    ("Comgrow - Cardboard", 166),
    ("Creality - Cardboard", 180),
    ("Creality - Plastic", 135),
    ("Das Filament - Plastic", 211),
    ("Devil Design - Plastic", 256),
    ("Duramic 3D - Cardboard", 136),
    ("Elegoo - Cardboard", 153),
    ("Elegoo - Plastic", 111),
    ("Eryone - Cardboard", 156),
    ("Eryone - Plastic", 187),
    ("eSUN - Cardboard", 147),
    ("eSUN - Plastic", 240),
    ("eSUN 2.5kg - Plastic", 634),
    ("Extrudr - Plastic", 244),
    ("Fiberlogy - Plastic", 260),
    ("Filament PM - Plastic", 224),
    ("Fillamentum - Plastic", 230),
    ("Flashforge - Plastic", 167),
    ("FormFutura - Cardboard", 155),
    ("FormFutura 750g - Plastic", 212),
    ("Geeetech - Plastic", 178),
    ("Gembird - Cardboard", 143),
    ("Hatchbox - Plastic", 225),
    ("Inland - Cardboard", 142),
    ("Inland - Plastic", 210),
    ("Jayo - Cardboard", 120),
    ("Jayo - Plastic", 126),
    ("Jayo 250g - Plastic", 58),
    ("Kingroon - Cardboard", 155),
    ("Kingroon - Plastic", 156),
    ("KVP - Plastic", 263),
    ("Matter Hackers - Plastic", 215),
    ("MG Chemicals - Cardboard", 150),
    ("MG Chemicals - Plastic", 239),
    ("Mika3D - Plastic", 175),
    ("MonoPrice - Plastic", 221),
    ("Overture - Cardboard", 150),
    ("Overture - Plastic", 237),
    ("PolyMaker - Cardboard", 137),
    ("PolyMaker - Plastic", 220),
    ("PolyMaker 3kg - Cardboard", 418),
    ("PolyTerra PLA - Cardboard", 147),
    ("PrimaSelect - Plastic", 222),
    ("ProtoPasta - Cardboard", 80),
    ("Prusament - Plastic", 201),
    ("Prusament - Plastic w/ Cardboard Core", 196),
    ("Rosa3D - Plastic", 245),
    ("Sakata3D - Plastic", 205),
    ("Snapmaker - Cardboard", 148),
    ("Sovol - Cardboard", 145),
    ("Spectrum - Cardboard", 180),
    ("Spectrum - Plastic", 257),
    ("Sunlu - Plastic", 117),
    ("Sunlu - Plastic V2", 165),
    ("Sunlu - Plastic V3", 179),
    ("Sunlu 250g - Plastic", 55),
    ("UltiMaker - Plastic", 235),
    ("Voolt3D - Plastic", 190),
    ("Voxelab - Plastic", 171),
    ("Wanhao - Plastic", 267),
    ("Ziro - Plastic", 166),
    ("ZYLtech - Plastic", 179),
]

# Default color catalog data (manufacturer, color_name, hex_color, material)
# Sources: Bambu Lab official hex code PDFs, filamentcolors.xyz
DEFAULT_COLOR_CATALOG = [
    # Bambu Lab PLA Basic
    ("Bambu Lab", "Jade White", "#FFFFFF", "PLA Basic"),
    ("Bambu Lab", "Black", "#000000", "PLA Basic"),
    ("Bambu Lab", "Silver", "#A6A9AA", "PLA Basic"),
    ("Bambu Lab", "Light Gray", "#C0C0C0", "PLA Basic"),
    ("Bambu Lab", "Gray", "#8E9089", "PLA Basic"),
    ("Bambu Lab", "Dark Gray", "#616364", "PLA Basic"),
    ("Bambu Lab", "Red", "#C12E1F", "PLA Basic"),
    ("Bambu Lab", "Magenta", "#EC008C", "PLA Basic"),
    ("Bambu Lab", "Hot Pink", "#FF69B4", "PLA Basic"),
    ("Bambu Lab", "Pink", "#F55A74", "PLA Basic"),
    ("Bambu Lab", "Beige", "#F7E6DE", "PLA Basic"),
    ("Bambu Lab", "Yellow", "#FFFF00", "PLA Basic"),
    ("Bambu Lab", "Sunflower Yellow", "#FEC600", "PLA Basic"),
    ("Bambu Lab", "Gold", "#E4BD68", "PLA Basic"),
    ("Bambu Lab", "Orange", "#FF8C00", "PLA Basic"),
    ("Bambu Lab", "Pumpkin Orange", "#FF9016", "PLA Basic"),
    ("Bambu Lab", "Bright Green", "#66FF00", "PLA Basic"),
    ("Bambu Lab", "Bambu Green", "#00AE42", "PLA Basic"),
    ("Bambu Lab", "Mistletoe Green", "#3F8E43", "PLA Basic"),
    ("Bambu Lab", "Turquoise", "#00B1B7", "PLA Basic"),
    ("Bambu Lab", "Cyan", "#0086D6", "PLA Basic"),
    ("Bambu Lab", "Blue", "#0A2989", "PLA Basic"),
    ("Bambu Lab", "Blue Grey", "#647988", "PLA Basic"),
    ("Bambu Lab", "Cobalt Blue", "#0047AB", "PLA Basic"),
    ("Bambu Lab", "Purple", "#5E43B7", "PLA Basic"),
    ("Bambu Lab", "Indigo Purple", "#482960", "PLA Basic"),
    ("Bambu Lab", "Brown", "#9D432C", "PLA Basic"),
    ("Bambu Lab", "Cocoa Brown", "#5C4033", "PLA Basic"),
    ("Bambu Lab", "Bronze", "#847D48", "PLA Basic"),
    # Bambu Lab PLA Matte
    ("Bambu Lab", "Ivory White", "#EBEBE3", "PLA Matte"),
    ("Bambu Lab", "Bone White", "#F5F5DC", "PLA Matte"),
    ("Bambu Lab", "Lemon Yellow", "#FFF44F", "PLA Matte"),
    ("Bambu Lab", "Mandarin Orange", "#FF7518", "PLA Matte"),
    ("Bambu Lab", "Scarlet Red", "#FF2400", "PLA Matte"),
    ("Bambu Lab", "Lilac Purple", "#C8A2C8", "PLA Matte"),
    ("Bambu Lab", "Grape Purple", "#6F2DA8", "PLA Matte"),
    ("Bambu Lab", "Grass Green", "#6BB173", "PLA Matte"),
    ("Bambu Lab", "Dark Green", "#656A4D", "PLA Matte"),
    ("Bambu Lab", "Sakura Pink", "#EAB8CA", "PLA Matte"),
    ("Bambu Lab", "Charcoal", "#36454F", "PLA Matte"),
    # Bambu Lab PLA Silk
    ("Bambu Lab", "Blue", "#4F9CCC", "PLA Silk"),
    ("Bambu Lab", "Gold", "#CFB53B", "PLA Silk"),
    ("Bambu Lab", "Silver", "#C0C0C0", "PLA Silk"),
    ("Bambu Lab", "Copper", "#B87333", "PLA Silk"),
    ("Bambu Lab", "Green", "#50C878", "PLA Silk"),
    ("Bambu Lab", "Red", "#DC143C", "PLA Silk"),
    # Bambu Lab PLA Sparkle
    ("Bambu Lab", "Alpine Green Sparkle", "#4F6359", "PLA Sparkle"),
    ("Bambu Lab", "Galaxy Black Sparkle", "#1C1C1C", "PLA Sparkle"),
    ("Bambu Lab", "Space Gray Sparkle", "#4A4A4A", "PLA Sparkle"),
    # Bambu Lab PETG Basic
    ("Bambu Lab", "Black", "#000000", "PETG Basic"),
    ("Bambu Lab", "White", "#FFFFFF", "PETG Basic"),
    ("Bambu Lab", "Gray", "#808080", "PETG Basic"),
    ("Bambu Lab", "Translucent", "#F0F0F0", "PETG Basic"),
    # Bambu Lab PETG-HF
    ("Bambu Lab", "White", "#F0F1F0", "PETG-HF"),
    ("Bambu Lab", "Black", "#000000", "PETG-HF"),
    ("Bambu Lab", "Gray", "#A3A6A6", "PETG-HF"),
    ("Bambu Lab", "Red", "#C33F45", "PETG-HF"),
    ("Bambu Lab", "Orange", "#FF7146", "PETG-HF"),
    ("Bambu Lab", "Blue", "#1E90FF", "PETG-HF"),
    ("Bambu Lab", "Translucent Orange", "#EF8E5B", "PETG-HF"),
    # Bambu Lab ABS
    ("Bambu Lab", "Black", "#000000", "ABS"),
    ("Bambu Lab", "White", "#FFFFFF", "ABS"),
    ("Bambu Lab", "Gray", "#808080", "ABS"),
    ("Bambu Lab", "Red", "#FF0000", "ABS"),
    # Bambu Lab ASA
    ("Bambu Lab", "Black", "#000000", "ASA"),
    ("Bambu Lab", "White", "#FFFFFF", "ASA"),
    ("Bambu Lab", "Gray", "#808080", "ASA"),
    # Bambu Lab TPU
    ("Bambu Lab", "White", "#F0EFE3", "TPU 95A"),
    ("Bambu Lab", "Black", "#000000", "TPU 95A"),
    ("Bambu Lab", "Gray", "#8C9091", "TPU 95A"),
    ("Bambu Lab", "Red", "#FF0000", "TPU 95A"),
    # Bambu Lab PLA-CF / PAHT-CF / PETG-CF
    ("Bambu Lab", "Black", "#1A1A1A", "PLA-CF"),
    ("Bambu Lab", "Black", "#1A1A1A", "PAHT-CF"),
    ("Bambu Lab", "Black", "#1A1A1A", "PETG-CF"),
    # Bambu Lab Support Materials
    ("Bambu Lab", "Natural", "#F5F5DC", "PLA Support"),
    ("Bambu Lab", "Natural", "#F5F5DC", "PVA Support"),
    # Polymaker PolyTerra PLA (popular brand)
    ("Polymaker", "Cotton White", "#F5F5F5", "PolyTerra PLA"),
    ("Polymaker", "Charcoal Black", "#2B2B2B", "PolyTerra PLA"),
    ("Polymaker", "Marble White", "#E8E8E8", "PolyTerra PLA"),
    ("Polymaker", "Fossil Grey", "#6B6B6B", "PolyTerra PLA"),
    ("Polymaker", "Shadow Black", "#1A1A1A", "PolyTerra PLA"),
    ("Polymaker", "Army Red", "#8B0000", "PolyTerra PLA"),
    ("Polymaker", "Lava Red", "#CF1020", "PolyTerra PLA"),
    ("Polymaker", "Sakura Pink", "#FFB7C5", "PolyTerra PLA"),
    ("Polymaker", "Rose", "#FF007F", "PolyTerra PLA"),
    ("Polymaker", "Peach", "#FFCBA4", "PolyTerra PLA"),
    ("Polymaker", "Banana", "#FFE135", "PolyTerra PLA"),
    ("Polymaker", "Savannah Yellow", "#F4C430", "PolyTerra PLA"),
    ("Polymaker", "Sunrise Orange", "#FF6600", "PolyTerra PLA"),
    ("Polymaker", "Muted Green", "#4F7942", "PolyTerra PLA"),
    ("Polymaker", "Forest Green", "#228B22", "PolyTerra PLA"),
    ("Polymaker", "Mint", "#98FF98", "PolyTerra PLA"),
    ("Polymaker", "Lavender Purple", "#B57EDC", "PolyTerra PLA"),
    ("Polymaker", "Sapphire Blue", "#0F52BA", "PolyTerra PLA"),
    ("Polymaker", "Ice", "#D6ECEF", "PolyTerra PLA"),
    # Prusament PLA
    ("Prusament", "Jet Black", "#1A1A1A", "PLA"),
    ("Prusament", "Galaxy Black", "#1F1F1F", "PLA"),
    ("Prusament", "Pristine White", "#FFFFFF", "PLA"),
    ("Prusament", "Gentleman's Grey", "#5A5A5A", "PLA"),
    ("Prusament", "Lipstick Red", "#C21E1E", "PLA"),
    ("Prusament", "Orange", "#FF6600", "PLA"),
    ("Prusament", "Pineapple Yellow", "#FFD700", "PLA"),
    ("Prusament", "Jungle Green", "#29AB87", "PLA"),
    ("Prusament", "Azure Blue", "#007FFF", "PLA"),
    ("Prusament", "Royal Blue", "#4169E1", "PLA"),
    ("Prusament", "Mystic Purple", "#7B68EE", "PLA"),
    # eSUN PLA+
    ("eSUN", "White", "#FFFFFF", "PLA+"),
    ("eSUN", "Black", "#000000", "PLA+"),
    ("eSUN", "Grey", "#808080", "PLA+"),
    ("eSUN", "Red", "#FF0000", "PLA+"),
    ("eSUN", "Blue", "#0000FF", "PLA+"),
    ("eSUN", "Green", "#00FF00", "PLA+"),
    ("eSUN", "Yellow", "#FFFF00", "PLA+"),
    ("eSUN", "Orange", "#FFA500", "PLA+"),
    ("eSUN", "Purple", "#800080", "PLA+"),
    ("eSUN", "Pink", "#FFC0CB", "PLA+"),
    # Hatchbox PLA
    ("Hatchbox", "White", "#FFFFFF", "PLA"),
    ("Hatchbox", "Black", "#000000", "PLA"),
    ("Hatchbox", "Gray", "#808080", "PLA"),
    ("Hatchbox", "Red", "#FF0000", "PLA"),
    ("Hatchbox", "Blue", "#0000FF", "PLA"),
    ("Hatchbox", "Green", "#00FF00", "PLA"),
    ("Hatchbox", "Yellow", "#FFFF00", "PLA"),
    ("Hatchbox", "Orange", "#FFA500", "PLA"),
    ("Hatchbox", "Purple", "#800080", "PLA"),
    ("Hatchbox", "Pink", "#FFC0CB", "PLA"),
    ("Hatchbox", "True Blue", "#0073CF", "PLA"),
    ("Hatchbox", "True Green", "#008000", "PLA"),
]


class Database:
    """Async SQLite database wrapper."""

    def __init__(self, db_path: Path):
        self.db_path = db_path
        self._connection: aiosqlite.Connection | None = None

    async def connect(self):
        """Connect to database and run migrations."""
        self._connection = await aiosqlite.connect(self.db_path)
        self._connection.row_factory = aiosqlite.Row
        await self._connection.executescript(SCHEMA)
        await self._connection.commit()

        # Run migrations for new columns
        await self._run_migrations()

        # Seed spool catalog with defaults if empty
        await self.seed_spool_catalog()

        # Seed color catalog with defaults if empty
        await self.seed_color_catalog()

    async def _run_migrations(self):
        """Run database migrations for new columns."""
        # Check if spool_number column exists
        async with self.conn.execute("PRAGMA table_info(spools)") as cursor:
            columns = [row["name"] for row in await cursor.fetchall()]

        if "spool_number" not in columns:
            # SQLite can't add UNIQUE column directly, so add without constraint first
            await self.conn.execute("ALTER TABLE spools ADD COLUMN spool_number INTEGER")
            # Assign sequential numbers to existing spools ordered by created_at
            await self.conn.execute("""
                UPDATE spools SET spool_number = (
                    SELECT COUNT(*) FROM spools s2
                    WHERE s2.created_at <= spools.created_at AND s2.id <= spools.id
                )
            """)
            # Create unique index for the constraint
            await self.conn.execute("CREATE UNIQUE INDEX IF NOT EXISTS idx_spools_spool_number ON spools(spool_number)")
            await self.conn.commit()

        if "location" not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN location TEXT")
            await self.conn.commit()

        if "ext_has_k" not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN ext_has_k INTEGER DEFAULT 0")
            await self.conn.commit()

        if "slicer_filament_name" not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN slicer_filament_name TEXT")
            await self.conn.commit()

        if "weight_used" not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN weight_used REAL DEFAULT 0")
            await self.conn.commit()

        if "archived_at" not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN archived_at INTEGER")
            await self.conn.commit()

        # Check printers table for nozzle_count
        async with self.conn.execute("PRAGMA table_info(printers)") as cursor:
            printer_columns = [row["name"] for row in await cursor.fetchall()]

        if "nozzle_count" not in printer_columns:
            await self.conn.execute("ALTER TABLE printers ADD COLUMN nozzle_count INTEGER DEFAULT 1")
            await self.conn.commit()

    async def disconnect(self):
        """Close database connection."""
        if self._connection:
            await self._connection.close()

    @property
    def conn(self) -> aiosqlite.Connection:
        if not self._connection:
            raise RuntimeError("Database not connected")
        return self._connection

    # ============ Spool Operations ============

    async def get_spools(self) -> list[Spool]:
        """Get all spools with last used timestamps."""
        # Join with usage_history to get last used time
        query = """
            SELECT s.*, (
                SELECT MAX(uh.timestamp) FROM usage_history uh WHERE uh.spool_id = s.id
            ) as last_used_time
            FROM spools s
            ORDER BY s.created_at DESC
        """
        async with self.conn.execute(query) as cursor:
            rows = await cursor.fetchall()
            return [Spool(**dict(row)) for row in rows]

    async def get_spool(self, spool_id: str) -> Spool | None:
        """Get a single spool by ID."""
        async with self.conn.execute("SELECT * FROM spools WHERE id = ?", (spool_id,)) as cursor:
            row = await cursor.fetchone()
            return Spool(**dict(row)) if row else None

    async def create_spool(self, spool: SpoolCreate) -> Spool:
        """Create a new spool."""
        spool_id = str(uuid.uuid4())
        now = int(time.time())

        # Get next spool_number (max + 1)
        async with self.conn.execute("SELECT COALESCE(MAX(spool_number), 0) + 1 FROM spools") as cursor:
            row = await cursor.fetchone()
            spool_number = row[0]

        await self.conn.execute(
            """INSERT INTO spools (id, spool_number, tag_id, material, subtype, color_name, rgba, brand,
               label_weight, core_weight, weight_new, weight_current, slicer_filament, slicer_filament_name,
               location, note, data_origin, tag_type, ext_has_k, created_at, updated_at)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""",
            (
                spool_id,
                spool_number,
                spool.tag_id,
                spool.material,
                spool.subtype,
                spool.color_name,
                spool.rgba,
                spool.brand,
                spool.label_weight,
                spool.core_weight,
                spool.weight_new,
                spool.weight_current,
                spool.slicer_filament,
                spool.slicer_filament_name,
                spool.location,
                spool.note,
                spool.data_origin,
                spool.tag_type,
                1 if spool.ext_has_k else 0,
                now,
                now,
            ),
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def update_spool(self, spool_id: str, spool: SpoolUpdate) -> Spool | None:
        """Update an existing spool."""
        existing = await self.get_spool(spool_id)
        if not existing:
            return None

        # Build update query dynamically for non-None fields
        updates = []
        values = []
        for field, value in spool.model_dump(exclude_unset=True).items():
            updates.append(f"{field} = ?")
            # Convert boolean to int for SQLite
            if field == "ext_has_k":
                values.append(1 if value else 0)
            else:
                values.append(value)

        if updates:
            updates.append("updated_at = ?")
            values.append(int(time.time()))
            values.append(spool_id)

            query = f"UPDATE spools SET {', '.join(updates)} WHERE id = ?"
            await self.conn.execute(query, values)
            await self.conn.commit()

        return await self.get_spool(spool_id)

    async def delete_spool(self, spool_id: str) -> bool:
        """Delete a spool."""
        cursor = await self.conn.execute("DELETE FROM spools WHERE id = ?", (spool_id,))
        await self.conn.commit()
        return cursor.rowcount > 0

    async def archive_spool(self, spool_id: str) -> Spool | None:
        """Archive a spool by setting archived_at timestamp."""
        now = int(time.time())
        await self.conn.execute("UPDATE spools SET archived_at = ?, updated_at = ? WHERE id = ?", (now, now, spool_id))
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def restore_spool(self, spool_id: str) -> Spool | None:
        """Restore an archived spool by clearing archived_at."""
        now = int(time.time())
        await self.conn.execute("UPDATE spools SET archived_at = NULL, updated_at = ? WHERE id = ?", (now, spool_id))
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def get_spool_by_tag(self, tag_id: str, include_archived: bool = False) -> Spool | None:
        """Get a spool by tag ID (base64-encoded UID).

        Args:
            tag_id: The tag ID to look up
            include_archived: If False (default), skip archived spools so recycled tags work
        """
        if include_archived:
            query = "SELECT * FROM spools WHERE tag_id = ?"
        else:
            query = "SELECT * FROM spools WHERE tag_id = ? AND archived_at IS NULL"
        async with self.conn.execute(query, (tag_id,)) as cursor:
            row = await cursor.fetchone()
            return Spool(**dict(row)) if row else None

    async def get_untagged_spools(self) -> list[Spool]:
        """Get all spools without a tag_id assigned."""
        async with self.conn.execute(
            "SELECT * FROM spools WHERE tag_id IS NULL OR tag_id = '' ORDER BY created_at DESC"
        ) as cursor:
            rows = await cursor.fetchall()
            return [Spool(**dict(row)) for row in rows]

    async def clear_spool_tag(self, spool_id: str) -> None:
        """Remove tag_id from a spool (for tag recycling)."""
        now = int(time.time())
        await self.conn.execute(
            "UPDATE spools SET tag_id = NULL, tag_type = NULL, updated_at = ? WHERE id = ?", (now, spool_id)
        )
        await self.conn.commit()

    async def link_tag_to_spool(
        self, spool_id: str, tag_id: str, tag_type: str | None = None, data_origin: str | None = None
    ) -> Spool | None:
        """Link an NFC tag to an existing spool.

        Args:
            spool_id: Spool UUID
            tag_id: Base64-encoded NFC UID
            tag_type: Optional tag type (e.g., "bambu", "generic")
            data_origin: Optional data origin (e.g., "nfc_link")

        Returns:
            Updated spool on success, None if spool not found
        """
        existing = await self.get_spool(spool_id)
        if not existing:
            return None

        now = int(time.time())
        updates = ["tag_id = ?", "updated_at = ?"]
        values = [tag_id, now]

        if tag_type:
            updates.append("tag_type = ?")
            values.append(tag_type)

        if data_origin:
            updates.append("data_origin = ?")
            values.append(data_origin)

        values.append(spool_id)
        query = f"UPDATE spools SET {', '.join(updates)} WHERE id = ?"

        await self.conn.execute(query, values)
        await self.conn.commit()

        return await self.get_spool(spool_id)

    # ============ Printer Operations ============

    async def get_printers(self) -> list[Printer]:
        """Get all printers."""
        async with self.conn.execute("SELECT * FROM printers ORDER BY name") as cursor:
            rows = await cursor.fetchall()
            return [Printer(**{**dict(row), "auto_connect": bool(row["auto_connect"])}) for row in rows]

    async def get_printer(self, serial: str) -> Printer | None:
        """Get a single printer by serial."""
        async with self.conn.execute("SELECT * FROM printers WHERE serial = ?", (serial,)) as cursor:
            row = await cursor.fetchone()
            if row:
                return Printer(**{**dict(row), "auto_connect": bool(row["auto_connect"])})
            return None

    async def create_printer(self, printer: PrinterCreate) -> Printer:
        """Create or update a printer."""
        now = int(time.time())

        await self.conn.execute(
            """INSERT INTO printers (serial, name, model, ip_address, access_code, last_seen, auto_connect)
               VALUES (?, ?, ?, ?, ?, ?, ?)
               ON CONFLICT(serial) DO UPDATE SET
               name = excluded.name,
               model = excluded.model,
               ip_address = excluded.ip_address,
               access_code = excluded.access_code,
               last_seen = excluded.last_seen,
               auto_connect = excluded.auto_connect""",
            (
                printer.serial,
                printer.name,
                printer.model,
                printer.ip_address,
                printer.access_code,
                now,
                int(printer.auto_connect),
            ),
        )
        await self.conn.commit()
        return await self.get_printer(printer.serial)

    async def update_printer(self, serial: str, printer: PrinterUpdate) -> Printer | None:
        """Update an existing printer."""
        existing = await self.get_printer(serial)
        if not existing:
            return None

        updates = []
        values = []
        for field, value in printer.model_dump(exclude_unset=True).items():
            if field == "auto_connect":
                value = int(value) if value is not None else None
            updates.append(f"{field} = ?")
            values.append(value)

        if updates:
            values.append(serial)
            query = f"UPDATE printers SET {', '.join(updates)} WHERE serial = ?"
            await self.conn.execute(query, values)
            await self.conn.commit()

        return await self.get_printer(serial)

    async def delete_printer(self, serial: str) -> bool:
        """Delete a printer."""
        cursor = await self.conn.execute("DELETE FROM printers WHERE serial = ?", (serial,))
        await self.conn.commit()
        return cursor.rowcount > 0

    async def update_nozzle_count(self, serial: str, nozzle_count: int) -> bool:
        """Update printer nozzle_count (auto-detected from MQTT)."""
        cursor = await self.conn.execute(
            "UPDATE printers SET nozzle_count = ? WHERE serial = ?", (nozzle_count, serial)
        )
        await self.conn.commit()
        return cursor.rowcount > 0

    async def get_auto_connect_printers(self) -> list[Printer]:
        """Get printers with auto_connect enabled."""
        async with self.conn.execute("SELECT * FROM printers WHERE auto_connect = 1") as cursor:
            rows = await cursor.fetchall()
            return [Printer(**{**dict(row), "auto_connect": True}) for row in rows]

    # ============ Spool Assignment Operations ============

    async def assign_spool_to_slot(self, spool_id: str, printer_serial: str, ams_id: int, tray_id: int) -> bool:
        """Assign a spool to an AMS slot (upsert)."""
        now = int(time.time())
        await self.conn.execute(
            """INSERT INTO spool_assignments (spool_id, printer_serial, ams_id, tray_id, assigned_at)
               VALUES (?, ?, ?, ?, ?)
               ON CONFLICT(printer_serial, ams_id, tray_id) DO UPDATE SET
               spool_id = excluded.spool_id,
               assigned_at = excluded.assigned_at""",
            (spool_id, printer_serial, ams_id, tray_id, now),
        )
        await self.conn.commit()
        return True

    async def unassign_slot(self, printer_serial: str, ams_id: int, tray_id: int) -> bool:
        """Remove spool assignment from a slot."""
        cursor = await self.conn.execute(
            "DELETE FROM spool_assignments WHERE printer_serial = ? AND ams_id = ? AND tray_id = ?",
            (printer_serial, ams_id, tray_id),
        )
        await self.conn.commit()
        return cursor.rowcount > 0

    async def get_spool_for_slot(self, printer_serial: str, ams_id: int, tray_id: int) -> str | None:
        """Get spool ID assigned to a slot."""
        async with self.conn.execute(
            "SELECT spool_id FROM spool_assignments WHERE printer_serial = ? AND ams_id = ? AND tray_id = ?",
            (printer_serial, ams_id, tray_id),
        ) as cursor:
            row = await cursor.fetchone()
            return row["spool_id"] if row else None

    async def get_slot_assignments(self, printer_serial: str) -> list[dict]:
        """Get all spool assignments for a printer."""
        async with self.conn.execute(
            """SELECT sa.*, s.material, s.color_name, s.rgba, s.brand
               FROM spool_assignments sa
               LEFT JOIN spools s ON sa.spool_id = s.id
               WHERE sa.printer_serial = ?""",
            (printer_serial,),
        ) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    # ============ Usage History Operations ============

    async def log_usage(self, spool_id: str, printer_serial: str, print_name: str, weight_used: float) -> int:
        """Log filament usage for a print job."""
        cursor = await self.conn.execute(
            """INSERT INTO usage_history (spool_id, printer_serial, print_name, weight_used)
               VALUES (?, ?, ?, ?)""",
            (spool_id, printer_serial, print_name, weight_used),
        )
        await self.conn.commit()
        return cursor.lastrowid

    async def get_usage_history(self, spool_id: str | None = None, limit: int = 100) -> list[dict]:
        """Get usage history, optionally filtered by spool."""
        if spool_id:
            query = """SELECT uh.*, s.material, s.color_name, s.brand
                       FROM usage_history uh
                       LEFT JOIN spools s ON uh.spool_id = s.id
                       WHERE uh.spool_id = ?
                       ORDER BY uh.timestamp DESC LIMIT ?"""
            params = (spool_id, limit)
        else:
            query = """SELECT uh.*, s.material, s.color_name, s.brand
                       FROM usage_history uh
                       LEFT JOIN spools s ON uh.spool_id = s.id
                       ORDER BY uh.timestamp DESC LIMIT ?"""
            params = (limit,)

        async with self.conn.execute(query, params) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    async def update_spool_consumption(
        self, spool_id: str, weight_used: float, new_weight: int | None = None
    ) -> Spool | None:
        """Update spool consumption after a print.

        Args:
            spool_id: Spool ID
            weight_used: Grams of filament consumed
            new_weight: Optional new current weight (from scale)
        """
        spool = await self.get_spool(spool_id)
        if not spool:
            return None

        now = int(time.time())
        updates = ["updated_at = ?"]
        values = [now]

        # Increment consumption counters
        new_consumed_add = (spool.consumed_since_add or 0) + weight_used
        new_consumed_weight = (spool.consumed_since_weight or 0) + weight_used
        updates.extend(["consumed_since_add = ?", "consumed_since_weight = ?"])
        values.extend([new_consumed_add, new_consumed_weight])

        # Update current weight if provided (from scale) or calculate from consumption
        if new_weight is not None:
            updates.append("weight_current = ?")
            values.append(new_weight)
            # Reset consumed_since_weight when scale reading is taken
            updates[-2] = "consumed_since_weight = 0"
            values[-2] = 0
        elif spool.weight_current is not None:
            # Decrement current weight by usage
            calculated_weight = max(0, spool.weight_current - int(weight_used))
            updates.append("weight_current = ?")
            values.append(calculated_weight)

        values.append(spool_id)
        query = f"UPDATE spools SET {', '.join(updates)} WHERE id = ?"
        await self.conn.execute(query, values)
        await self.conn.commit()

        return await self.get_spool(spool_id)

    async def set_spool_weight(self, spool_id: str, weight: int) -> Spool | None:
        """Set spool current weight from scale and recalculate weight_used to match.

        This syncs the tracking to match the scale reading by:
        1. Setting weight_current to the scale reading (gross weight)
        2. Resetting consumed_since_weight to 0
        3. Calculating weight_used so that: gross = core_weight + (label_weight - weight_used)
           => weight_used = core_weight + label_weight - gross
        """
        spool = await self.get_spool(spool_id)
        if not spool:
            return None

        # Calculate what weight_used should be to match the scale reading
        # gross_weight = core_weight + net_weight
        # net_weight = label_weight - weight_used - consumed_since_weight
        # After sync: gross = core_weight + (label_weight - weight_used_new - 0)
        # So: weight_used_new = core_weight + label_weight - gross
        core_weight = spool.core_weight or 0
        label_weight = spool.label_weight or 0
        weight_used_new = max(0, core_weight + label_weight - weight)

        now = int(time.time())
        await self.conn.execute(
            """UPDATE spools SET weight_current = ?, weight_used = ?, consumed_since_weight = 0, updated_at = ?
               WHERE id = ?""",
            (weight, weight_used_new, now, spool_id),
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    # ============ Settings Operations ============

    async def get_setting(self, key: str) -> str | None:
        """Get a setting value by key."""
        async with self.conn.execute("SELECT value FROM settings WHERE key = ?", (key,)) as cursor:
            row = await cursor.fetchone()
            return row["value"] if row else None

    async def set_setting(self, key: str, value: str) -> None:
        """Set a setting value (upsert)."""
        now = int(time.time())
        await self.conn.execute(
            """INSERT INTO settings (key, value, updated_at)
               VALUES (?, ?, ?)
               ON CONFLICT(key) DO UPDATE SET
               value = excluded.value,
               updated_at = excluded.updated_at""",
            (key, value, now),
        )
        await self.conn.commit()

    async def delete_setting(self, key: str) -> bool:
        """Delete a setting."""
        cursor = await self.conn.execute("DELETE FROM settings WHERE key = ?", (key,))
        await self.conn.commit()
        return cursor.rowcount > 0

    # ============ K-Profile Operations ============

    async def get_spool_k_profiles(self, spool_id: str) -> list[dict]:
        """Get K-profiles associated with a spool."""
        async with self.conn.execute("SELECT * FROM k_profiles WHERE spool_id = ?", (spool_id,)) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    async def save_spool_k_profiles(self, spool_id: str, profiles: list[dict]) -> None:
        """Save K-profiles for a spool (replaces existing)."""
        # Delete existing profiles
        await self.conn.execute("DELETE FROM k_profiles WHERE spool_id = ?", (spool_id,))

        # Insert new profiles
        for profile in profiles:
            await self.conn.execute(
                """INSERT INTO k_profiles
                   (spool_id, printer_serial, extruder, nozzle_diameter, nozzle_type,
                    k_value, name, cali_idx, setting_id)
                   VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)""",
                (
                    spool_id,
                    profile.get("printer_serial"),
                    profile.get("extruder"),
                    profile.get("nozzle_diameter"),
                    profile.get("nozzle_type"),
                    profile.get("k_value"),
                    profile.get("name"),
                    profile.get("cali_idx"),
                    profile.get("setting_id"),
                ),
            )
        await self.conn.commit()

    async def delete_spool_k_profiles(self, spool_id: str) -> None:
        """Delete all K-profiles for a spool."""
        await self.conn.execute("DELETE FROM k_profiles WHERE spool_id = ?", (spool_id,))
        await self.conn.commit()

    # ============ Spool Catalog Operations ============

    async def seed_spool_catalog(self) -> None:
        """Seed the spool catalog with default entries if empty."""
        async with self.conn.execute("SELECT COUNT(*) FROM spool_catalog") as cursor:
            row = await cursor.fetchone()
            if row[0] > 0:
                return  # Already has data

        for name, weight in DEFAULT_SPOOL_CATALOG:
            await self.conn.execute(
                "INSERT OR IGNORE INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 1)", (name, weight)
            )
        await self.conn.commit()

    async def get_spool_catalog(self) -> list[dict]:
        """Get all spool catalog entries."""
        async with self.conn.execute(
            "SELECT id, name, weight, is_default, created_at FROM spool_catalog ORDER BY name"
        ) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    async def add_spool_catalog_entry(self, name: str, weight: int) -> dict:
        """Add a new spool catalog entry."""
        cursor = await self.conn.execute(
            "INSERT INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 0)", (name, weight)
        )
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, name, weight, is_default, created_at FROM spool_catalog WHERE id = ?", (cursor.lastrowid,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else {}

    async def update_spool_catalog_entry(self, entry_id: int, name: str, weight: int) -> dict | None:
        """Update a spool catalog entry."""
        await self.conn.execute("UPDATE spool_catalog SET name = ?, weight = ? WHERE id = ?", (name, weight, entry_id))
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, name, weight, is_default, created_at FROM spool_catalog WHERE id = ?", (entry_id,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else None

    async def delete_spool_catalog_entry(self, entry_id: int) -> bool:
        """Delete a spool catalog entry."""
        cursor = await self.conn.execute("DELETE FROM spool_catalog WHERE id = ?", (entry_id,))
        await self.conn.commit()
        return cursor.rowcount > 0

    async def reset_spool_catalog(self) -> None:
        """Reset spool catalog to defaults."""
        await self.conn.execute("DELETE FROM spool_catalog")
        for name, weight in DEFAULT_SPOOL_CATALOG:
            await self.conn.execute(
                "INSERT INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 1)", (name, weight)
            )
        await self.conn.commit()

    # ============ Color Catalog Operations ============

    async def seed_color_catalog(self) -> None:
        """Seed the color catalog with default entries if empty."""
        async with self.conn.execute("SELECT COUNT(*) FROM color_catalog") as cursor:
            row = await cursor.fetchone()
            if row[0] > 0:
                return  # Already has data

        for manufacturer, color_name, hex_color, material in DEFAULT_COLOR_CATALOG:
            await self.conn.execute(
                "INSERT OR IGNORE INTO color_catalog (manufacturer, color_name, hex_color, material, is_default) VALUES (?, ?, ?, ?, 1)",
                (manufacturer, color_name, hex_color, material),
            )
        await self.conn.commit()

    async def get_color_catalog(self) -> list[dict]:
        """Get all color catalog entries."""
        async with self.conn.execute(
            "SELECT id, manufacturer, color_name, hex_color, material, is_default, created_at FROM color_catalog ORDER BY manufacturer, material, color_name"
        ) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    async def add_color_catalog_entry(
        self, manufacturer: str, color_name: str, hex_color: str, material: str | None
    ) -> dict:
        """Add a new color catalog entry."""
        cursor = await self.conn.execute(
            "INSERT INTO color_catalog (manufacturer, color_name, hex_color, material, is_default) VALUES (?, ?, ?, ?, 0)",
            (manufacturer, color_name, hex_color, material),
        )
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, manufacturer, color_name, hex_color, material, is_default, created_at FROM color_catalog WHERE id = ?",
            (cursor.lastrowid,),
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else {}

    async def update_color_catalog_entry(
        self, entry_id: int, manufacturer: str, color_name: str, hex_color: str, material: str | None
    ) -> dict | None:
        """Update a color catalog entry."""
        await self.conn.execute(
            "UPDATE color_catalog SET manufacturer = ?, color_name = ?, hex_color = ?, material = ? WHERE id = ?",
            (manufacturer, color_name, hex_color, material, entry_id),
        )
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, manufacturer, color_name, hex_color, material, is_default, created_at FROM color_catalog WHERE id = ?",
            (entry_id,),
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else None

    async def delete_color_catalog_entry(self, entry_id: int) -> bool:
        """Delete a color catalog entry."""
        cursor = await self.conn.execute("DELETE FROM color_catalog WHERE id = ?", (entry_id,))
        await self.conn.commit()
        return cursor.rowcount > 0

    async def reset_color_catalog(self) -> None:
        """Reset color catalog to defaults."""
        await self.conn.execute("DELETE FROM color_catalog")
        for manufacturer, color_name, hex_color, material in DEFAULT_COLOR_CATALOG:
            await self.conn.execute(
                "INSERT INTO color_catalog (manufacturer, color_name, hex_color, material, is_default) VALUES (?, ?, ?, ?, 1)",
                (manufacturer, color_name, hex_color, material),
            )
        await self.conn.commit()

    async def lookup_color(self, manufacturer: str, color_name: str, material: str | None = None) -> dict | None:
        """Look up a color by manufacturer and color name, optionally filtering by material."""
        if material:
            async with self.conn.execute(
                "SELECT id, manufacturer, color_name, hex_color, material, is_default, created_at FROM color_catalog WHERE manufacturer = ? AND color_name = ? AND material = ?",
                (manufacturer, color_name, material),
            ) as cursor:
                row = await cursor.fetchone()
                return dict(row) if row else None
        else:
            async with self.conn.execute(
                "SELECT id, manufacturer, color_name, hex_color, material, is_default, created_at FROM color_catalog WHERE manufacturer = ? AND color_name = ? LIMIT 1",
                (manufacturer, color_name),
            ) as cursor:
                row = await cursor.fetchone()
                return dict(row) if row else None

    # ============ AMS Sensor History Operations ============

    async def record_ams_sensor(
        self,
        printer_serial: str,
        ams_id: int,
        humidity: float | None,
        humidity_raw: float | None,
        temperature: float | None,
    ) -> int:
        """Record AMS sensor reading (humidity/temperature)."""
        now = int(time.time())
        cursor = await self.conn.execute(
            """INSERT INTO ams_sensor_history (printer_serial, ams_id, humidity, humidity_raw, temperature, recorded_at)
               VALUES (?, ?, ?, ?, ?, ?)""",
            (printer_serial, ams_id, humidity, humidity_raw, temperature, now),
        )
        await self.conn.commit()
        return cursor.lastrowid

    async def get_ams_sensor_history(self, printer_serial: str, ams_id: int, hours: int = 24) -> list[dict]:
        """Get AMS sensor history for a given time range."""
        now = int(time.time())
        since = now - (hours * 3600)
        async with self.conn.execute(
            """SELECT humidity, humidity_raw, temperature, recorded_at
               FROM ams_sensor_history
               WHERE printer_serial = ? AND ams_id = ? AND recorded_at >= ?
               ORDER BY recorded_at ASC""",
            (printer_serial, ams_id, since),
        ) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    async def get_ams_sensor_stats(self, printer_serial: str, ams_id: int, hours: int = 24) -> dict:
        """Get AMS sensor statistics (min/max/avg) for a given time range."""
        now = int(time.time())
        since = now - (hours * 3600)
        async with self.conn.execute(
            """SELECT
                 MIN(humidity) as min_humidity,
                 MAX(humidity) as max_humidity,
                 AVG(humidity) as avg_humidity,
                 MIN(temperature) as min_temperature,
                 MAX(temperature) as max_temperature,
                 AVG(temperature) as avg_temperature,
                 COUNT(*) as count
               FROM ams_sensor_history
               WHERE printer_serial = ? AND ams_id = ? AND recorded_at >= ?""",
            (printer_serial, ams_id, since),
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else {}

    async def cleanup_ams_sensor_history(self, retention_days: int = 30) -> int:
        """Delete AMS sensor history older than retention period."""
        cutoff = int(time.time()) - (retention_days * 24 * 3600)
        cursor = await self.conn.execute("DELETE FROM ams_sensor_history WHERE recorded_at < ?", (cutoff,))
        await self.conn.commit()
        return cursor.rowcount


# Global database instance
_db: Database | None = None


async def get_db() -> Database:
    """Get database instance."""
    global _db
    if _db is None:
        _db = Database(settings.database_path)
        await _db.connect()
    return _db
