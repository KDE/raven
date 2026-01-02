// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Database migration system

use anyhow::Result;
use log::{debug, info};
use rusqlite::Connection;

/// Current database revision
const DATABASE_REVISION: u32 = 1;

/// Run all pending migrations
pub fn run_migrations(conn: &mut Connection) -> Result<()> {
    // Create metadata table if it doesn't exist
    conn.execute(
        "CREATE TABLE IF NOT EXISTS metadata (migrationId INTEGER NOT NULL)",
        [],
    )?;

    // Get current revision
    let current_revision = get_migration_version(conn)?;
    info!("Current database revision: {}", current_revision);

    if current_revision >= DATABASE_REVISION {
        info!("Database is up to date");
        return Ok(());
    }

    // Run migrations
    if current_revision < 1 {
        debug!("Running migration V1...");
        migrate_v1(conn)?;
        debug!("Finished migration V1");
    }

    // Update migration info
    conn.execute(
        "INSERT INTO metadata (migrationId) VALUES (?1)",
        [DATABASE_REVISION],
    )?;

    info!("Database migrated to revision {}", DATABASE_REVISION);
    Ok(())
}

/// Get the current migration version
pub fn get_migration_version(conn: &Connection) -> Result<u32> {
    let result: Result<u32, _> = conn.query_row(
        "SELECT migrationId FROM metadata ORDER BY migrationId DESC LIMIT 1",
        [],
        |row| row.get(0),
    );

    Ok(result.unwrap_or(0))
}

fn migrate_v1(conn: &mut Connection) -> Result<()> {
    let tx = conn.transaction()?;

    // Jobs table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS job (
            id INTEGER PRIMARY KEY,
            accountId TEXT,
            data TEXT,
            createdAt DATETIME,
            status TEXT
        )",
        [],
    )?;

    // Messages table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS message (
            id TEXT PRIMARY KEY,
            accountId TEXT,
            data TEXT,
            folderId TEXT,
            threadId TEXT,
            headerMessageId TEXT,
            gmailMessageId TEXT,
            gmailThreadId TEXT,
            subject TEXT,
            draft TINYINT(1),
            unread TINYINT(1),
            starred TINYINT(1),
            date DATETIME,
            remoteUID INTEGER
        )",
        [],
    )?;

    // Create indexes for message table
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_message_account_folder ON message(accountId, folderId)",
        [],
    )?;
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_message_thread ON message(threadId)",
        [],
    )?;
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_message_header_id ON message(headerMessageId)",
        [],
    )?;

    // Message body table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS message_body (
            id TEXT PRIMARY KEY,
            value TEXT,
            fetchedAt DATETIME
        )",
        [],
    )?;

    // Threads table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS thread (
            id TEXT PRIMARY KEY,
            accountId TEXT,
            data TEXT,
            gmailThreadId TEXT,
            subject TEXT,
            snippet TEXT,
            unread INTEGER,
            starred INTEGER,
            firstMessageTimestamp DATETIME,
            lastMessageTimestamp DATETIME
        )",
        [],
    )?;

    // Create index for thread table
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_thread_account ON thread(accountId)",
        [],
    )?;

    // Thread references table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS thread_reference (
            threadId TEXT,
            accountId TEXT,
            headerMessageId TEXT,
            PRIMARY KEY (threadId, accountId, headerMessageId)
        )",
        [],
    )?;

    // Thread folder association table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS thread_folder (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            accountId TEXT,
            threadId TEXT,
            folderId TEXT,
            FOREIGN KEY(threadId) REFERENCES thread(id),
            FOREIGN KEY(folderId) REFERENCES folder(id)
        )",
        [],
    )?;

    // Create index for thread_folder
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_thread_folder_thread ON thread_folder(threadId)",
        [],
    )?;
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_thread_folder_folder ON thread_folder(folderId)",
        [],
    )?;

    // Folders table (includes CONDSTORE/QRESYNC columns)
    tx.execute(
        "CREATE TABLE IF NOT EXISTS folder (
            id TEXT PRIMARY KEY,
            accountId TEXT,
            data TEXT,
            path TEXT,
            role TEXT,
            createdAt DATETIME,
            uidValidity INTEGER,
            uidNext INTEGER,
            highestModSeq INTEGER
        )",
        [],
    )?;

    // Create index for folder table
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_folder_account ON folder(accountId)",
        [],
    )?;

    // Labels table
    tx.execute(
        "CREATE TABLE IF NOT EXISTS label (
            id TEXT PRIMARY KEY,
            accountId TEXT,
            data TEXT,
            path TEXT,
            role TEXT,
            createdAt DATETIME
        )",
        [],
    )?;

    // Create index for label table
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_label_account ON label(accountId)",
        [],
    )?;

    // Files table (full attachment support)
    tx.execute(
        "CREATE TABLE IF NOT EXISTS file (
            id TEXT PRIMARY KEY,
            accountId TEXT NOT NULL,
            messageId TEXT NOT NULL,
            fileName TEXT NOT NULL,
            partId TEXT NOT NULL,
            contentId TEXT,
            contentType TEXT NOT NULL,
            size INTEGER NOT NULL,
            isInline INTEGER NOT NULL DEFAULT 0,
            downloaded INTEGER NOT NULL DEFAULT 0,
            data TEXT
        )",
        [],
    )?;

    // Create indexes for file table
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_file_message ON file(messageId)",
        [],
    )?;
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_file_account ON file(accountId)",
        [],
    )?;
    tx.execute(
        "CREATE INDEX IF NOT EXISTS idx_file_content_id ON file(contentId)",
        [],
    )?;

    tx.commit()?;
    Ok(())
}
