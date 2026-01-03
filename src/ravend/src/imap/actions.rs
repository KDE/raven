// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! IMAP message action functions
//!
//! This module provides functions for performing message actions (mark read/unread,
//! flag/unflag, move to trash, fetch attachments) via IMAP.

use crate::config::AccountManager;
use crate::db::{self, Database};
use crate::imap::connect_with_secrets;
use crate::models::{Account, Attachment};
use base64::{engine::general_purpose::STANDARD, Engine};
use dirs;
use log::{info, warn};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::io::Write;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};

// ============================================================================
// Action Result Types
// ============================================================================

/// Represents a failed action for a specific message
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FailedAction {
    pub message_id: String,
    pub error: String,
}

/// Result of a batch message action
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ActionResult {
    pub succeeded: Vec<String>,
    pub failed: Vec<FailedAction>,
}

impl ActionResult {
    pub fn new() -> Self {
        Self {
            succeeded: Vec::new(),
            failed: Vec::new(),
        }
    }

    pub fn add_success(&mut self, message_id: String) {
        self.succeeded.push(message_id);
    }

    pub fn add_failure(&mut self, message_id: String, error: String) {
        self.failed.push(FailedAction { message_id, error });
    }

    pub fn to_json(&self) -> String {
        serde_json::to_string(self).unwrap_or_else(|_| r#"{"succeeded":[],"failed":[]}"#.to_string())
    }
}

impl Default for ActionResult {
    fn default() -> Self {
        Self::new()
    }
}

// ============================================================================
// Flag Action Types
// ============================================================================

/// Flag action types for message operations
#[derive(Debug, Clone, Copy)]
pub enum FlagAction {
    MarkRead,
    MarkUnread,
    Flag,
    Unflag,
}

impl FlagAction {
    /// Get the IMAP flag string for this action
    pub fn imap_flag(&self) -> &'static str {
        match self {
            FlagAction::MarkRead | FlagAction::MarkUnread => "\\Seen",
            FlagAction::Flag | FlagAction::Unflag => "\\Flagged",
        }
    }

    /// Check if this action adds (true) or removes (false) the flag
    pub fn is_add(&self) -> bool {
        match self {
            FlagAction::MarkRead | FlagAction::Flag => true,
            FlagAction::MarkUnread | FlagAction::Unflag => false,
        }
    }
}

// ============================================================================
// IMAP Action Functions
// ============================================================================

