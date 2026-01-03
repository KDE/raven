// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! IMAP worker that handles email synchronization for a single account.
//!
//! This module implements incremental sync using IMAP's UIDNEXT mechanism:
//! - First sync: Fetches the last N messages and stores UIDNEXT
//! - Subsequent syncs: Only fetches messages with UID >= stored UIDNEXT
//! - UIDVALIDITY tracking: Detects mailbox recreation and triggers full resync

use super::connection::{connect_and_authenticate, ImapSession};
use super::parser::{
    decode_header, format_sender, get_snippet_from_message, parse_message_body_full,
    replace_cid_urls, serialize_addresses,
};
use crate::db::{self, Database};
use crate::dbus::DBusNotifier;
use crate::secrets;
use crate::models::{
    Account, Attachment, AuthenticationType, Folder, Message, MessageBody, Thread,
    ThreadFolder, ThreadReference,
};
use crate::notifications::{NewEmailInfo, NotificationBatch};
use crate::oauth2::{find_provider_by_email, refresh_oauth2_token};
use anyhow::{Context, Result};
use chrono::{DateTime, Utc};
use imap::types::Fetch;
use log::{debug, error, info, warn};
use std::fs;
use std::io::Write;
use std::path::PathBuf;
use std::sync::mpsc::Receiver;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use uuid::Uuid;

// =============================================================================
// Constants
// =============================================================================

/// Maximum messages to fetch per folder during sync
const MAX_MESSAGES_PER_BATCH: u32 = 100;

/// IMAP IDLE timeout in seconds (RFC 2177 recommends max 29 minutes)
/// We use 25 minutes to have some margin before server timeout
const IDLE_TIMEOUT_SECS: u64 = 25 * 60;

// =============================================================================
// Types
// =============================================================================

/// Sync mode for a folder
#[derive(Debug, Clone, Copy, PartialEq)]
enum SyncMode {
    /// Full sync - fetch last N messages (first sync or UIDVALIDITY changed)
    Full,
    /// Incremental sync - only fetch new messages using UIDNEXT
    Incremental,
}

impl std::fmt::Display for SyncMode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            SyncMode::Full => write!(f, "full"),
            SyncMode::Incremental => write!(f, "incremental"),
        }
    }
}

/// Result of syncing a folder's messages
struct FolderSyncResult {
    new_messages: usize,
    deleted_messages: usize,
    flag_updates: usize,
}

/// Result of processing a single message
#[derive(Debug)]
enum MessageProcessResult {
    /// New message was inserted
    New,
    /// Existing message had flags updated
    FlagsUpdated,
    /// Message already exists and is unchanged
    Unchanged,
}

// =============================================================================
// ImapWorker
// =============================================================================

/// IMAP worker for synchronizing a single email account
pub struct ImapWorker {
    account: Account,
    db: Arc<Mutex<Database>>,
    files_dir: PathBuf,
    dbus_notifier: DBusNotifier,
}

impl ImapWorker {
    /// Create a new IMAP worker for the given account
    pub fn new(
        account: Account,
        db: Arc<Mutex<Database>>,
        files_dir: PathBuf,
        dbus_notifier: DBusNotifier,
    ) -> Self {
        Self {
            account,
            db,
            files_dir,
            dbus_notifier,
        }
    }

    /// Get the account email (for logging)
    pub fn account_email(&self) -> &str {
        &self.account.email
    }

    // =========================================================================
    // Main Sync Entry Point
    // =========================================================================

    /// Run sync with IMAP IDLE support for power efficiency
    ///
    /// This method:
    /// 1. Performs an initial full sync of all folders
    /// 2. If IDLE is supported, waits for new mail using IDLE on INBOX
    /// 3. If IDLE is not supported, falls back to polling at the specified interval
    ///
    /// The method returns after one sync cycle (either IDLE wakeup, poll interval, or sync trigger)
    pub fn sync_with_idle(
        &mut self,
        shutdown_rx: &Receiver<()>,
        sync_trigger_rx: &Receiver<()>,
        poll_interval_secs: u64,
    ) -> Result<()> {
        info!("[{}] Starting sync cycle", self.account.email);

        // Refresh OAuth2 token if needed
        if self.account.uses_oauth2() {
            self.refresh_oauth2_token_if_needed()?;
        }

        // Connect and authenticate
        let mut session = self.connect()?;

        // Check if server supports IDLE
        let supports_idle = self.check_idle_support(&mut session);

        if supports_idle {
            info!("[{}] Using IMAP IDLE for power-efficient sync", self.account.email);
        } else {
            info!("[{}] IDLE not supported, using polling ({}s interval)",
                  self.account.email, poll_interval_secs);
        }

        // Sync folder list from server
        let folders = self.sync_folders(&mut session)?;

        // Initial sync of all folders
        self.sync_all_folder_messages(&mut session, &folders)?;

        if supports_idle {
            // Use IDLE to wait for new mail
            self.idle_loop(&mut session, &folders, shutdown_rx, sync_trigger_rx)?;
        } else {
            // Fall back to polling - just wait for the interval
            self.poll_wait(shutdown_rx, sync_trigger_rx, poll_interval_secs)?;
        }

        // Clean up
        if let Err(e) = session.logout() {
            warn!("[{}] Failed to logout: {}", self.account.email, e);
        }

        Ok(())
    }

