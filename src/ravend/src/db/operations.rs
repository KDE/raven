// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Database CRUD operations

use crate::models::{Attachment, Folder, Message, MessageBody, MessageData, Thread, ThreadFolder, ThreadReference};
use anyhow::{Context, Result};
use chrono::{DateTime, Utc};
use rusqlite::{params, Connection, OptionalExtension};

// ============================================================================
// Folder operations
// ============================================================================

/// Insert or update a folder
/// Note: Does NOT update sync state (uidValidity, uidNext, highestModSeq) on conflict
/// Use update_folder_sync_state() to update sync state
pub fn upsert_folder(conn: &Connection, folder: &Folder) -> Result<()> {
    conn.execute(
        "INSERT INTO folder (id, accountId, data, path, role, createdAt, uidValidity, uidNext, highestModSeq)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
         ON CONFLICT(id) DO UPDATE SET
             data = excluded.data,
             path = excluded.path,
             role = excluded.role",
        params![
            folder.id,
            folder.account_id,
            folder.data,
            folder.path,
            folder.role.to_string(),
            folder.created_at.to_rfc3339(),
            folder.uid_validity,
            folder.uid_next,
            folder.highest_mod_seq,
        ],
    )
    .with_context(|| format!("Failed to upsert folder with id '{}' for account '{}'", folder.id, folder.account_id))?;
    Ok(())
}

/// Get a folder by ID with sync state
pub fn get_folder_by_id(conn: &Connection, folder_id: &str) -> Result<Option<Folder>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, data, path, role, createdAt, uidValidity, uidNext, highestModSeq
         FROM folder WHERE id = ?1"
    ).with_context(|| format!("Failed to prepare query for folder with id '{}'", folder_id))?;

    let folder = stmt.query_row([folder_id], |row| {
        let created_at_str: String = row.get(5)?;
        let created_at = DateTime::parse_from_rfc3339(&created_at_str)
            .map(|dt| dt.with_timezone(&Utc))
            .unwrap_or_else(|_| Utc::now());

        let role_str: Option<String> = row.get(4)?;
        let role = role_str
            .map(|s| match s.as_str() {
                "inbox" => crate::models::FolderRole::Inbox,
                "sent" => crate::models::FolderRole::Sent,
                "drafts" => crate::models::FolderRole::Drafts,
                "trash" => crate::models::FolderRole::Trash,
                "spam" => crate::models::FolderRole::Spam,
                "archive" => crate::models::FolderRole::Archive,
                "all" => crate::models::FolderRole::All,
                "starred" => crate::models::FolderRole::Starred,
                "important" => crate::models::FolderRole::Important,
                other => crate::models::FolderRole::Custom(other.to_string()),
            })
            .unwrap_or_default();

        Ok(Folder {
            id: row.get(0)?,
            account_id: row.get(1)?,
            data: row.get(2)?,
            path: row.get(3)?,
            role,
            created_at,
            uid_validity: row.get(6)?,
            uid_next: row.get(7)?,
            highest_mod_seq: row.get(8)?,
        })
    }).optional()
    .with_context(|| format!("Failed to query folder with id '{}'", folder_id))?;

    Ok(folder)
}

/// Update folder sync state (UIDVALIDITY, UIDNEXT, HIGHESTMODSEQ)
pub fn update_folder_sync_state(
    conn: &Connection,
    folder_id: &str,
    uid_validity: Option<u32>,
    uid_next: Option<u32>,
    highest_mod_seq: Option<u64>,
) -> Result<()> {
    conn.execute(
        "UPDATE folder SET uidValidity = ?2, uidNext = ?3, highestModSeq = ?4 WHERE id = ?1",
        params![folder_id, uid_validity, uid_next, highest_mod_seq],
    ).with_context(|| format!("Failed to update sync state for folder '{}'", folder_id))?;
    Ok(())
}

// ============================================================================
// Message operations
// ============================================================================

