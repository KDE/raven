// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! D-Bus interface for Raven mail daemon
//!
//! This module provides:
//! - D-Bus signals to notify the frontend about database changes
//! - D-Bus methods for account management operations

use crate::config::AccountManager;
use crate::db::{self, Database};
use crate::imap::{self, ActionResult, FlagAction};
use crate::secrets;
use dirs;
use log::{debug, error, info, warn};
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use tokio::sync::{mpsc, oneshot};
use zbus::fdo::RequestNameFlags;
use zbus::fdo::RequestNameReply;
use zbus::{interface, Connection, Result};

/// D-Bus object path
const DBUS_PATH: &str = "/org/kde/raven/daemon";

/// D-Bus service name
const DBUS_SERVICE: &str = "org.kde.raven.daemon";

/// Table change event types
#[derive(Debug, Clone)]
pub enum TableChange {
    Folder,
    Label,
    Message,
    Account,
}

/// Notification event types
#[derive(Debug, Clone)]
pub enum NotificationEvent {
    TableChanged(TableChange),
    AccountReload,
    /// Trigger sync for specific account (empty string for all accounts)
    SyncTrigger(String),
}

impl TableChange {
    pub fn as_str(&self) -> &'static str {
        match self {
            TableChange::Folder => "folder",
            TableChange::Label => "label",
            TableChange::Message => "message",
            TableChange::Account => "account",
        }
    }
}

/// D-Bus interface implementation for Raven daemon
pub struct RavenDaemonDBus {
    db: Arc<Mutex<Database>>,
    notifier: DBusNotifier,
}

impl RavenDaemonDBus {
    pub fn new(db: Arc<Mutex<Database>>, notifier: DBusNotifier) -> Self {
        Self {
            db,
            notifier,
        }
    }
}