/// Perform a flag action (mark read/unread, flag/unflag) on messages
///
/// This function handles:
/// 1. Looking up message info from database
/// 2. Grouping messages by account and folder for efficient IMAP operations
/// 3. Connecting to IMAP and performing STORE commands
/// 4. Updating local database with new flag states
/// 5. Updating thread counts
///
/// # Arguments
/// * `db` - Database connection
/// * `config_dir` - Path to configuration directory
/// * `message_ids` - List of message IDs to act on
/// * `action` - The flag action to perform
///
/// # Returns
/// ActionResult with lists of succeeded and failed message IDs
pub fn perform_flag_action(
    db: &Arc<Mutex<Database>>,
    message_ids: &[String],
    action: FlagAction,
) -> ActionResult {
    if message_ids.is_empty() {
        return ActionResult::new();
    }

    // Get message info from database and group by account
    let messages_by_account: HashMap<String, Vec<db::MessageActionInfo>> = {
        let db_guard = match db.lock() {
            Ok(db) => db,
            Err(e) => {
                let mut result = ActionResult::new();
                for id in message_ids {
                    result.add_failure(id.clone(), format!("Database lock failed: {}", e));
                }
                return result;
            }
        };

        let mut grouped: HashMap<String, Vec<db::MessageActionInfo>> = HashMap::new();
        let mut result = ActionResult::new();
        for id in message_ids {
            match db::get_message_info_by_id(db_guard.conn(), id) {
                Ok(Some(info)) => {
                    grouped.entry(info.account_id.clone()).or_default().push(info);
                }
                Ok(None) => {
                    result.add_failure(id.clone(), "Message not found".to_string());
                }
                Err(e) => {
                    result.add_failure(id.clone(), format!("Database error: {}", e));
                }
            }
        }
        // Note: early failures are logged but we proceed with what we have
        grouped
    };

    // Get accounts from cached list
    let accounts = AccountManager::global().accounts();

    let mut result = ActionResult::new();

    // Process each account
    for (account_id, messages) in messages_by_account {
        let account = match accounts.iter().find(|a| a.id == account_id) {
            Some(a) => a,
            None => {
                for msg in &messages {
                    result.add_failure(msg.id.clone(), "Account not found".to_string());
                }
                continue;
            }
        };

        // Group messages by folder for IMAP efficiency
        let mut by_folder: HashMap<String, Vec<&db::MessageActionInfo>> = HashMap::new();
        for msg in &messages {
            by_folder.entry(msg.folder_path.clone()).or_default().push(msg);
        }

        // Connect to IMAP
        let session_result = connect_with_secrets(account);
        let mut session = match session_result {
            Ok(s) => s,
            Err(e) => {
                for msg in &messages {
                    result.add_failure(msg.id.clone(), e.clone());
                }
                continue;
            }
        };

        // Process each folder
        for (folder_path, folder_messages) in by_folder {
            // Select folder
            if let Err(e) = session.select(&folder_path) {
                for msg in &folder_messages {
                    result.add_failure(msg.id.clone(), format!("Failed to select folder: {}", e));
                }
                continue;
            }

            // Build UID set
            let uids: Vec<String> = folder_messages.iter().map(|m| m.remote_uid.to_string()).collect();
            let uid_set = uids.join(",");

            // Execute STORE command
            let store_cmd = if action.is_add() {
                format!("+FLAGS ({})", action.imap_flag())
            } else {
                format!("-FLAGS ({})", action.imap_flag())
            };

            match session.uid_store(&uid_set, &store_cmd) {
                Ok(_) => {
                    // Update database
                    let db_guard = match db.lock() {
                        Ok(db) => db,
                        Err(e) => {
                            for msg in &folder_messages {
                                result.add_failure(msg.id.clone(), format!("Database lock failed: {}", e));
                            }
                            continue;
                        }
                    };

                    for msg in &folder_messages {
                        let db_result = match action {
                            FlagAction::MarkRead => db::update_message_read_status_by_id(db_guard.conn(), &msg.id, false),
                            FlagAction::MarkUnread => db::update_message_read_status_by_id(db_guard.conn(), &msg.id, true),
                            FlagAction::Flag => db::update_message_starred_status_by_id(db_guard.conn(), &msg.id, true),
                            FlagAction::Unflag => db::update_message_starred_status_by_id(db_guard.conn(), &msg.id, false),
                        };

                        match db_result {
                            Ok(_) => {
                                // Also update thread counts
                                let thread_result = match action {
                                    FlagAction::MarkRead => db::decrement_thread_unread(db_guard.conn(), &msg.thread_id),
                                    FlagAction::MarkUnread => db::increment_thread_unread(db_guard.conn(), &msg.thread_id),
                                    FlagAction::Flag => db::increment_thread_starred(db_guard.conn(), &msg.thread_id),
                                    FlagAction::Unflag => db::decrement_thread_starred(db_guard.conn(), &msg.thread_id),
                                };
                                if let Err(e) = thread_result {
                                    warn!("Failed to update thread counts: {}", e);
                                }
                                result.add_success(msg.id.clone());
                            }
                            Err(e) => result.add_failure(msg.id.clone(), format!("Database update failed: {}", e)),
                        }
                    }
                }
                Err(e) => {
                    for msg in &folder_messages {
                        result.add_failure(msg.id.clone(), format!("IMAP STORE failed: {}", e));
                    }
                }
            }
        }

        let _ = session.logout();
    }

    result
}

