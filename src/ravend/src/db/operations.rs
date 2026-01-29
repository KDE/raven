// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use crate::models::{Attachment, Folder, FolderRole, Message, MessageBody, MessageData, Thread, ThreadFolder, ThreadReference};
use anyhow::{Context, Result};
use chrono::{DateTime, Utc};
use rusqlite::{params, Connection, OptionalExtension};

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
    )?;
    Ok(())
}

pub fn get_folder_by_id(conn: &Connection, folder_id: &str) -> Result<Option<Folder>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, data, path, role, createdAt, uidValidity, uidNext, highestModSeq
         FROM folder WHERE id = ?1"
    )?;

    stmt.query_row([folder_id], |row| {
        let created_at_str: String = row.get(5)?;
        let created_at = DateTime::parse_from_rfc3339(&created_at_str)
            .map(|dt| dt.with_timezone(&Utc))
            .unwrap_or_else(|_| Utc::now());

        let role_str: Option<String> = row.get(4)?;
        let role = role_str
            .map(|s| match s.as_str() {
                "inbox" => FolderRole::Inbox,
                "sent" => FolderRole::Sent,
                "drafts" => FolderRole::Drafts,
                "trash" => FolderRole::Trash,
                "spam" => FolderRole::Spam,
                "archive" => FolderRole::Archive,
                "all" => FolderRole::All,
                "starred" => FolderRole::Starred,
                "important" => FolderRole::Important,
                other => FolderRole::Custom(other.to_string()),
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
    .with_context(|| format!("Failed to query folder '{}'", folder_id))
}

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
    )?;
    Ok(())
}

pub fn upsert_message(conn: &Connection, message: &Message) -> Result<()> {
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
    )?;
    Ok(())
}

pub fn get_message_by_uid(conn: &Connection, account_id: &str, folder_id: &str, remote_uid: u32) -> Result<Option<Message>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, data, folderId, threadId, headerMessageId,
                subject, draft, unread, starred, date, remoteUID
         FROM message WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3"
    )?;

    stmt.query_row(params![account_id, folder_id, remote_uid], |row| {
        let date_str: String = row.get(10)?;
        let date = DateTime::parse_from_rfc3339(&date_str)
            .map(|dt| dt.with_timezone(&Utc))
            .unwrap_or_else(|_| Utc::now());

        let data_json: Option<String> = row.get(2)?;
        let (snippet, is_plaintext) = data_json
            .and_then(|json| MessageData::from_json_string(&json).ok())
            .map(|data| (data.snippet, data.plaintext))
            .unwrap_or_default();

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
    .with_context(|| format!("Failed to query message UID {} in folder '{}'", remote_uid, folder_id))
}

pub fn update_message_flags(conn: &Connection, account_id: &str, folder_id: &str, remote_uid: u32, unread: bool, starred: bool, draft: bool) -> Result<bool> {
    let rows = conn.execute(
        "UPDATE message SET unread = ?4, starred = ?5, draft = ?6
         WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3",
        params![account_id, folder_id, remote_uid, unread, starred, draft],
    )?;
    Ok(rows > 0)
}

pub fn delete_message_by_uid(conn: &Connection, account_id: &str, folder_id: &str, remote_uid: u32) -> Result<bool> {
    let msg_id: Option<String> = conn.query_row(
        "SELECT id FROM message WHERE accountId = ?1 AND folderId = ?2 AND remoteUID = ?3",
        params![account_id, folder_id, remote_uid],
        |row| row.get(0),
    ).optional()?;

    if let Some(id) = msg_id {
        conn.execute("DELETE FROM message_body WHERE id = ?1", [&id])?;
        conn.execute("DELETE FROM file WHERE messageId = ?1", [&id])?;
        conn.execute("DELETE FROM message WHERE id = ?1", [&id])?;
        return Ok(true);
    }
    Ok(false)
}

pub fn get_folder_message_uids(conn: &Connection, account_id: &str, folder_id: &str) -> Result<Vec<u32>> {
    let mut stmt = conn.prepare(
        "SELECT remoteUID FROM message WHERE accountId = ?1 AND folderId = ?2"
    )?;
    let uids = stmt.query_map(params![account_id, folder_id], |row| row.get(0))?
        .collect::<std::result::Result<Vec<_>, _>>()?;
    Ok(uids)
}

pub fn upsert_message_body(conn: &Connection, body: &MessageBody) -> Result<()> {
    conn.execute(
        "INSERT INTO message_body (id, value, fetchedAt)
         VALUES (?1, ?2, ?3)
         ON CONFLICT(id) DO UPDATE SET
             value = excluded.value,
             fetchedAt = excluded.fetchedAt",
        params![body.id, body.value, body.fetched_at.to_rfc3339()],
    )?;
    Ok(())
}

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
    )?;
    Ok(())
}