/// Insert or update a message
pub fn upsert_message(conn: &Connection, message: &Message) -> Result<()> {
    // Assemble the data field on-demand for database storage
    let data = message.get_data();

    conn.execute(
        "INSERT INTO message (id, accountId, data, folderId, threadId, headerMessageId,
                             subject, draft, unread, starred, date, remoteUID)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12)
         ON CONFLICT(id) DO UPDATE SET
             data = excluded.data,
             folderId = excluded.folderId,
             threadId = excluded.threadId,
             subject = excluded.subject,
             draft = excluded.draft,
             unread = excluded.unread,
             starred = excluded.starred,
             date = excluded.date",
        params![
            message.id,
            message.account_id,
            data,
            message.folder_id,
            message.thread_id,
            message.header_message_id,
            message.subject,
            message.draft,
            message.unread,
            message.starred,
            message.date.to_rfc3339(),
            message.remote_uid,
        ],
    )
    .with_context(|| format!("Failed to upsert message with id '{}' for account '{}'", message.id, message.account_id))?;
    Ok(())
}

/// Get message by remote UID in a folder
pub fn get_message_by_uid(
    conn: &Connection,
    account_id: &str,
    folder_id: &str,
    remote_uid: u32,
) -> Result<Option<Message>> {
    let mut stmt = conn
        .prepare(
            "SELECT id, accountId, data, folderId, threadId, headerMessageId,
                    subject, draft, unread, starred, date, remoteUID
             FROM message WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3",
        )
        .with_context(|| format!("Failed to prepare query for message with UID {} in folder '{}' for account '{}'", remote_uid, folder_id, account_id))?;

    let message = stmt
        .query_row(params![account_id, folder_id, remote_uid], |row| {
            let date_str: String = row.get(10)?;
            let date = DateTime::parse_from_rfc3339(&date_str)
                .map(|dt| dt.with_timezone(&Utc))
                .unwrap_or_else(|_| Utc::now());

            // Parse snippet and is_plaintext from the data JSON field
            let data_json: Option<String> = row.get(2)?;
            let (snippet, is_plaintext) = data_json
                .and_then(|json| MessageData::from_json_string(&json).ok())
                .map(|data| (data.snippet, data.plaintext))
                .unwrap_or_else(|| (String::new(), false));

            Ok(Message {
                id: row.get(0)?,
                account_id: row.get(1)?,
                folder_id: row.get(3)?,
                thread_id: row.get(4)?,
                header_message_id: row.get(5)?,
                subject: row.get(6)?,
                draft: row.get(7)?,
                unread: row.get(8)?,
                starred: row.get(9)?,
                date,
                remote_uid: row.get(11)?,
                from_contacts: "[]".to_string(),
                to_contacts: "[]".to_string(),
                cc_contacts: "[]".to_string(),
                bcc_contacts: "[]".to_string(),
                reply_to_contacts: "[]".to_string(),
                snippet,
                is_plaintext,
            })
        })
        .optional()
        .with_context(|| format!("Failed to query message with UID {} in folder '{}' for account '{}'", remote_uid, folder_id, account_id))?;

    Ok(message)
}

/// Update message flags (unread, starred, draft)
pub fn update_message_flags(
    conn: &Connection,
    account_id: &str,
    folder_id: &str,
    remote_uid: u32,
    unread: bool,
    starred: bool,
    draft: bool,
) -> Result<bool> {
    let rows_affected = conn.execute(
        "UPDATE message SET unread = ?4, starred = ?5, draft = ?6
         WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3",
        params![account_id, folder_id, remote_uid, unread, starred, draft],
    ).with_context(|| format!("Failed to update flags for message UID {} in folder '{}'", remote_uid, folder_id))?;
    Ok(rows_affected > 0)
}

/// Delete a message by UID (for expunged messages)
pub fn delete_message_by_uid(
    conn: &Connection,
    account_id: &str,
    folder_id: &str,
    remote_uid: u32,
) -> Result<bool> {
    // First get the message ID to delete the body and files
    let message_id: Option<String> = conn.query_row(
        "SELECT id FROM message WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3",
        params![account_id, folder_id, remote_uid],
        |row| row.get(0),
    ).optional()
    .with_context(|| format!("Failed to query message ID for UID {} in folder '{}'", remote_uid, folder_id))?;

    if let Some(id) = message_id {
        // Delete message body
        conn.execute("DELETE FROM message_body WHERE id = ?1", [&id])
            .with_context(|| format!("Failed to delete message body for message '{}'", id))?;

        // Delete associated files/attachments
        conn.execute("DELETE FROM file WHERE messageId = ?1", [&id])
            .with_context(|| format!("Failed to delete files for message '{}'", id))?;

        // Delete message
        conn.execute(
            "DELETE FROM message WHERE id = ?1",
            [&id],
        ).with_context(|| format!("Failed to delete message '{}'", id))?;

        return Ok(true);
    }
    Ok(false)
}