/// Move messages to trash folder
///
/// This function handles:
/// 1. Looking up message info and trash folder from database
/// 2. Grouping messages by account and folder
/// 3. Using IMAP MOVE command (or COPY+DELETE fallback)
/// 4. Deleting messages from local database (they'll be re-synced from trash)
///
/// # Arguments
/// * `db` - Database connection
/// * `message_ids` - List of message IDs to move to trash
///
/// # Returns
/// ActionResult with lists of succeeded and failed message IDs
pub fn move_to_trash(
    db: &Arc<Mutex<Database>>,
    message_ids: &[String],
) -> ActionResult {
    if message_ids.is_empty() {
        return ActionResult::new();
    }

    // Get message info from database and group by account
    // Also get trash folder info for each account
    let (messages_by_account, trash_folders): (
        HashMap<String, Vec<db::MessageActionInfo>>,
        HashMap<String, (String, String)>,  // account_id -> (trash_folder_id, trash_folder_path)
    ) = {
        let db_guard = match db.lock() {
            Ok(db) => db,
            Err(e) => {
                let mut result = ActionResult::new();
                for id in message_ids {
                    result.add_failure(id.clone(), format!("Database lock failed: {}", e));
                }
                return result;
            }
        };

        let mut grouped: HashMap<String, Vec<db::MessageActionInfo>> = HashMap::new();
        let mut trash_map: HashMap<String, (String, String)> = HashMap::new();

        for id in message_ids {
            match db::get_message_info_by_id(db_guard.conn(), id) {
                Ok(Some(info)) => {
                    // Get trash folder for this account if not already cached
                    if !trash_map.contains_key(&info.account_id) {
                        if let Ok(Some((trash_id, trash_path))) = db::get_trash_folder_for_account(db_guard.conn(), &info.account_id) {
                            trash_map.insert(info.account_id.clone(), (trash_id, trash_path));
                        }
                    }
                    grouped.entry(info.account_id.clone()).or_default().push(info);
                }
                Ok(None) => {
                    // Message not found - will be handled later
                }
                Err(_) => {
                    // Database error - will be handled later
                }
            }
        }
        (grouped, trash_map)
    };

    // Get accounts from cached list
    let accounts = AccountManager::global().accounts();

    let mut result = ActionResult::new();

    // Process each account
    for (account_id, messages) in messages_by_account {
        let account = match accounts.iter().find(|a| a.id == account_id) {
            Some(a) => a,
            None => {
                for msg in &messages {
                    result.add_failure(msg.id.clone(), "Account not found".to_string());
                }
                continue;
            }
        };

        // Get trash folder for this account
        let (_trash_folder_id, trash_folder_path) = match trash_folders.get(&account_id) {
            Some((id, path)) => (id.clone(), path.clone()),
            None => {
                for msg in &messages {
                    result.add_failure(msg.id.clone(), "Trash folder not found".to_string());
                }
                continue;
            }
        };

        // Group messages by folder
        let mut by_folder: HashMap<String, Vec<&db::MessageActionInfo>> = HashMap::new();
        for msg in &messages {
            // Skip if already in trash
            if msg.folder_path == trash_folder_path {
                result.add_success(msg.id.clone());
                continue;
            }
            by_folder.entry(msg.folder_path.clone()).or_default().push(msg);
        }

        if by_folder.is_empty() {
            continue;
        }

        // Connect to IMAP
        let session_result = connect_with_secrets(account);
        let mut session = match session_result {
            Ok(s) => s,
            Err(e) => {
                for msg in &messages {
                    if !result.succeeded.contains(&msg.id) {
                        result.add_failure(msg.id.clone(), e.clone());
                    }
                }
                continue;
            }
        };

        // Check for MOVE capability
        let has_move = session.capabilities()
            .map(|caps| caps.has_str("MOVE"))
            .unwrap_or(false);

        // Process each folder
        for (folder_path, folder_messages) in by_folder {
            // Select folder
            if let Err(e) = session.select(&folder_path) {
                for msg in &folder_messages {
                    result.add_failure(msg.id.clone(), format!("Failed to select folder: {}", e));
                }
                continue;
            }

            // Process each message individually for MOVE (to track new UIDs)
            for msg in folder_messages {
                let uid_str = msg.remote_uid.to_string();

                let move_result: std::result::Result<(), imap::Error> = if has_move {
                    // Use MOVE command
                    session.uid_mv(&uid_str, &trash_folder_path)
                } else {
                    // Fallback: COPY + DELETE + EXPUNGE
                    session.uid_copy(&uid_str, &trash_folder_path)
                        .and_then(|_| session.uid_store(&uid_str, "+FLAGS (\\Deleted)"))
                        .and_then(|_| session.expunge())
                        .map(|_| ())  // Discard expunge result
                };

                match move_result {
                    Ok(_) => {
                        // For now, we just delete the message from local DB
                        // The next sync will pick up the message in trash with new UID
                        // TODO: Could query for new UID with UIDPLUS extension
                        let mut db_guard = match db.lock() {
                            Ok(db) => db,
                            Err(e) => {
                                result.add_failure(msg.id.clone(), format!("Database lock failed: {}", e));
                                continue;
                            }
                        };

                        // Delete the message from local DB - it will be re-synced from trash folder
                        if let Err(e) = db::delete_message_by_uid(
                            db_guard.conn_mut(),
                            &msg.account_id,
                            &msg.folder_id,
                            msg.remote_uid,
                        ) {
                            warn!("Failed to delete moved message from DB: {}", e);
                        }

                        result.add_success(msg.id.clone());
                    }
                    Err(e) => {
                        result.add_failure(msg.id.clone(), format!("IMAP move failed: {}", e));
                    }
                }
            }
        }

        let _ = session.logout();
    }

    result
}