pub fn get_thread_by_id(conn: &Connection, thread_id: &str) -> Result<Option<Thread>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, data, subject, snippet, unread, starred,
                firstMessageTimestamp, lastMessageTimestamp
         FROM thread WHERE id = ?1"
    )?;

    stmt.query_row([thread_id], |row| {
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
    }).optional()
    .with_context(|| format!("Failed to query thread '{}'", thread_id))
}

pub fn insert_thread_folder(conn: &Connection, tf: &ThreadFolder) -> Result<()> {
    conn.execute(
        "INSERT OR IGNORE INTO thread_folder (accountId, threadId, folderId)
         VALUES (?1, ?2, ?3)",
        params![tf.account_id, tf.thread_id, tf.folder_id],
    )?;
    Ok(())
}

pub fn thread_folder_exists(conn: &Connection, account_id: &str, thread_id: &str, folder_id: &str) -> Result<bool> {
    let count: i64 = conn.query_row(
        "SELECT COUNT(*) FROM thread_folder WHERE accountId = ?1 AND threadId = ?2 AND folderId = ?3",
        params![account_id, thread_id, folder_id],
        |row| row.get(0),
    )?;
    Ok(count > 0)
}

pub fn insert_thread_reference(conn: &Connection, tr: &ThreadReference) -> Result<()> {
    conn.execute(
        "INSERT OR IGNORE INTO thread_reference (threadId, accountId, headerMessageId)
         VALUES (?1, ?2, ?3)",
        params![tr.thread_id, tr.account_id, tr.header_message_id],
    )?;
    Ok(())
}

pub fn find_thread_by_message_id(conn: &Connection, account_id: &str, header_message_id: &str) -> Result<Option<String>> {
    conn.query_row(
        "SELECT threadId FROM thread_reference WHERE accountId = ?1 AND headerMessageId = ?2",
        params![account_id, header_message_id],
        |row| row.get(0),
    ).optional()
    .with_context(|| format!("Failed to find thread by Message-ID '{}'", header_message_id))
}

/// Delete all data for an account (atomic transaction)
pub fn delete_account_data(conn: &mut Connection, account_id: &str) -> Result<()> {
    let tx = conn.transaction()?;
    tx.execute("DELETE FROM message_body WHERE id IN (SELECT id FROM message WHERE accountId = ?1)", [account_id])?;
    tx.execute("DELETE FROM message WHERE accountId = ?1", [account_id])?;
    tx.execute("DELETE FROM thread_folder WHERE accountId = ?1", [account_id])?;
    tx.execute("DELETE FROM thread_reference WHERE accountId = ?1", [account_id])?;
    tx.execute("DELETE FROM thread WHERE accountId = ?1", [account_id])?;
    tx.execute("DELETE FROM folder WHERE accountId = ?1", [account_id])?;
    tx.execute("DELETE FROM file WHERE accountId = ?1", [account_id])?;
    tx.commit()?;
    Ok(())
}

pub fn upsert_file(conn: &Connection, file: &Attachment) -> Result<()> {
    conn.execute(
        "INSERT INTO file (id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)
         ON CONFLICT(id) DO UPDATE SET downloaded = excluded.downloaded",
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
    )?;
    Ok(())
}