/// Get all remote UIDs for messages in a folder
pub fn get_folder_message_uids(
    conn: &Connection,
    account_id: &str,
    folder_id: &str,
) -> Result<Vec<u32>> {
    let mut stmt = conn.prepare(
        "SELECT remoteUID FROM message WHERE accountId = ?1 AND folderId = ?2"
    ).with_context(|| format!("Failed to prepare query for message UIDs in folder '{}'", folder_id))?;

    let uids = stmt.query_map(params![account_id, folder_id], |row| {
        row.get::<_, u32>(0)
    })?.collect::<std::result::Result<Vec<_>, _>>()
    .with_context(|| format!("Failed to collect message UIDs for folder '{}'", folder_id))?;

    Ok(uids)
}

/// Get a message by header Message-ID (for detecting moves)
///
/// FUTURE USE: This function will be useful for:
/// - Detecting when a message is moved between folders (same Message-ID, different folder)
/// - Deduplicating messages across folders
/// - Email threading improvements
#[allow(dead_code)]
pub fn get_message_by_header_id(
    conn: &Connection,
    account_id: &str,
    header_message_id: &str,
) -> Result<Option<Message>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, data, folderId, threadId, headerMessageId,
                subject, draft, unread, starred, date, remoteUID
         FROM message WHERE accountId = ?1 AND headerMessageId = ?2"
    ).with_context(|| format!("Failed to prepare query for message with header ID '{}'", header_message_id))?;

    let message = stmt.query_row(params![account_id, header_message_id], |row| {
        let date_str: String = row.get(10)?;
        let date = DateTime::parse_from_rfc3339(&date_str)
            .map(|dt| dt.with_timezone(&Utc))
            .unwrap_or_else(|_| Utc::now());

        let data_json: Option<String> = row.get(2)?;
        let (snippet, is_plaintext) = data_json
            .and_then(|json| MessageData::from_json_string(&json).ok())
            .map(|data| (data.snippet, data.plaintext))
            .unwrap_or_else(|| (String::new(), false));

        Ok(Message {
            id: row.get(0)?,
            account_id: row.get(1)?,
            folder_id: row.get(3)?,
            thread_id: row.get(4)?,
            header_message_id: row.get(5)?,
            subject: row.get(6)?,
            draft: row.get(7)?,
            unread: row.get(8)?,
            starred: row.get(9)?,
            date,
            remote_uid: row.get(11)?,
            from_contacts: "[]".to_string(),
            to_contacts: "[]".to_string(),
            cc_contacts: "[]".to_string(),
            bcc_contacts: "[]".to_string(),
            reply_to_contacts: "[]".to_string(),
            snippet,
            is_plaintext,
        })
    }).optional()
    .with_context(|| format!("Failed to query message with header ID '{}'", header_message_id))?;

    Ok(message)
}

/// Update a message's folder (for when message is moved)
///
/// FUTURE USE: This function will be useful for:
/// - Real-time move tracking (updating local DB when message is moved on server)
/// - Preserving message continuity when messages are moved between folders
#[allow(dead_code)]
pub fn update_message_folder(
    conn: &Connection,
    message_id: &str,
    new_folder_id: &str,
    new_remote_uid: u32,
) -> Result<()> {
    // Generate new message ID based on new folder
    let new_id = format!("{}:{}", new_folder_id, new_remote_uid);

    conn.execute(
        "UPDATE message SET id = ?2, folderId = ?3, remoteUID = ?4 WHERE id = ?1",
        params![message_id, new_id, new_folder_id, new_remote_uid],
    ).with_context(|| format!("Failed to update folder for message '{}'", message_id))?;

    // Also update message_body ID if it exists
    conn.execute(
        "UPDATE message_body SET id = ?2 WHERE id = ?1",
        params![message_id, new_id],
    ).with_context(|| format!("Failed to update message_body ID for message '{}'", message_id))?;

    Ok(())
}