    /// Check if the server supports IMAP IDLE (RFC 2177)
    fn check_idle_support(&self, session: &mut ImapSession) -> bool {
        match session.capabilities() {
            Ok(caps) => {
                let has_idle = caps.has_str("IDLE");
                if has_idle {
                    debug!("[{}] Server supports IDLE", self.account.email);
                }
                has_idle
            }
            Err(e) => {
                warn!("[{}] Failed to get capabilities: {}", self.account.email, e);
                false
            }
        }
    }

    /// Wait using IMAP IDLE (power-efficient)
    /// Returns when new mail arrives, timeout occurs, or sync is triggered
    fn idle_loop(
        &self,
        session: &mut ImapSession,
        folders: &[Folder],
        shutdown_rx: &Receiver<()>,
        sync_trigger_rx: &Receiver<()>,
    ) -> Result<()> {
        use imap::extensions::idle::WaitOutcome;

        // Find INBOX folder
        let inbox = folders.iter().find(|f| f.path.eq_ignore_ascii_case("INBOX"));

        if inbox.is_none() {
            warn!("[{}] No INBOX folder found, cannot use IDLE", self.account.email);
            return Ok(());
        }

        // Select INBOX for IDLE
        session.select("INBOX")?;

        debug!("[{}] Entering IDLE mode on INBOX", self.account.email);

        // We use a shorter timeout (60s) and loop, checking for shutdown in between
        // This allows us to respond to shutdown signals while still being power-efficient
        let idle_check_interval = Duration::from_secs(60);
        let max_idle_time = Duration::from_secs(IDLE_TIMEOUT_SECS);
        let mut total_idle_time = Duration::ZERO;

        loop {
            // Check for shutdown signal before starting IDLE
            if shutdown_rx.try_recv().is_ok() {
                debug!("[{}] Shutdown signal received before IDLE", self.account.email);
                return Ok(());
            }

            // Check for sync trigger before starting IDLE
            if sync_trigger_rx.try_recv().is_ok() {
                info!("[{}] Manual sync triggered, exiting IDLE", self.account.email);
                return Ok(()); // Exit to do a sync immediately
            }

            // Start IDLE with timeout
            let idle_handle = session.idle()?;
            let idle_result = idle_handle.wait_with_timeout(idle_check_interval);

            match idle_result {
                Ok(WaitOutcome::MailboxChanged) => {
                    debug!("[{}] IDLE: Mailbox changed, new mail arrived", self.account.email);
                    return Ok(()); // Exit to sync new mail
                }
                Ok(WaitOutcome::TimedOut) => {
                    total_idle_time += idle_check_interval;
                    debug!("[{}] IDLE: Timeout (total: {:?})", self.account.email, total_idle_time);

                    // Check if we've been idle long enough
                    if total_idle_time >= max_idle_time {
                        debug!("[{}] IDLE: Max idle time reached, reconnecting", self.account.email);
                        return Ok(()); // Exit to do a full sync and reconnect
                    }

                    // Continue IDLE loop
                }
                Err(e) => {
                    // IDLE errors are often recoverable (connection issues, etc.)
                    warn!("[{}] IDLE error (will reconnect): {}", self.account.email, e);
                    return Ok(()); // Exit to reconnect
                }
            }
        }
    }

    /// Wait using polling (fallback when IDLE not supported)
    fn poll_wait(&self, shutdown_rx: &Receiver<()>, sync_trigger_rx: &Receiver<()>, interval_secs: u64) -> Result<()> {
        debug!("[{}] Polling wait for {}s", self.account.email, interval_secs);

        // Wait in 1-second increments to check for shutdown and sync triggers
        for _ in 0..interval_secs {
            if shutdown_rx.try_recv().is_ok() {
                debug!("[{}] Shutdown signal received during poll wait", self.account.email);
                return Ok(());
            }
            if sync_trigger_rx.try_recv().is_ok() {
                info!("[{}] Manual sync triggered during poll wait", self.account.email);
                return Ok(()); // Exit to do a sync immediately
            }
            std::thread::sleep(Duration::from_secs(1));
        }

        Ok(())
    }