#[interface(name = "org.kde.raven.daemon")]
impl RavenDaemonDBus {
    /// Signal emitted when a database table is changed
    ///
    /// # Arguments
    /// * `table_name` - Name of the table that changed (folder, label, message, or account)
    #[zbus(signal)]
    async fn table_changed(signal_ctxt: &zbus::SignalContext<'_>, table_name: &str) -> Result<()>;

    /// Signal emitted when specific messages are updated (for UI refresh after actions)
    ///
    /// # Arguments
    /// * `message_ids` - List of message IDs that were updated
    #[zbus(signal)]
    async fn messages_changed(signal_ctxt: &zbus::SignalContext<'_>, message_ids: Vec<String>) -> Result<()>;

    /// Delete an account and all its associated data
    ///
    /// # Arguments
    /// * `account_id` - The ID of the account to delete
    ///
    /// # Returns
    /// * `true` if the account was deleted successfully, `false` otherwise
    async fn delete_account(&self, account_id: &str) -> bool {
        info!("D-Bus: deleteAccount called for account: {}", account_id);

        // Delete database data first
        let db_result = {
            let mut db = match self.db.lock() {
                Ok(db) => db,
                Err(e) => {
                    error!("Failed to acquire database lock: {}", e);
                    return false;
                }
            };

            db::delete_account_data(db.conn_mut(), account_id)
        };

        if let Err(e) = db_result {
            error!("Failed to delete account data from database: {}", e);
            return false;
        }

        // Delete account config and secrets entries
        if let Err(e) = AccountManager::global().delete_account(account_id) {
            error!("Failed to delete account config: {}", e);
            return false;
        }

        // Notify about account change (for UI updates)
        self.notifier.notify_account_change();
        self.notifier.notify_folder_change();

        // Trigger account reload to stop the sync worker for this account
        self.notifier.notify_account_reload();

        info!("D-Bus: Account {} deleted successfully", account_id);
        true
    }

    /// Trigger a sync for a specific account
    ///
    /// # Arguments
    /// * `account_id` - The ID of the account to sync (empty string for all accounts)
    async fn trigger_sync(&self, account_id: &str) -> bool {
        info!("D-Bus: triggerSync called for account: {}", account_id);
        self.notifier.notify_sync_trigger(account_id.to_string());
        true
    }

    /// Reload the accounts list from filesystem
    ///
    /// This triggers the daemon to reload all account configurations from disk
    /// and start/stop workers as needed.
    async fn reload_accounts(&self) -> bool {
        info!("D-Bus: reloadAccounts called");
        self.notifier.notify_account_reload();
        true
    }

    /// Get the file path for an attachment if it has been downloaded
    ///
    /// # Arguments
    /// * `file_id` - The ID of the attachment file
    ///
    /// # Returns
    /// * The full file path if the attachment is downloaded, empty string otherwise
    async fn get_attachment_path(&self, file_id: &str) -> String {
        debug!("D-Bus: getAttachmentPath called for file: {}", file_id);

        let db = match self.db.lock() {
            Ok(db) => db,
            Err(e) => {
                error!("Failed to acquire database lock: {}", e);
                return String::new();
            }
        };

        // Look up file record
        let file = match db::get_file_by_id(db.conn(), file_id) {
            Ok(Some(f)) => f,
            Ok(None) => {
                debug!("Attachment not found: {}", file_id);
                return String::new();
            }
            Err(e) => {
                error!("Failed to query attachment: {}", e);
                return String::new();
            }
        };

        // Check if downloaded
        if !file.downloaded {
            debug!("Attachment not yet downloaded: {}", file_id);
            return String::new();
        }

        // Build the file path
        let files_dir = dirs::data_dir()
            .unwrap_or_else(|| std::path::PathBuf::from("."))
            .join("raven")
            .join("files")
            .join(&file.account_id);

        let file_path = files_dir.join(file.disk_filename());

        if file_path.exists() {
            file_path.to_string_lossy().to_string()
        } else {
            warn!(
                "Attachment marked as downloaded but file not found: {:?}",
                file_path
            );
            String::new()
        }
    }

    /// Get all attachments for a message as a JSON array
    ///
    /// # Arguments
    /// * `message_id` - The ID of the message
    ///
    /// # Returns
    /// * JSON array of attachment objects with fields: id, filename, contentType, size, isInline, downloaded
    async fn get_message_attachments(&self, message_id: &str) -> String {
        debug!("D-Bus: getMessageAttachments called for message: {}", message_id);

        let db = match self.db.lock() {
            Ok(db) => db,
            Err(e) => {
                error!("Failed to acquire database lock: {}", e);
                return "[]".to_string();
            }
        };

        // Get all attachments for the message
        let files = match db::get_files_by_message(db.conn(), message_id) {
            Ok(f) => f,
            Err(e) => {
                error!("Failed to query attachments for message {}: {}", message_id, e);
                return "[]".to_string();
            }
        };

        // Convert to JSON array
        let json_array: Vec<serde_json::Value> = files
            .iter()
            .map(|f| {
                serde_json::json!({
                    "id": f.id,
                    "filename": f.filename,
                    "contentType": f.content_type,
                    "size": f.size,
                    "isInline": f.is_inline,
                    "downloaded": f.downloaded
                })
            })
            .collect();

        serde_json::to_string(&json_array).unwrap_or_else(|_| "[]".to_string())
    }

    /// Fetch an attachment from the server and save it to disk
    ///
    /// # Arguments
    /// * `file_id` - The ID of the attachment file to fetch
    ///
    /// # Returns
    /// * The full file path if successful, empty string on error
    async fn fetch_attachment(&self, file_id: &str) -> String {
        info!("D-Bus: fetchAttachment called for file: {}", file_id);

        // Get file record from database
        let file = {
            let db = match self.db.lock() {
                Ok(db) => db,
                Err(e) => {
                    error!("Failed to acquire database lock: {}", e);
                    return String::new();
                }
            };

            match db::get_file_by_id(db.conn(), file_id) {
                Ok(Some(f)) => f,
                Ok(None) => {
                    error!("Attachment not found: {}", file_id);
                    return String::new();
                }
                Err(e) => {
                    error!("Failed to query attachment: {}", e);
                    return String::new();
                }
            }
        };

        // If already downloaded, just return the path
        if file.downloaded {
            let files_dir = dirs::data_dir()
                .unwrap_or_else(|| PathBuf::from("."))
                .join("raven/files")
                .join(&file.account_id);
            let file_path = files_dir.join(file.disk_filename());
            if file_path.exists() {
                return file_path.to_string_lossy().to_string();
            }
        }

        // Parse message_id to get folder path and UID
        // Format: {account_id}:{folder_id}:{uid} where folder_id is {account_id}:{folder_path}
        let parts: Vec<&str> = file.message_id.split(':').collect();
        if parts.len() < 4 {
            error!("Invalid message_id format: {}", file.message_id);
            return String::new();
        }
        let folder_path = parts[2].to_string();
        let uid: u32 = match parts[3].parse() {
            Ok(u) => u,
            Err(_) => {
                error!("Invalid UID in message_id: {}", file.message_id);
                return String::new();
            }
        };

        // Get account from cached list
        let accounts = AccountManager::global().accounts();
        let account = match accounts.iter().find(|a| a.id == file.account_id) {
            Some(a) => a.clone(),
            None => {
                error!("Account not found: {}", file.account_id);
                return String::new();
            }
        };

        // Clone data for the blocking task
        let db_clone = Arc::clone(&self.db);
        let notifier = self.notifier.clone();
        let file_id_owned = file_id.to_string();

        // Run blocking IMAP operations in a separate thread to avoid runtime nesting
        let result = tokio::task::spawn_blocking(move || {
            // Fetch the attachment from IMAP (blocking operation)
            match imap::fetch_attachment_from_imap(&account, &folder_path, uid, &file) {
                Ok(path) => {
                    // Mark as downloaded in database
                    let db = match db_clone.lock() {
                        Ok(db) => db,
                        Err(e) => {
                            error!("Failed to acquire database lock: {}", e);
                            return path;
                        }
                    };
                    if let Err(e) = db::mark_file_downloaded(db.conn(), &file_id_owned) {
                        error!("Failed to mark file as downloaded: {}", e);
                    }

                    // Notify frontend about the change
                    notifier.notify_message_change();

                    path
                }
                Err(e) => {
                    error!("Failed to fetch attachment: {}", e);
                    String::new()
                }
            }
        }).await.unwrap_or_else(|e| {
            error!("Task failed: {}", e);
            String::new()
        });

        result
    }

    // ========================================================================
    // Message Action Methods
    // ========================================================================

    /// Mark messages as read
    ///
    /// # Arguments
    /// * `message_ids` - List of message IDs to mark as read
    ///
    /// # Returns
    /// * JSON string containing ActionResult with succeeded and failed lists
    #[zbus(name = "MarkAsRead")]
    async fn mark_as_read(&self, message_ids: Vec<String>) -> String {
        info!("D-Bus: MarkAsRead called for {} messages", message_ids.len());
        let db = Arc::clone(&self.db);
        let notifier = self.notifier.clone();

        let result = tokio::task::spawn_blocking(move || {
            let result = imap::perform_flag_action(&db, &message_ids, FlagAction::MarkRead);
            if !result.succeeded.is_empty() {
                notifier.notify_message_change();
            }
            result
        }).await.unwrap_or_else(|e| {
            error!("Task failed: {}", e);
            ActionResult::new()
        });

        result.to_json()
    }

    /// Mark messages as unread
    ///
    /// # Arguments
    /// * `message_ids` - List of message IDs to mark as unread
    ///
    /// # Returns
    /// * JSON string containing ActionResult with succeeded and failed lists
    #[zbus(name = "MarkAsUnread")]
    async fn mark_as_unread(&self, message_ids: Vec<String>) -> String {
        info!("D-Bus: MarkAsUnread called for {} messages", message_ids.len());
        let db = Arc::clone(&self.db);
        let notifier = self.notifier.clone();

        let result = tokio::task::spawn_blocking(move || {
            let result = imap::perform_flag_action(&db, &message_ids, FlagAction::MarkUnread);
            if !result.succeeded.is_empty() {
                notifier.notify_message_change();
            }
            result
        }).await.unwrap_or_else(|e| {
            error!("Task failed: {}", e);
            ActionResult::new()
        });

        result.to_json()
    }

    /// Set or unset the flagged (starred) status of messages
    ///
    /// # Arguments
    /// * `message_ids` - List of message IDs to update
    /// * `flagged` - true to flag, false to unflag
    ///
    /// # Returns
    /// * JSON string containing ActionResult with succeeded and failed lists
    #[zbus(name = "SetFlagged")]
    async fn set_flagged(&self, message_ids: Vec<String>, flagged: bool) -> String {
        info!("D-Bus: SetFlagged({}) called for {} messages", flagged, message_ids.len());
        let action = if flagged { FlagAction::Flag } else { FlagAction::Unflag };
        let db = Arc::clone(&self.db);
        let notifier = self.notifier.clone();

        let result = tokio::task::spawn_blocking(move || {
            let result = imap::perform_flag_action(&db, &message_ids, action);
            if !result.succeeded.is_empty() {
                notifier.notify_message_change();
            }
            result
        }).await.unwrap_or_else(|e| {
            error!("Task failed: {}", e);
            ActionResult::new()
        });

        result.to_json()
    }

    /// Move messages to trash
    ///
    /// # Arguments
    /// * `message_ids` - List of message IDs to move to trash
    ///
    /// # Returns
    /// * JSON string containing ActionResult with succeeded and failed lists
    #[zbus(name = "MoveToTrash")]
    async fn move_to_trash(&self, message_ids: Vec<String>) -> String {
        info!("D-Bus: MoveToTrash called for {} messages", message_ids.len());
        let db = Arc::clone(&self.db);
        let notifier = self.notifier.clone();

        let result = tokio::task::spawn_blocking(move || {
            let result = imap::move_to_trash(&db, &message_ids);
            if !result.succeeded.is_empty() {
                notifier.notify_message_change();
            }
            result
        }).await.unwrap_or_else(|e| {
            error!("Task failed: {}", e);
            ActionResult::new()
        });

        result.to_json()
    }

    // ========================================================================
    // Password/Secret Storage Methods
    // ========================================================================

    /// Read a password from the secure credential store
    ///
    /// # Arguments
    /// * `key` - The key identifying the credential
    ///
    /// # Returns
    /// * The password if found, empty string otherwise
    #[zbus(name = "ReadPassword")]
    async fn read_password(&self, key: &str) -> String {
        debug!("D-Bus: ReadPassword called for key: {}", key);
        secrets::read_password(key).unwrap_or_default()
    }

    /// Write a password to the secure credential store
    ///
    /// # Arguments
    /// * `key` - The key identifying the credential
    /// * `password` - The password to store
    ///
    /// # Returns
    /// * `true` if the password was stored successfully, `false` otherwise
    #[zbus(name = "WritePassword")]
    async fn write_password(&self, key: &str, password: &str) -> bool {
        debug!("D-Bus: WritePassword called for key: {}", key);
        secrets::write_password(key, password)
    }

    /// Delete a password from the secure credential store
    ///
    /// # Arguments
    /// * `key` - The key identifying the credential to delete
    ///
    /// # Returns
    /// * `true` if the password was deleted (or didn't exist), `false` on error
    #[zbus(name = "DeletePassword")]
    async fn delete_password(&self, key: &str) -> bool {
        debug!("D-Bus: DeletePassword called for key: {}", key);
        secrets::delete_password(key)
    }
}

/// D-Bus notifier for database changes
/// Uses a channel to send notifications from worker threads to the async D-Bus task
#[derive(Clone)]
pub struct DBusNotifier {
    sender: mpsc::UnboundedSender<NotificationEvent>,
}

impl DBusNotifier {
    /// Create a new D-Bus notifier
    /// Returns the notifier and a receiver for the notification channel
    pub fn new() -> (Self, mpsc::UnboundedReceiver<NotificationEvent>) {
        let (sender, receiver) = mpsc::unbounded_channel();
        (Self { sender }, receiver)
    }

    /// Notify that a table changed (non-blocking, thread-safe)
    pub fn notify_table_change(&self, table: TableChange) {
        if let Err(e) = self.sender.send(NotificationEvent::TableChanged(table)) {
            error!("Failed to send D-Bus notification: {}", e);
        }
    }

    /// Notify that the folder table changed
    pub fn notify_folder_change(&self) {
        self.notify_table_change(TableChange::Folder);
    }

    /// Notify that the label table changed
    ///
    /// FUTURE USE: For Gmail-style labels and IMAP keywords feature
    #[allow(dead_code)]
    pub fn notify_label_change(&self) {
        self.notify_table_change(TableChange::Label);
    }

    /// Notify that the message table changed
    pub fn notify_message_change(&self) {
        self.notify_table_change(TableChange::Message);
    }

    /// Notify that the account list changed
    pub fn notify_account_change(&self) {
        self.notify_table_change(TableChange::Account);
    }

    /// Notify that accounts should be reloaded from filesystem
    pub fn notify_account_reload(&self) {
        if let Err(e) = self.sender.send(NotificationEvent::AccountReload) {
            error!("Failed to send account reload notification: {}", e);
        }
    }

    /// Notify that a sync should be triggered for an account
    /// Pass empty string to trigger sync for all accounts
    pub fn notify_sync_trigger(&self, account_id: String) {
        if let Err(e) = self.sender.send(NotificationEvent::SyncTrigger(account_id)) {
            error!("Failed to send sync trigger notification: {}", e);
        }
    }
}

/// Error type for D-Bus initialization
#[derive(Debug)]
pub enum DbusInitError {
    /// Another instance of ravend is already running
    AnotherInstanceRunning,
    /// Other D-Bus error
    DbusError(zbus::Error),
}

impl std::fmt::Display for DbusInitError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DbusInitError::AnotherInstanceRunning => {
                write!(f, "Another instance of ravend is already running")
            }
            DbusInitError::DbusError(e) => write!(f, "D-Bus error: {}", e),
        }
    }
}

