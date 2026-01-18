import aiosqlite
import uuid
import time
from pathlib import Path
from typing import Optional
from contextlib import asynccontextmanager

from config import settings
from models import Spool, SpoolCreate, SpoolUpdate, Printer, PrinterCreate, PrinterUpdate


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

-- Indexes
CREATE INDEX IF NOT EXISTS idx_spools_tag_id ON spools(tag_id);
CREATE INDEX IF NOT EXISTS idx_spools_material ON spools(material);
CREATE INDEX IF NOT EXISTS idx_k_profiles_spool ON k_profiles(spool_id);
CREATE INDEX IF NOT EXISTS idx_usage_history_spool ON usage_history(spool_id);
CREATE INDEX IF NOT EXISTS idx_spool_assignments_slot ON spool_assignments(printer_serial, ams_id, tray_id);
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


class Database:
    """Async SQLite database wrapper."""

    def __init__(self, db_path: Path):
        self.db_path = db_path
        self._connection: Optional[aiosqlite.Connection] = None

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

    async def _run_migrations(self):
        """Run database migrations for new columns."""
        # Check if spool_number column exists
        async with self.conn.execute("PRAGMA table_info(spools)") as cursor:
            columns = [row['name'] for row in await cursor.fetchall()]

        if 'spool_number' not in columns:
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

        if 'location' not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN location TEXT")
            await self.conn.commit()

        if 'ext_has_k' not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN ext_has_k INTEGER DEFAULT 0")
            await self.conn.commit()

        if 'slicer_filament_name' not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN slicer_filament_name TEXT")
            await self.conn.commit()

        if 'weight_used' not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN weight_used REAL DEFAULT 0")
            await self.conn.commit()

        if 'archived_at' not in columns:
            await self.conn.execute("ALTER TABLE spools ADD COLUMN archived_at INTEGER")
            await self.conn.commit()

        # Check printers table for nozzle_count
        async with self.conn.execute("PRAGMA table_info(printers)") as cursor:
            printer_columns = [row['name'] for row in await cursor.fetchall()]

        if 'nozzle_count' not in printer_columns:
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

    async def get_spool(self, spool_id: str) -> Optional[Spool]:
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
            (spool_id, spool_number, spool.tag_id, spool.material, spool.subtype, spool.color_name,
             spool.rgba, spool.brand, spool.label_weight, spool.core_weight,
             spool.weight_new, spool.weight_current, spool.slicer_filament, spool.slicer_filament_name,
             spool.location, spool.note, spool.data_origin, spool.tag_type, 1 if spool.ext_has_k else 0, now, now)
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def update_spool(self, spool_id: str, spool: SpoolUpdate) -> Optional[Spool]:
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
            if field == 'ext_has_k':
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

    async def archive_spool(self, spool_id: str) -> Optional[Spool]:
        """Archive a spool by setting archived_at timestamp."""
        now = int(time.time())
        await self.conn.execute(
            "UPDATE spools SET archived_at = ?, updated_at = ? WHERE id = ?",
            (now, now, spool_id)
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def restore_spool(self, spool_id: str) -> Optional[Spool]:
        """Restore an archived spool by clearing archived_at."""
        now = int(time.time())
        await self.conn.execute(
            "UPDATE spools SET archived_at = NULL, updated_at = ? WHERE id = ?",
            (now, spool_id)
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    async def get_spool_by_tag(self, tag_id: str, include_archived: bool = False) -> Optional[Spool]:
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
            "UPDATE spools SET tag_id = NULL, tag_type = NULL, updated_at = ? WHERE id = ?",
            (now, spool_id)
        )
        await self.conn.commit()

    async def link_tag_to_spool(
        self, spool_id: str, tag_id: str, tag_type: Optional[str] = None, data_origin: Optional[str] = None
    ) -> Optional[Spool]:
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
            return [Printer(**{**dict(row), 'auto_connect': bool(row['auto_connect'])}) for row in rows]

    async def get_printer(self, serial: str) -> Optional[Printer]:
        """Get a single printer by serial."""
        async with self.conn.execute("SELECT * FROM printers WHERE serial = ?", (serial,)) as cursor:
            row = await cursor.fetchone()
            if row:
                return Printer(**{**dict(row), 'auto_connect': bool(row['auto_connect'])})
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
            (printer.serial, printer.name, printer.model, printer.ip_address,
             printer.access_code, now, int(printer.auto_connect))
        )
        await self.conn.commit()
        return await self.get_printer(printer.serial)

    async def update_printer(self, serial: str, printer: PrinterUpdate) -> Optional[Printer]:
        """Update an existing printer."""
        existing = await self.get_printer(serial)
        if not existing:
            return None

        updates = []
        values = []
        for field, value in printer.model_dump(exclude_unset=True).items():
            if field == 'auto_connect':
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
            "UPDATE printers SET nozzle_count = ? WHERE serial = ?",
            (nozzle_count, serial)
        )
        await self.conn.commit()
        return cursor.rowcount > 0

    async def get_auto_connect_printers(self) -> list[Printer]:
        """Get printers with auto_connect enabled."""
        async with self.conn.execute("SELECT * FROM printers WHERE auto_connect = 1") as cursor:
            rows = await cursor.fetchall()
            return [Printer(**{**dict(row), 'auto_connect': True}) for row in rows]

    # ============ Spool Assignment Operations ============

    async def assign_spool_to_slot(
        self, spool_id: str, printer_serial: str, ams_id: int, tray_id: int
    ) -> bool:
        """Assign a spool to an AMS slot (upsert)."""
        now = int(time.time())
        await self.conn.execute(
            """INSERT INTO spool_assignments (spool_id, printer_serial, ams_id, tray_id, assigned_at)
               VALUES (?, ?, ?, ?, ?)
               ON CONFLICT(printer_serial, ams_id, tray_id) DO UPDATE SET
               spool_id = excluded.spool_id,
               assigned_at = excluded.assigned_at""",
            (spool_id, printer_serial, ams_id, tray_id, now)
        )
        await self.conn.commit()
        return True

    async def unassign_slot(self, printer_serial: str, ams_id: int, tray_id: int) -> bool:
        """Remove spool assignment from a slot."""
        cursor = await self.conn.execute(
            "DELETE FROM spool_assignments WHERE printer_serial = ? AND ams_id = ? AND tray_id = ?",
            (printer_serial, ams_id, tray_id)
        )
        await self.conn.commit()
        return cursor.rowcount > 0

    async def get_spool_for_slot(
        self, printer_serial: str, ams_id: int, tray_id: int
    ) -> Optional[str]:
        """Get spool ID assigned to a slot."""
        async with self.conn.execute(
            "SELECT spool_id FROM spool_assignments WHERE printer_serial = ? AND ams_id = ? AND tray_id = ?",
            (printer_serial, ams_id, tray_id)
        ) as cursor:
            row = await cursor.fetchone()
            return row['spool_id'] if row else None

    async def get_slot_assignments(self, printer_serial: str) -> list[dict]:
        """Get all spool assignments for a printer."""
        async with self.conn.execute(
            """SELECT sa.*, s.material, s.color_name, s.rgba, s.brand
               FROM spool_assignments sa
               LEFT JOIN spools s ON sa.spool_id = s.id
               WHERE sa.printer_serial = ?""",
            (printer_serial,)
        ) as cursor:
            rows = await cursor.fetchall()
            return [dict(row) for row in rows]

    # ============ Usage History Operations ============

    async def log_usage(
        self, spool_id: str, printer_serial: str, print_name: str, weight_used: float
    ) -> int:
        """Log filament usage for a print job."""
        cursor = await self.conn.execute(
            """INSERT INTO usage_history (spool_id, printer_serial, print_name, weight_used)
               VALUES (?, ?, ?, ?)""",
            (spool_id, printer_serial, print_name, weight_used)
        )
        await self.conn.commit()
        return cursor.lastrowid

    async def get_usage_history(self, spool_id: Optional[str] = None, limit: int = 100) -> list[dict]:
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
        self, spool_id: str, weight_used: float, new_weight: Optional[int] = None
    ) -> Optional[Spool]:
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

    async def set_spool_weight(self, spool_id: str, weight: int) -> Optional[Spool]:
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
            (weight, weight_used_new, now, spool_id)
        )
        await self.conn.commit()
        return await self.get_spool(spool_id)

    # ============ Settings Operations ============

    async def get_setting(self, key: str) -> Optional[str]:
        """Get a setting value by key."""
        async with self.conn.execute(
            "SELECT value FROM settings WHERE key = ?", (key,)
        ) as cursor:
            row = await cursor.fetchone()
            return row['value'] if row else None

    async def set_setting(self, key: str, value: str) -> None:
        """Set a setting value (upsert)."""
        now = int(time.time())
        await self.conn.execute(
            """INSERT INTO settings (key, value, updated_at)
               VALUES (?, ?, ?)
               ON CONFLICT(key) DO UPDATE SET
               value = excluded.value,
               updated_at = excluded.updated_at""",
            (key, value, now)
        )
        await self.conn.commit()

    async def delete_setting(self, key: str) -> bool:
        """Delete a setting."""
        cursor = await self.conn.execute(
            "DELETE FROM settings WHERE key = ?", (key,)
        )
        await self.conn.commit()
        return cursor.rowcount > 0

    # ============ K-Profile Operations ============

    async def get_spool_k_profiles(self, spool_id: str) -> list[dict]:
        """Get K-profiles associated with a spool."""
        async with self.conn.execute(
            "SELECT * FROM k_profiles WHERE spool_id = ?", (spool_id,)
        ) as cursor:
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
                (spool_id, profile.get('printer_serial'), profile.get('extruder'),
                 profile.get('nozzle_diameter'), profile.get('nozzle_type'),
                 profile.get('k_value'), profile.get('name'),
                 profile.get('cali_idx'), profile.get('setting_id'))
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
                "INSERT OR IGNORE INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 1)",
                (name, weight)
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
            "INSERT INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 0)",
            (name, weight)
        )
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, name, weight, is_default, created_at FROM spool_catalog WHERE id = ?",
            (cursor.lastrowid,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else {}

    async def update_spool_catalog_entry(self, entry_id: int, name: str, weight: int) -> Optional[dict]:
        """Update a spool catalog entry."""
        await self.conn.execute(
            "UPDATE spool_catalog SET name = ?, weight = ? WHERE id = ?",
            (name, weight, entry_id)
        )
        await self.conn.commit()
        async with self.conn.execute(
            "SELECT id, name, weight, is_default, created_at FROM spool_catalog WHERE id = ?",
            (entry_id,)
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
                "INSERT INTO spool_catalog (name, weight, is_default) VALUES (?, ?, 1)",
                (name, weight)
            )
        await self.conn.commit()


# Global database instance
_db: Optional[Database] = None


async def get_db() -> Database:
    """Get database instance."""
    global _db
    if _db is None:
        _db = Database(settings.database_path)
        await _db.connect()
    return _db