    // =========================================================================
    // Connection
    // =========================================================================

    /// Connect to the IMAP server and authenticate using the unified connection module
    fn connect(&self) -> Result<ImapSession> {
        info!(
            "[{}] Connecting to {}:{}",
            self.account.email, self.account.imap_host, self.account.imap_port
        );

        // Get password or access token based on authentication type
        let password = match self.account.imap_authentication_type {
            AuthenticationType::Plain => Some(self.account.imap_password.trim()),
            _ => None,
        };

        let access_token = match self.account.imap_authentication_type {
            AuthenticationType::OAuth2 => self.account.oauth2_access_token.as_deref(),
            _ => None,
        };

        // Use unified connection function
        let session = connect_and_authenticate(&self.account, password, access_token)
            .map_err(|e| anyhow::anyhow!("{}", e))?;

        info!("[{}] Logged in", self.account.email);
        Ok(session)
    }

    // =========================================================================
    // OAuth2 Token Refresh
    // =========================================================================

    /// Refresh OAuth2 token if it's expired or about to expire
    fn refresh_oauth2_token_if_needed(&mut self) -> Result<()> {
        if !self.account.needs_token_refresh() {
            return Ok(());
        }

        info!("[{}] OAuth2 token needs refresh", self.account.email);

        let refresh_token = self.account.oauth2_refresh_token.clone()
            .ok_or_else(|| anyhow::anyhow!("No OAuth2 refresh token available"))?;

        let provider_id = self.account.oauth2_provider_id.clone()
            .or_else(|| find_provider_by_email(&self.account.email).map(|p| p.id.to_string()))
            .ok_or_else(|| anyhow::anyhow!("Cannot determine OAuth provider"))?;

        let refreshed = refresh_oauth2_token(&provider_id, &refresh_token)?;

        // Update account with new token
        self.account.oauth2_access_token = Some(refreshed.access_token.clone());
        if let Some(expires_in) = refreshed.expires_in {
            self.account.oauth2_token_expiry = Some(Utc::now().timestamp() + expires_in as i64);
        }

        // Store in secret store
        let key = secrets::oauth2_access_token_key(&self.account.id);
        if !secrets::write_password(&key, &refreshed.access_token) {
            warn!("[{}] Failed to save token to secret store", self.account.email);
        }

        info!("[{}] OAuth2 token refreshed", self.account.email);
        Ok(())
    }

    // =========================================================================
    // Folder Sync
    // =========================================================================

    /// Sync the folder list from the server
    fn sync_folders(&self, session: &mut ImapSession) -> Result<Vec<Folder>> {
        let mailboxes = session.list(Some(""), Some("*"))?;

        let db = self.db.lock().unwrap();
        let mut folders = Vec::new();

        for mailbox in mailboxes.iter() {
            let path = mailbox.name().to_string();
            let id = format!("{}:{}", self.account.id, path);
            let folder = Folder::new(id, self.account.id.clone(), path);

            if let Err(e) = db::upsert_folder(db.conn(), &folder) {
                error!("[{}] Failed to save folder: {}", self.account.email, e);
                continue;
            }

            folders.push(folder);
        }

        drop(db);

        if !folders.is_empty() {
            self.dbus_notifier.notify_folder_change();
        }

        info!("[{}] Synced {} folders", self.account.email, folders.len());
        Ok(folders)
    }

    // =========================================================================
    // Message Sync
    // =========================================================================

    /// Sync messages for all folders, prioritizing INBOX for quick notifications
    fn sync_all_folder_messages(&self, session: &mut ImapSession, folders: &[Folder]) -> Result<()> {
        let mut notification_batch = NotificationBatch::new(&self.account.email);

        // Sync INBOX first and send notifications immediately
        if let Some(inbox) = folders.iter().find(|f| f.path.eq_ignore_ascii_case("INBOX")) {
            if let Err(e) = self.sync_folder_messages(session, inbox, &mut notification_batch) {
                error!("[{}] Failed to sync INBOX: {}", self.account.email, e);
            }
            notification_batch.send_notifications();
        }

        // Sync remaining folders (no notifications)
        let mut empty_batch = NotificationBatch::new("");
        for folder in folders {
            if !folder.path.eq_ignore_ascii_case("INBOX") {
                if let Err(e) = self.sync_folder_messages(session, folder, &mut empty_batch) {
                    error!("[{}] Failed to sync {}: {}", self.account.email, folder.path, e);
                }
            }
        }

        Ok(())
    }