/// Delete thread-folder association
///
/// FUTURE USE: This function will be useful for:
/// - Cleaning up thread-folder associations when all messages in a thread are removed from a folder
/// - Move tracking to maintain accurate folder associations
#[allow(dead_code)]
pub fn delete_thread_folder(
    conn: &Connection,
    account_id: &str,
    thread_id: &str,
    folder_id: &str,
) -> Result<()> {
    conn.execute(
        "DELETE FROM thread_folder WHERE accountId = ?1 AND threadId = ?2 AND folderId = ?3",
        params![account_id, thread_id, folder_id],
    ).with_context(|| format!("Failed to delete thread-folder for thread '{}' folder '{}'", thread_id, folder_id))?;
    Ok(())
}

// ============================================================================
// Message body operations
// ============================================================================

/// Insert or update a message body
pub fn upsert_message_body(conn: &Connection, body: &MessageBody) -> Result<()> {
    conn.execute(
        "INSERT INTO message_body (id, value, fetchedAt)
         VALUES (?1, ?2, ?3)
         ON CONFLICT(id) DO UPDATE SET
             value = excluded.value,
             fetchedAt = excluded.fetchedAt",
        params![body.id, body.value, body.fetched_at.to_rfc3339(),],
    )
    .with_context(|| format!("Failed to upsert message body for message id '{}'", body.id))?;
    Ok(())
}

// ============================================================================
// Thread operations
// ============================================================================

/// Insert or update a thread
pub fn upsert_thread(conn: &Connection, thread: &Thread) -> Result<()> {
    conn.execute(
        "INSERT INTO thread (id, accountId, data, subject, snippet,
                            unread, starred, firstMessageTimestamp, lastMessageTimestamp)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
         ON CONFLICT(id) DO UPDATE SET
             data = excluded.data,
             subject = excluded.subject,
             snippet = excluded.snippet,
             unread = excluded.unread,
             starred = excluded.starred,
             firstMessageTimestamp = excluded.firstMessageTimestamp,
             lastMessageTimestamp = excluded.lastMessageTimestamp",
        params![
            thread.id,
            thread.account_id,
            thread.data,
            thread.subject,
            thread.snippet,
            thread.unread_count,
            thread.starred_count,
            thread.first_message_timestamp.to_rfc3339(),
            thread.last_message_timestamp.to_rfc3339(),
        ],
    )
    .with_context(|| format!("Failed to upsert thread with id '{}' for account '{}'", thread.id, thread.account_id))?;
    Ok(())
}

/// Get a thread by ID
pub fn get_thread_by_id(conn: &Connection, thread_id: &str) -> Result<Option<Thread>> {
    let mut stmt = conn
        .prepare(
            "SELECT id, accountId, data, subject, snippet, unread, starred,
                    firstMessageTimestamp, lastMessageTimestamp
             FROM thread WHERE id = ?1",
        )
        .with_context(|| format!("Failed to prepare query for thread with id '{}'", thread_id))?;

    let thread = stmt
        .query_row([thread_id], |row| {
            let first_ts_str: String = row.get(7)?;
            let last_ts_str: String = row.get(8)?;

            let first_message_timestamp = DateTime::parse_from_rfc3339(&first_ts_str)
                .map(|dt| dt.with_timezone(&Utc))
                .unwrap_or_else(|_| Utc::now());
            let last_message_timestamp = DateTime::parse_from_rfc3339(&last_ts_str)
                .map(|dt| dt.with_timezone(&Utc))
                .unwrap_or_else(|_| Utc::now());

            Ok(Thread {
                id: row.get(0)?,
                account_id: row.get(1)?,
                data: row.get(2)?,
                subject: row.get(3)?,
                snippet: row.get(4)?,
                unread_count: row.get(5)?,
                starred_count: row.get(6)?,
                first_message_timestamp,
                last_message_timestamp,
            })
        })
        .optional()
        .with_context(|| format!("Failed to query thread with id '{}'", thread_id))?;

    Ok(thread)
}

// ============================================================================
// Thread folder operations
// ============================================================================

/// Insert a thread-folder association
pub fn insert_thread_folder(conn: &Connection, tf: &ThreadFolder) -> Result<()> {
    conn.execute(
        "INSERT OR IGNORE INTO thread_folder (accountId, threadId, folderId)
         VALUES (?1, ?2, ?3)",
        params![tf.account_id, tf.thread_id, tf.folder_id],
    )
    .with_context(|| format!("Failed to insert thread-folder association for thread '{}' and folder '{}' for account '{}'", tf.thread_id, tf.folder_id, tf.account_id))?;
    Ok(())
}