impl std::error::Error for DbusInitError {}

impl From<zbus::Error> for DbusInitError {
    fn from(e: zbus::Error) -> Self {
        DbusInitError::DbusError(e)
    }
}

/// Initialize D-Bus and run the notification loop
/// This should be spawned as an async task on the Tokio runtime
/// Returns a receiver that gets AccountReload events
///
/// # Arguments
/// * `init_tx` - Oneshot sender to signal initialization result (Ok or error)
/// * `sync_trigger_tx` - Channel to send sync trigger requests (account_id)
pub async fn run_dbus_service(
    mut receiver: mpsc::UnboundedReceiver<NotificationEvent>,
    db: Arc<Mutex<Database>>,
    notifier: DBusNotifier,
    reload_tx: mpsc::UnboundedSender<()>,
    sync_trigger_tx: mpsc::UnboundedSender<String>,
    init_tx: oneshot::Sender<std::result::Result<(), DbusInitError>>,
) -> Result<()> {
    info!("Initializing D-Bus interface...");

    // Connect to session bus
    let conn = match Connection::session().await {
        Ok(c) => c,
        Err(e) => {
            let _ = init_tx.send(Err(DbusInitError::DbusError(e)));
            return Err(zbus::Error::Failure("Failed to connect to session bus".into()));
        }
    };

    // Request the service name with DoNotQueue flag
    // This will fail immediately if another instance already owns the name
    let reply = match conn
        .request_name_with_flags(DBUS_SERVICE, RequestNameFlags::DoNotQueue.into())
        .await
    {
        Ok(reply) => reply,
        Err(e) => {
            // Check if the error indicates the name is already taken
            let error_str = e.to_string();
            if error_str.contains("already taken") || error_str.contains("NameAlreadyOwner") {
                error!("Another instance of ravend is already running (D-Bus name {} is taken)", DBUS_SERVICE);
                let _ = init_tx.send(Err(DbusInitError::AnotherInstanceRunning));
                return Err(zbus::Error::Failure("Another instance is already running".into()));
            }
            let _ = init_tx.send(Err(DbusInitError::DbusError(e)));
            return Err(zbus::Error::Failure("Failed to request D-Bus name".into()));
        }
    };

    // Check if we actually got the name
    match reply {
        RequestNameReply::PrimaryOwner => {
            // We got the name, continue
        }
        RequestNameReply::Exists | RequestNameReply::AlreadyOwner | RequestNameReply::InQueue => {
            // Another instance is running (or we're already running, which shouldn't happen)
            error!("Another instance of ravend is already running (D-Bus name {} is taken)", DBUS_SERVICE);
            let _ = init_tx.send(Err(DbusInitError::AnotherInstanceRunning));
            return Err(zbus::Error::Failure("Another instance is already running".into()));
        }
    }

    // Create and register the interface with database access
    let dbus_iface = RavenDaemonDBus::new(db, notifier);
    conn.object_server().at(DBUS_PATH, dbus_iface).await?;

    info!("D-Bus interface registered at {}", DBUS_PATH);
    info!("D-Bus service name: {}", DBUS_SERVICE);

    // Signal successful initialization to main
    let _ = init_tx.send(Ok(()));

    // Get interface reference for signal emission
    let object_server = conn.object_server();
    let iface_ref = object_server
        .interface::<_, RavenDaemonDBus>(DBUS_PATH)
        .await?;

    // Process notifications from the channel using async recv
    loop {
        match receiver.recv().await {
            Some(event) => match event {
                NotificationEvent::TableChanged(table) => {
                    let signal_ctxt = iface_ref.signal_context();
                    match RavenDaemonDBus::table_changed(signal_ctxt, table.as_str()).await {
                        Ok(_) => {
                            debug!("Emitted D-Bus signal for table: {}", table.as_str());
                        }
                        Err(e) => {
                            error!(
                                "Failed to emit D-Bus signal for table {}: {}",
                                table.as_str(),
                                e
                            );
                        }
                    }
                }
                NotificationEvent::AccountReload => {
                    debug!("Received account reload request");
                    if let Err(e) = reload_tx.send(()) {
                        error!("Failed to send account reload signal to main loop: {}", e);
                    }
                }
                NotificationEvent::SyncTrigger(account_id) => {
                    debug!("Received sync trigger request for account: {}", if account_id.is_empty() { "all" } else { &account_id });
                    if let Err(e) = sync_trigger_tx.send(account_id) {
                        error!("Failed to send sync trigger signal to main loop: {}", e);
                    }
                }
            },
            None => {
                info!("D-Bus notification channel closed, shutting down");
                break;
            }
        }
    }

    Ok(())
}