    /// Sync messages for a single folder
    fn sync_folder_messages(
        &self,
        session: &mut ImapSession,
        folder: &Folder,
        notification_batch: &mut NotificationBatch,
    ) -> Result<()> {
        // Load stored sync state
        let stored_state = self.load_folder_sync_state(&folder.id)?;

        // Select the folder and get server state
        let mailbox = session.select(&folder.path)?;
        let server_state = FolderState {
            exists: mailbox.exists,
            uid_validity: mailbox.uid_validity,
            uid_next: mailbox.uid_next,
        };

        // Determine sync mode
        let sync_mode = self.determine_sync_mode(&stored_state, &server_state);

        info!(
            "[{}] {} - sync mode: {}, exists: {}",
            self.account.email, folder.path, sync_mode, server_state.exists
        );

        // Handle UIDVALIDITY change (mailbox was recreated)
        if self.uid_validity_changed(&stored_state, &server_state) {
            warn!(
                "[{}] {} - UIDVALIDITY changed, clearing local messages",
                self.account.email, folder.path
            );
            self.clear_folder_messages(&folder.id)?;
        }

        // Handle empty folder - delete all local messages for this folder
        if server_state.exists == 0 {
            let deleted = self.delete_removed_messages(session, folder)?;
            self.save_folder_sync_state(&folder.id, &server_state)?;
            if deleted > 0 {
                info!("[{}] {} - {} deleted (folder now empty)", self.account.email, folder.path, deleted);
                self.dbus_notifier.notify_message_change();
            }
            return Ok(());
        }

        // Fetch and process messages (new + flag updates)
        let mut result = match sync_mode {
            SyncMode::Full => self.sync_folder_full(session, folder, notification_batch)?,
            SyncMode::Incremental => {
                self.sync_folder_incremental(session, folder, stored_state.uid_next, notification_batch)?
            }
        };

        // Detect and delete messages that were removed from this folder
        result.deleted_messages = self.delete_removed_messages(session, folder)?;

        // Save updated sync state
        self.save_folder_sync_state(&folder.id, &server_state)?;

        // Notify UI if there were any changes
        if result.new_messages > 0 || result.deleted_messages > 0 || result.flag_updates > 0 {
            self.dbus_notifier.notify_message_change();
        }

        info!(
            "[{}] {} - {} new, {} deleted, {} flag updates",
            self.account.email, folder.path, result.new_messages, result.deleted_messages, result.flag_updates
        );

        Ok(())
    }

    /// Full sync: fetch the last N messages
    fn sync_folder_full(
        &self,
        session: &mut ImapSession,
        folder: &Folder,
        notification_batch: &mut NotificationBatch,
    ) -> Result<FolderSyncResult> {
        let db = self.db.lock().unwrap();

        // Get message count from last SELECT
        let exists = session.select(&folder.path)?.exists;

        // Calculate range: last N messages
        let start = if exists > MAX_MESSAGES_PER_BATCH {
            exists - MAX_MESSAGES_PER_BATCH + 1
        } else {
            1
        };

        let fetch_range = format!("{}:*", start);
        let messages = session.fetch(&fetch_range, "(UID FLAGS ENVELOPE INTERNALDATE BODY.PEEK[])")?;

        let mut new_count = 0;
        let mut flag_updates = 0;

        for msg in messages.iter() {
            match self.process_message(&db, folder, msg, notification_batch) {
                Ok(MessageProcessResult::New) => new_count += 1,
                Ok(MessageProcessResult::FlagsUpdated) => flag_updates += 1,
                Ok(MessageProcessResult::Unchanged) => {}
                Err(e) => error!("[{}] Failed to process message: {}", self.account.email, e),
            }
        }

        Ok(FolderSyncResult {
            new_messages: new_count,
            deleted_messages: 0, // Calculated separately
            flag_updates,
        })
    }

    /// Incremental sync: fetch only new messages (UID >= stored UIDNEXT)
    fn sync_folder_incremental(
        &self,
        session: &mut ImapSession,
        folder: &Folder,
        stored_uid_next: Option<u32>,
        notification_batch: &mut NotificationBatch,
    ) -> Result<FolderSyncResult> {
        let uid_next = match stored_uid_next {
            Some(uid) => uid,
            None => return self.sync_folder_full(session, folder, notification_batch),
        };

        let db = self.db.lock().unwrap();

        // Fetch messages with UID >= stored UIDNEXT
        let fetch_range = format!("{}:*", uid_next);
        debug!(
            "[{}] {} - fetching UID {}:*",
            self.account.email, folder.path, uid_next
        );

        let messages = match session.uid_fetch(&fetch_range, "(UID FLAGS ENVELOPE INTERNALDATE BODY.PEEK[])") {
            Ok(msgs) => msgs,
            Err(e) => {
                debug!("[{}] {} - no new messages: {}", self.account.email, folder.path, e);
                return Ok(FolderSyncResult {
                    new_messages: 0,
                    deleted_messages: 0,
                    flag_updates: 0,
                });
            }
        };

        let mut new_count = 0;
        let mut flag_updates = 0;

        for msg in messages.iter() {
            // Skip messages with UID < stored UIDNEXT (edge case with UID:* range)
            if let Some(uid) = msg.uid {
                if uid < uid_next {
                    continue;
                }
            }

            match self.process_message(&db, folder, msg, notification_batch) {
                Ok(MessageProcessResult::New) => new_count += 1,
                Ok(MessageProcessResult::FlagsUpdated) => flag_updates += 1,
                Ok(MessageProcessResult::Unchanged) => {}
                Err(e) => error!("[{}] Failed to process message: {}", self.account.email, e),
            }
        }

        Ok(FolderSyncResult {
            new_messages: new_count,
            deleted_messages: 0, // Calculated separately
            flag_updates,
        })
    }