/// Check if a thread-folder association exists
pub fn thread_folder_exists(conn: &Connection, account_id: &str, thread_id: &str, folder_id: &str) -> Result<bool> {
    let count: i64 = conn.query_row(
        "SELECT COUNT(*) FROM thread_folder WHERE accountId = ?1 AND threadId = ?2 AND folderId = ?3",
        params![account_id, thread_id, folder_id],
        |row| row.get(0),
    ).with_context(|| format!("Failed to check thread-folder existence for thread '{}' and folder '{}'", thread_id, folder_id))?;
    Ok(count > 0)
}

// ============================================================================
// Thread reference operations (for email threading)
// ============================================================================

/// Insert a thread reference (Message-ID to thread mapping)
pub fn insert_thread_reference(conn: &Connection, tr: &ThreadReference) -> Result<()> {
    conn.execute(
        "INSERT OR IGNORE INTO thread_reference (threadId, accountId, headerMessageId)
         VALUES (?1, ?2, ?3)",
        params![tr.thread_id, tr.account_id, tr.header_message_id],
    )
    .with_context(|| format!("Failed to insert thread reference for Message-ID '{}'", tr.header_message_id))?;
    Ok(())
}

/// Find thread ID by header Message-ID
pub fn find_thread_by_message_id(conn: &Connection, account_id: &str, header_message_id: &str) -> Result<Option<String>> {
    let thread_id: Option<String> = conn.query_row(
        "SELECT threadId FROM thread_reference WHERE accountId = ?1 AND headerMessageId = ?2",
        params![account_id, header_message_id],
        |row| row.get(0),
    ).optional()
    .with_context(|| format!("Failed to find thread by Message-ID '{}'", header_message_id))?;
    Ok(thread_id)
}

// ============================================================================
// Cleanup operations
// ============================================================================

/// Delete all data for an account
/// This operation is atomic - all deletions succeed or all fail together
pub fn delete_account_data(conn: &mut Connection, account_id: &str) -> Result<()> {
    // Start a transaction to ensure atomicity
    let tx = conn
        .transaction()
        .with_context(|| format!("Failed to start transaction for deleting account data for account '{}'", account_id))?;

    // Delete message bodies first (referenced by messages)
    tx.execute(
        "DELETE FROM message_body WHERE id IN (SELECT id FROM message WHERE accountId = ?1)",
        [account_id],
    )
    .with_context(|| format!("Failed to delete message bodies for account '{}'", account_id))?;

    tx.execute("DELETE FROM message WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete messages for account '{}'", account_id))?;

    tx.execute("DELETE FROM thread_folder WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete thread-folder associations for account '{}'", account_id))?;

    tx.execute("DELETE FROM thread_reference WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete thread references for account '{}'", account_id))?;

    tx.execute("DELETE FROM thread WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete threads for account '{}'", account_id))?;

    tx.execute("DELETE FROM folder WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete folders for account '{}'", account_id))?;

    tx.execute("DELETE FROM file WHERE accountId = ?1", [account_id])
        .with_context(|| format!("Failed to delete files for account '{}'", account_id))?;

    // Commit the transaction
    tx.commit()
        .with_context(|| format!("Failed to commit transaction for deleting account data for account '{}'", account_id))?;

    Ok(())
}

// ============================================================================
// File/Attachment operations
// ============================================================================

/// Insert or update a file attachment record
pub fn upsert_file(conn: &Connection, file: &Attachment) -> Result<()> {
    conn.execute(
        "INSERT INTO file (id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)
         ON CONFLICT(id) DO UPDATE SET
             downloaded = excluded.downloaded",
        params![
            file.id,
            file.account_id,
            file.message_id,
            file.filename,
            file.part_id,
            file.content_id,
            file.content_type,
            file.size as i64,
            file.is_inline,
            file.downloaded,
        ],
    )
    .with_context(|| format!("Failed to upsert file '{}' for message '{}'", file.filename, file.message_id))?;
    Ok(())
}