/// Fetch a specific attachment from IMAP server
///
/// # Arguments
/// * `account` - Account to fetch from
/// * `folder_path` - IMAP folder path
/// * `uid` - Message UID
/// * `file` - Attachment metadata
///
/// # Returns
/// File path on success, error message on failure
pub fn fetch_attachment_from_imap(
    account: &Account,
    folder_path: &str,
    uid: u32,
    file: &Attachment,
) -> Result<String, String> {
    // Connect to IMAP using the unified connection module
    let mut session = connect_with_secrets(account)?;

    // Select folder
    session
        .select(folder_path)
        .map_err(|e| format!("Failed to select folder {}: {}", folder_path, e))?;

    // Fetch the specific MIME part
    let fetch_query = format!("BODY.PEEK[{}]", file.part_id);
    let messages = session
        .uid_fetch(uid.to_string(), &fetch_query)
        .map_err(|e| format!("Failed to fetch attachment: {}", e))?;

    let message = messages
        .iter()
        .next()
        .ok_or_else(|| "No message returned".to_string())?;

    // Get the body data
    let body_data = message
        .body()
        .ok_or_else(|| "No body in response".to_string())?;

    // Decode if base64 encoded (most attachments are)
    let decoded_data = match STANDARD.decode(body_data) {
        Ok(decoded) => decoded,
        Err(_) => body_data.to_vec(),
    };

    // Save to disk
    let files_dir = dirs::data_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .join("raven/files")
        .join(&file.account_id);

    fs::create_dir_all(&files_dir)
        .map_err(|e| format!("Failed to create directory: {}", e))?;

    let file_path = files_dir.join(file.disk_filename());

    let mut out_file = fs::File::create(&file_path)
        .map_err(|e| format!("Failed to create file: {}", e))?;

    out_file
        .write_all(&decoded_data)
        .map_err(|e| format!("Failed to write file: {}", e))?;

    // Logout
    let _ = session.logout();

    info!("Attachment saved to: {:?}", file_path);
    Ok(file_path.to_string_lossy().to_string())
}