    // =========================================================================
    // Sync State Management
    // =========================================================================

    /// Load stored sync state for a folder
    fn load_folder_sync_state(&self, folder_id: &str) -> Result<StoredFolderState> {
        let db = self.db.lock().unwrap();
        let folder = db::get_folder_by_id(db.conn(), folder_id)?;

        Ok(StoredFolderState {
            uid_validity: folder.as_ref().and_then(|f| f.uid_validity),
            uid_next: folder.as_ref().and_then(|f| f.uid_next),
        })
    }

    /// Save sync state for a folder
    fn save_folder_sync_state(&self, folder_id: &str, state: &FolderState) -> Result<()> {
        let db = self.db.lock().unwrap();
        db::update_folder_sync_state(
            db.conn(),
            folder_id,
            state.uid_validity,
            state.uid_next,
            None, // HIGHESTMODSEQ not currently used
        )?;
        Ok(())
    }

    /// Determine sync mode based on stored and server state
    fn determine_sync_mode(&self, stored: &StoredFolderState, server: &FolderState) -> SyncMode {
        // If UIDVALIDITY changed, force full sync
        if self.uid_validity_changed(stored, server) {
            return SyncMode::Full;
        }

        // If we have no stored UIDNEXT, do full sync
        if stored.uid_next.is_none() {
            return SyncMode::Full;
        }

        // Otherwise, incremental sync
        SyncMode::Incremental
    }

    /// Check if UIDVALIDITY changed (mailbox was recreated)
    fn uid_validity_changed(&self, stored: &StoredFolderState, server: &FolderState) -> bool {
        match (stored.uid_validity, server.uid_validity) {
            (Some(stored_val), Some(server_val)) => stored_val != server_val,
            _ => false,
        }
    }

    /// Clear all messages for a folder (used when UIDVALIDITY changes)
    fn clear_folder_messages(&self, folder_id: &str) -> Result<()> {
        let db = self.db.lock().unwrap();
        db.conn().execute(
            "DELETE FROM message_body WHERE id IN (SELECT id FROM message WHERE folderId = ?1)",
            [folder_id],
        )?;
        db.conn().execute("DELETE FROM message WHERE folderId = ?1", [folder_id])?;
        Ok(())
    }

    /// Delete messages that no longer exist on the server (moved or deleted)
    fn delete_removed_messages(&self, session: &mut ImapSession, folder: &Folder) -> Result<usize> {
        // Get all UIDs currently on the server
        let server_uids: std::collections::HashSet<u32> = match session.uid_search("ALL") {
            Ok(uids) => uids.into_iter().collect(),
            Err(e) => {
                warn!("[{}] {} - failed to search UIDs: {}", self.account.email, folder.path, e);
                return Ok(0);
            }
        };

        // Get all UIDs we have stored in the database
        let db = self.db.lock().unwrap();
        let local_uids = db::get_folder_message_uids(db.conn(), &self.account.id, &folder.id)?;

        // Find UIDs that exist locally but not on server
        let mut deleted_count = 0;
        for local_uid in local_uids {
            if !server_uids.contains(&local_uid) {
                // Message was deleted or moved - remove from database
                if db::delete_message_by_uid(db.conn(), &self.account.id, &folder.id, local_uid)? {
                    debug!(
                        "[{}] {} - deleted message UID {} (no longer on server)",
                        self.account.email, folder.path, local_uid
                    );
                    deleted_count += 1;
                }
            }
        }

        Ok(deleted_count)
    }

    // =========================================================================
    // Message Processing
    // =========================================================================