/// Get all attachments for a message
pub fn get_files_by_message(conn: &Connection, message_id: &str) -> Result<Vec<Attachment>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded
         FROM file WHERE messageId = ?1"
    ).with_context(|| format!("Failed to prepare query for files for message '{}'", message_id))?;

    let files = stmt.query_map([message_id], |row| {
        Ok(Attachment {
            id: row.get(0)?,
            account_id: row.get(1)?,
            message_id: row.get(2)?,
            filename: row.get(3)?,
            part_id: row.get(4)?,
            content_id: row.get(5)?,
            content_type: row.get(6)?,
            size: row.get::<_, i64>(7)? as usize,
            is_inline: row.get(8)?,
            downloaded: row.get(9)?,
        })
    })?.collect::<std::result::Result<Vec<_>, _>>()
    .with_context(|| format!("Failed to collect files for message '{}'", message_id))?;

    Ok(files)
}

/// Get a file by its content ID (for inline image lookup)
///
/// FUTURE USE: This function will be useful for:
/// - Rendering HTML emails with inline images (cid: URLs)
/// - Resolving Content-ID references in multipart/related emails
#[allow(dead_code)]
pub fn get_file_by_content_id(conn: &Connection, message_id: &str, content_id: &str) -> Result<Option<Attachment>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded
         FROM file WHERE messageId = ?1 AND contentId = ?2"
    ).with_context(|| format!("Failed to prepare query for file with content ID '{}' for message '{}'", content_id, message_id))?;

    let file = stmt.query_row(params![message_id, content_id], |row| {
        Ok(Attachment {
            id: row.get(0)?,
            account_id: row.get(1)?,
            message_id: row.get(2)?,
            filename: row.get(3)?,
            part_id: row.get(4)?,
            content_id: row.get(5)?,
            content_type: row.get(6)?,
            size: row.get::<_, i64>(7)? as usize,
            is_inline: row.get(8)?,
            downloaded: row.get(9)?,
        })
    }).optional()
    .with_context(|| format!("Failed to query file with content ID '{}' for message '{}'", content_id, message_id))?;

    Ok(file)
}

/// Get a file by its ID
pub fn get_file_by_id(conn: &Connection, file_id: &str) -> Result<Option<Attachment>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded
         FROM file WHERE id = ?1"
    ).with_context(|| format!("Failed to prepare query for file with id '{}'", file_id))?;

    let file = stmt.query_row([file_id], |row| {
        Ok(Attachment {
            id: row.get(0)?,
            account_id: row.get(1)?,
            message_id: row.get(2)?,
            filename: row.get(3)?,
            part_id: row.get(4)?,
            content_id: row.get(5)?,
            content_type: row.get(6)?,
            size: row.get::<_, i64>(7)? as usize,
            is_inline: row.get(8)?,
            downloaded: row.get(9)?,
        })
    }).optional()
    .with_context(|| format!("Failed to query file with id '{}'", file_id))?;

    Ok(file)
}

/// Mark a file as downloaded
pub fn mark_file_downloaded(conn: &Connection, file_id: &str) -> Result<()> {
    conn.execute(
        "UPDATE file SET downloaded = 1 WHERE id = ?1",
        [file_id],
    ).with_context(|| format!("Failed to mark file '{}' as downloaded", file_id))?;
    Ok(())
}

// ============================================================================
// Message action operations (for D-Bus actions)
// ============================================================================

/// Get message info by ID for IMAP operations
pub fn get_message_info_by_id(conn: &Connection, message_id: &str) -> Result<Option<MessageActionInfo>> {
    let mut stmt = conn.prepare(
        "SELECT m.id, m.accountId, m.folderId, f.path, m.remoteUID, m.threadId
         FROM message m
         JOIN folder f ON m.folderId = f.id
         WHERE m.id = ?1"
    ).with_context(|| format!("Failed to prepare query for message info '{}'", message_id))?;

    let info = stmt.query_row([message_id], |row| {
        Ok(MessageActionInfo {
            id: row.get(0)?,
            account_id: row.get(1)?,
            folder_id: row.get(2)?,
            folder_path: row.get(3)?,
            remote_uid: row.get(4)?,
            thread_id: row.get(5)?,
        })
    }).optional()
    .with_context(|| format!("Failed to query message info for '{}'", message_id))?;

    Ok(info)
}

/// Message info for IMAP operations
#[derive(Debug, Clone)]
pub struct MessageActionInfo {
    pub id: String,
    pub account_id: String,
    pub folder_id: String,
    pub folder_path: String,
    pub remote_uid: u32,
    pub thread_id: String,
}

