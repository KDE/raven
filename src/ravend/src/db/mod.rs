// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Database management module for the Raven mail daemon

mod migrations;
mod operations;

use anyhow::Result;
use rusqlite::Connection;
use std::path::Path;

pub use operations::*;

/// Database connection wrapper
pub struct Database {
    conn: Connection,
}

impl Database {
    /// Create a new database connection
    pub fn new(path: &Path) -> Result<Self> {
        let conn = Connection::open(path)?;

        // Enable foreign keys
        conn.execute_batch("PRAGMA foreign_keys = ON")?;

        // Enable WAL mode for better concurrency
        conn.execute_batch("PRAGMA journal_mode = WAL")?;

        // Set busy timeout to wait up to 5 seconds if database is locked
        // This prevents failures when the frontend is reading
        conn.execute_batch("PRAGMA busy_timeout = 5000")?;

        Ok(Self { conn })
    }

    /// Get the underlying connection
    pub fn conn(&self) -> &Connection {
        &self.conn
    }

    /// Get a mutable reference to the connection
    pub fn conn_mut(&mut self) -> &mut Connection {
        &mut self.conn
    }

    /// Run database migrations
    pub fn migrate(&mut self) -> Result<()> {
        migrations::run_migrations(&mut self.conn)
    }
}