    /// Process a single message from the server
    /// Returns the result of processing (new, moved, flags updated, or unchanged)
    fn process_message(
        &self,
        db: &Database,
        folder: &Folder,
        fetch: &Fetch,
        notification_batch: &mut NotificationBatch,
    ) -> Result<MessageProcessResult> {
        let uid = fetch.uid.ok_or_else(|| anyhow::anyhow!("Message has no UID"))?;

        // Extract flags from the fetch
        let mut is_seen = false;
        let mut is_flagged = false;
        let mut is_draft = false;
        for flag in fetch.flags() {
            match flag {
                imap::types::Flag::Seen => is_seen = true,
                imap::types::Flag::Flagged => is_flagged = true,
                imap::types::Flag::Draft => is_draft = true,
                _ => {}
            }
        }

        // Check if message already exists in THIS folder
        if let Some(existing) = db::get_message_by_uid(db.conn(), &self.account.id, &folder.id, uid)? {
            // Check if flags need updating
            let server_unread = !is_seen;
            let server_starred = is_flagged;
            let server_draft = is_draft;

            if existing.unread != server_unread || existing.starred != server_starred || existing.draft != server_draft {
                db::update_message_flags(
                    db.conn(),
                    &self.account.id,
                    &folder.id,
                    uid,
                    server_unread,
                    server_starred,
                    server_draft,
                )?;
                debug!("[{}] {} - updated flags for UID {}", self.account.email, folder.path, uid);
                return Ok(MessageProcessResult::FlagsUpdated);
            }
            return Ok(MessageProcessResult::Unchanged);
        }

        // Extract envelope
        let envelope = fetch
            .envelope()
            .ok_or_else(|| anyhow::anyhow!("Message has no envelope"))?;

        // Extract header Message-ID
        let header_message_id = envelope
            .message_id
            .as_ref()
            .and_then(|s| std::str::from_utf8(s).ok())
            .map(|s| s.to_string());

        // NOTE: We intentionally do NOT check for messages with the same header_message_id
        // in other folders. In Gmail (and other IMAP servers with labels/virtual folders),
        // the same email can legitimately exist in multiple folders simultaneously
        // (e.g., INBOX, [Gmail]/All Mail, [Gmail]/Important, custom labels).
        // Each folder gets its own message record with a unique ID.
        // The delete_removed_messages() function handles cleanup when an email
        // is removed from a folder (e.g., when archiving removes it from INBOX).

        // New message - create it
        let message_id = format!("{}:{}:{}", self.account.id, folder.id, uid);
        let mut message = Message::new(
            message_id.clone(),
            self.account.id.clone(),
            folder.id.clone(),
            uid,
        );

        // Extract subject
        message.subject = envelope
            .subject
            .as_ref()
            .and_then(|s| std::str::from_utf8(s).ok())
            .map(decode_header)
            .unwrap_or_default();

        // Extract date
        if let Some(date) = fetch.internal_date() {
            message.date = DateTime::from(date);
        }

        message.header_message_id = header_message_id;

        // Set flags
        message.unread = !is_seen;
        message.starred = is_flagged;
        message.draft = is_draft;

        // Extract contacts
        message.from_contacts = serialize_addresses(&envelope.from);
        message.to_contacts = serialize_addresses(&envelope.to);
        message.cc_contacts = serialize_addresses(&envelope.cc);
        message.bcc_contacts = serialize_addresses(&envelope.bcc);
        message.reply_to_contacts = serialize_addresses(&envelope.reply_to);

        // Parse body and extract attachments
        let parsed = if let Some(body_bytes) = fetch.body() {
            parse_message_body_full(body_bytes)
        } else {
            Default::default()
        };

        message.snippet = parsed.snippet;
        message.is_plaintext = parsed.is_plaintext;

        // Extract In-Reply-To for threading
        let in_reply_to_ids = self.extract_in_reply_to_ids(envelope);

        // Find or create thread
        let thread_id = self.find_or_create_thread(db, &message, folder, &in_reply_to_ids)?;
        message.thread_id = Some(thread_id);

        // Save message
        db::upsert_message(db.conn(), &message)?;

        // Process and save attachments
        // Skip inline attachments for spam folders as a security measure
        let is_spam_folder = folder.role == crate::models::FolderRole::Spam;
        let cid_replacements = self.process_attachments(db, &message_id, &parsed.attachments, is_spam_folder)?;

        // Replace cid: URLs with file:// URLs in HTML body
        let body_content = if !parsed.html_body.is_empty() {
            if !cid_replacements.is_empty() {
                replace_cid_urls(&parsed.html_body, &cid_replacements)
            } else {
                parsed.html_body
            }
        } else {
            parsed.text_body
        };

        // Save body
        if !body_content.is_empty() {
            let message_body = MessageBody::new(message_id, body_content);
            db::upsert_message_body(db.conn(), &message_body)?;
        }

        // Add to notification batch (only for unread INBOX messages)
        if folder.path.eq_ignore_ascii_case("INBOX") && message.unread {
            notification_batch.add_email(NewEmailInfo {
                sender: format_sender(&message.from_contacts),
                subject: message.subject.clone(),
                preview: message.snippet.clone(),
            });
        }

        debug!("Saved message: {} - {}", uid, message.subject);
        Ok(MessageProcessResult::New)
    }

