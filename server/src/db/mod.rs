mod schema;

pub use schema::*;

use sqlx::{sqlite::SqlitePoolOptions, SqlitePool};

/// Connect to SQLite database
pub async fn connect(database_url: &str) -> Result<SqlitePool, sqlx::Error> {
    let pool = SqlitePoolOptions::new()
        .max_connections(5)
        .connect(database_url)
        .await?;

    Ok(pool)
}

/// Run database migrations
pub async fn migrate(pool: &SqlitePool) -> Result<(), sqlx::Error> {
    sqlx::query(SCHEMA)
        .execute(pool)
        .await?;

    // Add auto_connect column to existing printers table if it doesn't exist
    sqlx::query("ALTER TABLE printers ADD COLUMN auto_connect INTEGER DEFAULT 0")
        .execute(pool)
        .await
        .ok(); // Ignore error if column already exists

    tracing::info!("Database migrations complete");
    Ok(())
}

/// Database schema - will be split into migrations later
const SCHEMA: &str = r#"
-- Spools table
CREATE TABLE IF NOT EXISTS spools (
    id TEXT PRIMARY KEY,
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
    slicer_filament TEXT,
    note TEXT,
    added_time INTEGER,
    encode_time INTEGER,
    added_full INTEGER DEFAULT 0,
    consumed_since_add REAL DEFAULT 0,
    consumed_since_weight REAL DEFAULT 0,
    data_origin TEXT,
    tag_type TEXT,
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
    auto_connect INTEGER DEFAULT 0
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

-- Index for faster lookups
CREATE INDEX IF NOT EXISTS idx_spools_tag_id ON spools(tag_id);
CREATE INDEX IF NOT EXISTS idx_spools_material ON spools(material);
CREATE INDEX IF NOT EXISTS idx_k_profiles_spool ON k_profiles(spool_id);
CREATE INDEX IF NOT EXISTS idx_usage_history_spool ON usage_history(spool_id);
"#;