/// Update message read status by ID
pub fn update_message_read_status_by_id(conn: &Connection, message_id: &str, unread: bool) -> Result<()> {
    conn.execute(
        "UPDATE message SET unread = ?2 WHERE id = ?1",
        params![message_id, unread],
    ).with_context(|| format!("Failed to update read status for message '{}'", message_id))?;
    Ok(())
}

/// Update message starred status by ID
pub fn update_message_starred_status_by_id(conn: &Connection, message_id: &str, starred: bool) -> Result<()> {
    conn.execute(
        "UPDATE message SET starred = ?2 WHERE id = ?1",
        params![message_id, starred],
    ).with_context(|| format!("Failed to update starred status for message '{}'", message_id))?;
    Ok(())
}

/// Get trash folder for an account
pub fn get_trash_folder_for_account(conn: &Connection, account_id: &str) -> Result<Option<(String, String)>> {
    let mut stmt = conn.prepare(
        "SELECT id, path FROM folder WHERE accountId = ?1 AND role = 'trash'"
    ).with_context(|| format!("Failed to prepare query for trash folder for account '{}'", account_id))?;

    let result = stmt.query_row([account_id], |row| {
        Ok((row.get::<_, String>(0)?, row.get::<_, String>(1)?))
    }).optional()
    .with_context(|| format!("Failed to query trash folder for account '{}'", account_id))?;

    Ok(result)
}

/// Move a message to a different folder (updates folderId and generates new message ID)
///
/// FUTURE USE: This function will be useful for:
/// - User-initiated message move operations (e.g., Move to Archive)
/// - Drag-and-drop folder organization in the UI
/// - Note: Currently we delete and re-sync for moves, but this could be optimized
#[allow(dead_code)]
pub fn move_message_to_folder(
    conn: &Connection,
    message_id: &str,
    new_folder_id: &str,
    new_remote_uid: u32,
) -> Result<String> {
    // Generate new message ID based on new folder
    let new_id = format!("{}:{}", new_folder_id, new_remote_uid);

    // Update message
    conn.execute(
        "UPDATE message SET id = ?2, folderId = ?3, remoteUID = ?4 WHERE id = ?1",
        params![message_id, new_id, new_folder_id, new_remote_uid],
    ).with_context(|| format!("Failed to move message '{}' to folder '{}'", message_id, new_folder_id))?;

    // Update message_body ID if it exists
    conn.execute(
        "UPDATE message_body SET id = ?2 WHERE id = ?1",
        params![message_id, new_id],
    ).with_context(|| format!("Failed to update message_body ID for message '{}'", message_id))?;

    // Update file references
    conn.execute(
        "UPDATE file SET messageId = ?2 WHERE messageId = ?1",
        params![message_id, new_id],
    ).with_context(|| format!("Failed to update file references for message '{}'", message_id))?;

    Ok(new_id)
}

// ============================================================================
// Thread count update operations (for flag changes)
// ============================================================================

/// Increment thread unread count
pub fn increment_thread_unread(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute(
        "UPDATE thread SET unread = unread + 1 WHERE id = ?1",
        [thread_id],
    ).with_context(|| format!("Failed to increment unread count for thread '{}'", thread_id))?;
    Ok(())
}

/// Decrement thread unread count (won't go below 0)
pub fn decrement_thread_unread(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute(
        "UPDATE thread SET unread = MAX(0, unread - 1) WHERE id = ?1",
        [thread_id],
    ).with_context(|| format!("Failed to decrement unread count for thread '{}'", thread_id))?;
    Ok(())
}

/// Increment thread starred count
pub fn increment_thread_starred(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute(
        "UPDATE thread SET starred = starred + 1 WHERE id = ?1",
        [thread_id],
    ).with_context(|| format!("Failed to increment starred count for thread '{}'", thread_id))?;
    Ok(())
}

/// Decrement thread starred count (won't go below 0)
pub fn decrement_thread_starred(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute(
        "UPDATE thread SET starred = MAX(0, starred - 1) WHERE id = ?1",
        [thread_id],
    ).with_context(|| format!("Failed to decrement starred count for thread '{}'", thread_id))?;
    Ok(())
}