    /// Extract In-Reply-To message IDs from envelope
    fn extract_in_reply_to_ids(&self, envelope: &imap_proto::Envelope) -> Vec<String> {
        envelope
            .in_reply_to
            .as_ref()
            .and_then(|s| std::str::from_utf8(s).ok())
            .map(|s| {
                s.split_whitespace()
                    .map(|id| id.to_string())
                    .collect::<Vec<String>>()
            })
            .unwrap_or_default()
    }

    // =========================================================================
    // Attachment Processing
    // =========================================================================

    /// Process attachments from a parsed message
    /// Returns a list of (content_id, file_url) pairs for cid: URL replacement
    /// If is_spam_folder is true, inline attachments are not saved to disk (security measure)
    fn process_attachments(
        &self,
        db: &Database,
        message_id: &str,
        parsed_attachments: &[crate::models::ParsedAttachment],
        is_spam_folder: bool,
    ) -> Result<Vec<(String, String)>> {
        if parsed_attachments.is_empty() {
            return Ok(Vec::new());
        }

        let mut cid_replacements = Vec::new();
        let account_files_dir = self.files_dir.join(&self.account.id);

        // Ensure the account's files directory exists
        if !parsed_attachments.is_empty() {
            if let Err(e) = fs::create_dir_all(&account_files_dir) {
                warn!(
                    "[{}] Failed to create files directory: {}",
                    self.account.email, e
                );
            }
        }

        for parsed in parsed_attachments {
            // Convert ParsedAttachment to Attachment for database storage
            let mut attachment = parsed.to_attachment(&self.account.id, message_id);

            // Skip saving inline attachments for spam folders (security measure to prevent
            // tracking pixels and potentially malicious content from being auto-loaded)
            let should_save = if is_spam_folder && attachment.is_inline {
                debug!(
                    "[{}] Skipping inline attachment '{}' in spam folder",
                    self.account.email, attachment.filename
                );
                false
            } else {
                true
            };

            // Save to disk if we have the data (small attachments) and should save
            if should_save {
                if let Some(ref data) = parsed.data {
                    match self.save_attachment_to_disk(&attachment, data, &account_files_dir) {
                        Ok(file_path) => {
                            attachment.downloaded = true;

                            // If this is an inline attachment with a content ID, add to cid replacements
                            if let Some(ref content_id) = attachment.content_id {
                                let file_url = format!("file://{}", file_path.display());
                                cid_replacements.push((content_id.clone(), file_url));
                            }
                        }
                        Err(e) => {
                            warn!(
                                "[{}] Failed to save attachment '{}': {}",
                                self.account.email, attachment.filename, e
                            );
                        }
                    }
                }
            }

            // Save attachment metadata to database
            if let Err(e) = db::upsert_file(db.conn(), &attachment) {
                error!(
                    "[{}] Failed to save attachment record '{}': {}",
                    self.account.email, attachment.filename, e
                );
            } else {
                debug!(
                    "[{}] Saved attachment: {} ({} bytes, inline: {}, downloaded: {})",
                    self.account.email, attachment.filename, attachment.size, attachment.is_inline, attachment.downloaded
                );
            }
        }

        Ok(cid_replacements)
    }

    /// Save attachment data to disk
    /// Returns the full path to the saved file
    fn save_attachment_to_disk(
        &self,
        attachment: &Attachment,
        data: &[u8],
        account_dir: &PathBuf,
    ) -> Result<PathBuf> {
        let filename = attachment.disk_filename();
        let file_path = account_dir.join(&filename);

        // Check if file already exists (avoid overwriting)
        if file_path.exists() {
            return Ok(file_path);
        }

        // Write file atomically using temp file
        let temp_path = account_dir.join(format!(".{}.tmp", filename));
        let mut file = fs::File::create(&temp_path)
            .with_context(|| format!("Failed to create temp file: {:?}", temp_path))?;

        file.write_all(data)
            .with_context(|| format!("Failed to write attachment data to {:?}", temp_path))?;

        file.sync_all()
            .with_context(|| format!("Failed to sync attachment file: {:?}", temp_path))?;

        // Rename temp file to final name
        fs::rename(&temp_path, &file_path)
            .with_context(|| format!("Failed to rename temp file to {:?}", file_path))?;

        Ok(file_path)
    }

