/// Server configuration
#[derive(Debug, Clone)]
pub struct Config {
    /// Address to bind the server to
    pub bind_address: String,
    /// SQLite database URL
    pub database_url: String,
    /// Directory for static web files
    pub static_dir: String,
}

impl Config {
    /// Load configuration from environment variables with defaults
    pub fn from_env() -> Self {
        Self {
            bind_address: std::env::var("BIND_ADDRESS").unwrap_or_else(|_| "0.0.0.0:3000".into()),
            database_url: std::env::var("DATABASE_URL")
                .unwrap_or_else(|_| "sqlite:spoolbuddy.db?mode=rwc".into()),
            static_dir: std::env::var("STATIC_DIR").unwrap_or_else(|_| "../web/dist".into()),
        }
    }
}