pub fn get_files_by_message(conn: &Connection, message_id: &str) -> Result<Vec<Attachment>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded
         FROM file WHERE messageId = ?1"
    )?;

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
    })?.collect::<std::result::Result<Vec<_>, _>>()?;
    Ok(files)
}

pub fn get_file_by_id(conn: &Connection, file_id: &str) -> Result<Option<Attachment>> {
    let mut stmt = conn.prepare(
        "SELECT id, accountId, messageId, fileName, partId, contentId, contentType, size, isInline, downloaded
         FROM file WHERE id = ?1"
    )?;

    stmt.query_row([file_id], |row| {
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
    .with_context(|| format!("Failed to query file '{}'", file_id))
}

pub fn mark_file_downloaded(conn: &Connection, file_id: &str) -> Result<()> {
    conn.execute("UPDATE file SET downloaded = 1 WHERE id = ?1", [file_id])?;
    Ok(())
}

/// Message info needed for IMAP flag/move operations
#[derive(Debug, Clone)]
pub struct MessageActionInfo {
    pub id: String,
    pub account_id: String,
    pub folder_id: String,
    pub folder_path: String,
    pub remote_uid: u32,
    pub thread_id: String,
}

pub fn get_message_info_by_id(conn: &Connection, message_id: &str) -> Result<Option<MessageActionInfo>> {
    let mut stmt = conn.prepare(
        "SELECT m.id, m.accountId, m.folderId, f.path, m.remoteUID, m.threadId
         FROM message m
         JOIN folder f ON m.folderId = f.id
         WHERE m.id = ?1"
    )?;

    stmt.query_row([message_id], |row| {
        Ok(MessageActionInfo {
            id: row.get(0)?,
            account_id: row.get(1)?,
            folder_id: row.get(2)?,
            folder_path: row.get(3)?,
            remote_uid: row.get(4)?,
            thread_id: row.get(5)?,
        })
    }).optional()
    .with_context(|| format!("Failed to query message info for '{}'", message_id))
}

pub fn update_message_read_status_by_id(conn: &Connection, message_id: &str, unread: bool) -> Result<()> {
    conn.execute("UPDATE message SET unread = ?2 WHERE id = ?1", params![message_id, unread])?;
    Ok(())
}

pub fn update_message_starred_status_by_id(conn: &Connection, message_id: &str, starred: bool) -> Result<()> {
    conn.execute("UPDATE message SET starred = ?2 WHERE id = ?1", params![message_id, starred])?;
    Ok(())
}

pub fn get_trash_folder_for_account(conn: &Connection, account_id: &str) -> Result<Option<(String, String)>> {
    let mut stmt = conn.prepare(
        "SELECT id, path FROM folder WHERE accountId = ?1 AND role = 'trash'"
    )?;

    stmt.query_row([account_id], |row| {
        Ok((row.get::<_, String>(0)?, row.get::<_, String>(1)?))
    }).optional()
    .with_context(|| format!("Failed to query trash folder for account '{}'", account_id))
}

pub fn increment_thread_unread(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute("UPDATE thread SET unread = unread + 1 WHERE id = ?1", [thread_id])?;
    Ok(())
}

/// Decrement unread count (clamped to 0)
pub fn decrement_thread_unread(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute("UPDATE thread SET unread = MAX(0, unread - 1) WHERE id = ?1", [thread_id])?;
    Ok(())
}

pub fn increment_thread_starred(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute("UPDATE thread SET starred = starred + 1 WHERE id = ?1", [thread_id])?;
    Ok(())
}

/// Decrement starred count (clamped to 0)
pub fn decrement_thread_starred(conn: &Connection, thread_id: &str) -> Result<()> {
    conn.execute("UPDATE thread SET starred = MAX(0, starred - 1) WHERE id = ?1", [thread_id])?;
    Ok(())
}