    // =========================================================================
    // Threading
    // =========================================================================

    /// Find an existing thread or create a new one for a message
    fn find_or_create_thread(
        &self,
        db: &Database,
        message: &Message,
        folder: &Folder,
        in_reply_to_ids: &[String],
    ) -> Result<String> {
        let conn = db.conn();

        // Check if this message's own ID is registered (reply arrived first)
        if let Some(ref header_msg_id) = message.header_message_id {
            if let Some(thread_id) = db::find_thread_by_message_id(conn, &self.account.id, header_msg_id)? {
                debug!("Found thread {} via own Message-ID (reply arrived first)", thread_id);
                self.update_thread_with_message(db, &thread_id, message, folder)?;
                return Ok(thread_id);
            }
        }

        // Check In-Reply-To headers
        for reply_to_id in in_reply_to_ids {
            if let Some(thread_id) = db::find_thread_by_message_id(conn, &self.account.id, reply_to_id)? {
                debug!("Found thread {} via In-Reply-To", thread_id);
                self.update_thread_with_message(db, &thread_id, message, folder)?;

                // Register this message's ID for future replies
                if let Some(ref header_msg_id) = message.header_message_id {
                    let thread_ref = ThreadReference::new(
                        thread_id.clone(),
                        self.account.id.clone(),
                        header_msg_id.clone(),
                    );
                    let _ = db::insert_thread_reference(conn, &thread_ref);
                }

                return Ok(thread_id);
            }
        }

        // Create new thread
        let thread_id = Uuid::new_v4().to_string().replace("-", "");

        let mut thread = Thread::new(
            thread_id.clone(),
            self.account.id.clone(),
            message.subject.clone(),
        );

        thread.snippet = get_snippet_from_message(message);
        thread.first_message_timestamp = message.date;
        thread.last_message_timestamp = message.date;
        thread.unread_count = if message.unread { 1 } else { 0 };
        thread.starred_count = if message.starred { 1 } else { 0 };
        thread.data = Some(format!(
            r#"{{"participants":{},"folderIds":["{}"]}}"#,
            message.from_contacts, folder.id
        ));

        db::upsert_thread(conn, &thread)?;

        // Register this message's ID
        if let Some(ref header_msg_id) = message.header_message_id {
            let thread_ref = ThreadReference::new(
                thread_id.clone(),
                self.account.id.clone(),
                header_msg_id.clone(),
            );
            db::insert_thread_reference(conn, &thread_ref)?;
        }

        // Register In-Reply-To IDs (for out-of-order arrival)
        for reply_to_id in in_reply_to_ids {
            let thread_ref = ThreadReference::new(
                thread_id.clone(),
                self.account.id.clone(),
                reply_to_id.clone(),
            );
            let _ = db::insert_thread_reference(conn, &thread_ref);
        }

        // Create thread-folder association
        let thread_folder = ThreadFolder::new(
            self.account.id.clone(),
            thread_id.clone(),
            folder.id.clone(),
        );
        db::insert_thread_folder(conn, &thread_folder)?;

        debug!("Created thread: {} for: {}", thread_id, message.subject);
        Ok(thread_id)
    }

    /// Update an existing thread with a new message
    fn update_thread_with_message(
        &self,
        db: &Database,
        thread_id: &str,
        message: &Message,
        folder: &Folder,
    ) -> Result<()> {
        let conn = db.conn();

        if let Some(mut thread) = db::get_thread_by_id(conn, thread_id)? {
            // Update timestamps
            if message.date > thread.last_message_timestamp {
                thread.last_message_timestamp = message.date;
                thread.snippet = get_snippet_from_message(message);
            }
            if message.date < thread.first_message_timestamp {
                thread.first_message_timestamp = message.date;
            }

            // Update counts
            if message.unread {
                thread.unread_count += 1;
            }
            if message.starred {
                thread.starred_count += 1;
            }

            db::upsert_thread(conn, &thread)?;

            // Ensure thread-folder association
            if !db::thread_folder_exists(conn, &self.account.id, thread_id, &folder.id)? {
                let thread_folder = ThreadFolder::new(
                    self.account.id.clone(),
                    thread_id.to_string(),
                    folder.id.clone(),
                );
                db::insert_thread_folder(conn, &thread_folder)?;
            }
        }

        Ok(())
    }
}

// =============================================================================
// Helper Types
// =============================================================================

/// Stored sync state for a folder
struct StoredFolderState {
    uid_validity: Option<u32>,
    uid_next: Option<u32>,
}

/// Server state for a folder (from SELECT response)
struct FolderState {
    exists: u32,
    uid_validity: Option<u32>,
    uid_next: Option<u32>,
}
