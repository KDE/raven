// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Raven mail daemon - background email sync service
//!
//! This daemon handles:
//! - IMAP email synchronization
//! - SQLite database management
//! - Periodic polling

mod config;
mod db;
mod dbus;
mod imap;
mod kwallet;
mod models;
mod notifications;
mod oauth2;
mod portal;
mod tray;

use anyhow::Result;
use dbus::{DBusNotifier, DbusInitError};
use log::{debug, error, info};
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::process::ExitCode;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use config::AccountManager;
use db::Database;
use imap::ImapWorker;

/// Polling interval for email sync when IDLE is not supported (in seconds)
const POLL_INTERVAL_SECS: u64 = 300; // 5 minutes fallback

/// Nice value for worker threads (0-19, higher = lower priority)
/// 10 is a good balance - low priority but still responsive
const WORKER_NICE_VALUE: i32 = 10;

/// Worker thread handle with shutdown and sync trigger channels
struct WorkerHandle {
    #[allow(dead_code)]
    account_id: String,
    email: String,
    #[allow(dead_code)]
    handle: thread::JoinHandle<()>,
    shutdown_tx: std::sync::mpsc::Sender<()>,
    sync_trigger_tx: std::sync::mpsc::Sender<()>,
}

#[tokio::main(flavor = "multi_thread", worker_threads = 2)]
async fn main() -> ExitCode {
    // Initialize logging first so errors are properly logged
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    if let Err(e) = run_daemon().await {
        error!("{}", e);
        return ExitCode::FAILURE;
    }
    ExitCode::SUCCESS
}

async fn run_daemon() -> Result<()> {
    info!("Starting ravend v{}", env!("CARGO_PKG_VERSION"));

    // Initialize data directories
    let data_dir = get_data_dir()?;
    let config_dir = get_config_dir()?;
    let files_dir = data_dir.join("files");

    std::fs::create_dir_all(&data_dir)?;
    std::fs::create_dir_all(&config_dir)?;
    std::fs::create_dir_all(&files_dir)?;

    info!("Data directory: {:?}", data_dir);
    info!("Config directory: {:?}", config_dir);

    // Initialize database
    let db_path = data_dir.join("raven.sqlite");
    let db = Arc::new(Mutex::new(Database::new(&db_path)?));

    // Run migrations
    {
        let mut db_lock = db.lock().unwrap();
        db_lock.migrate()?;
    }
    info!("Database initialized and migrated");

    // Initialize notification system
    notifications::init();

    // Initialize D-Bus notifier (creates channel)
    let (dbus_notifier, dbus_receiver) = DBusNotifier::new();

    // Create account reload channel
    let (reload_tx, mut reload_rx) = tokio::sync::mpsc::unbounded_channel();

    // Create sync trigger channel
    let (sync_trigger_tx, mut sync_trigger_rx) = tokio::sync::mpsc::unbounded_channel::<String>();

    // Create D-Bus initialization channel
    let (dbus_init_tx, dbus_init_rx) = tokio::sync::oneshot::channel();

    // Spawn D-Bus service task with database and config access
    let db_for_dbus = Arc::clone(&db);
    let config_dir_for_dbus = config_dir.clone();
    let notifier_for_dbus = dbus_notifier.clone();
    tokio::spawn(async move {
        if let Err(e) = dbus::run_dbus_service(
            dbus_receiver,
            db_for_dbus,
            config_dir_for_dbus,
            notifier_for_dbus,
            reload_tx,
            sync_trigger_tx,
            dbus_init_tx,
        )
        .await
        {
            error!("D-Bus service failed: {}", e);
        }
    });

    // Wait for D-Bus initialization result
    match dbus_init_rx.await {
        Ok(Ok(())) => {
            // D-Bus initialized successfully
        }
        Ok(Err(DbusInitError::AnotherInstanceRunning)) => {
            // Exit cleanly - another instance is already running
            return Err(anyhow::anyhow!("Another instance of ravend is already running. Exiting."));
        }
        Ok(Err(DbusInitError::DbusError(e))) => {
            return Err(anyhow::anyhow!("Failed to initialize D-Bus: {}", e));
        }
        Err(_) => {
            return Err(anyhow::anyhow!("D-Bus initialization channel closed unexpectedly"));
        }
    }

    // Request background/autostart permission via XDG portal
    // This is especially important for Flatpak environments
    if portal::is_sandboxed() {
        info!("Running in sandboxed environment, requesting background permission...");
    }
    match portal::request_background_permission().await {
        Some(result) => {
            if result.response == portal::PortalResponse::Success {
                info!(
                    "Background portal: background={}, autostart={}",
                    result.background_granted, result.autostart_granted
                );
            } else {
                info!("Background permission not granted, continuing anyway");
            }
        }
        None => {
            // Portal not available or request failed - this is fine on non-portal systems
            debug!("Background portal not available or request failed");
        }
    }

    // Initialize system tray icon
    let tray_manager = tray::TrayManager::new();
    if tray_manager.is_none() {
        info!("System tray not available, continuing without tray icon");
    }

    // Initialize account manager
    let account_manager = AccountManager::new(&config_dir)?;

    // Track active workers
    let mut workers: HashMap<String, WorkerHandle> = HashMap::new();

    // Helper function to reload accounts and manage workers
    let reload_accounts = |workers: &mut HashMap<String, WorkerHandle>| -> Result<()> {
        match account_manager.load_accounts() {
            Ok(accounts) => {
                let current_account_ids: HashSet<String> =
                    accounts.iter().map(|a| a.id.clone()).collect();

                // Find new accounts that don't have workers
                let existing_account_ids: HashSet<String> =
                    workers.keys().cloned().collect();

                let new_account_ids: Vec<String> = current_account_ids
                    .difference(&existing_account_ids)
                    .cloned()
                    .collect();

                // Start workers for new accounts
                for account_id in &new_account_ids {
                    if let Some(account) = accounts.iter().find(|a| &a.id == account_id) {
                        info!("Detected new account: {} ({})", account.email, account.id);
                        info!("Starting sync worker for: {}", account.email);

                        let db_clone = Arc::clone(&db);
                        let files_dir_clone = files_dir.clone();
                        let dbus_clone = dbus_notifier.clone();
                        let account_clone = account.clone();

                        // Create shutdown channel
                        let (shutdown_tx, shutdown_rx) = std::sync::mpsc::channel();
                        // Create sync trigger channel
                        let (sync_trigger_tx, sync_trigger_rx) = std::sync::mpsc::channel();

                        let handle = thread::spawn(move || {
                            // Lower thread priority to reduce power consumption
                            // This makes the sync worker less aggressive about CPU time
                            unsafe {
                                let result = libc::nice(WORKER_NICE_VALUE);
                                if result == -1 {
                                    // nice() can legitimately return -1, check errno
                                    let errno = *libc::__errno_location();
                                    if errno != 0 {
                                        debug!("Failed to set nice value: errno {}", errno);
                                    }
                                } else {
                                    debug!("Set worker thread nice value to {}", WORKER_NICE_VALUE);
                                }
                            }

                            let mut worker = ImapWorker::new(
                                account_clone,
                                db_clone,
                                files_dir_clone,
                                dbus_clone
                            );

                            loop {
                                // Check for shutdown signal (non-blocking)
                                if shutdown_rx.try_recv().is_ok() {
                                    info!("Worker shutting down for account: {}", worker.account_email());
                                    break;
                                }

                                // Check for sync trigger (non-blocking)
                                if sync_trigger_rx.try_recv().is_ok() {
                                    info!("Manual sync triggered for account: {}", worker.account_email());
                                    // Continue to sync immediately
                                }

                                // Perform sync with IDLE support
                                // This will use IMAP IDLE if supported, falling back to polling
                                match worker.sync_with_idle(&shutdown_rx, &sync_trigger_rx, POLL_INTERVAL_SECS) {
                                    Ok(_) => {
                                        debug!("Sync cycle completed for account: {}", worker.account_email());
                                    }
                                    Err(e) => {
                                        error!("Sync failed for account {}: {}", worker.account_email(), e);
                                        // On error, wait before retrying to avoid spinning
                                        for _ in 0..60 {
                                            if shutdown_rx.try_recv().is_ok() {
                                                info!("Worker shutting down for account: {}", worker.account_email());
                                                return;
                                            }
                                            thread::sleep(Duration::from_secs(1));
                                        }
                                    }
                                }
                            }
                        });

                        workers.insert(
                            account.id.clone(),
                            WorkerHandle {
                                account_id: account.id.clone(),
                                email: account.email.clone(),
                                handle,
                                shutdown_tx,
                                sync_trigger_tx,
                            }
                        );
                    }
                }

                // Find deleted accounts and shut down their workers
                let deleted_account_ids: Vec<String> = existing_account_ids
                    .difference(&current_account_ids)
                    .cloned()
                    .collect();

                for account_id in &deleted_account_ids {
                    if let Some(worker) = workers.remove(account_id) {
                        info!("Account removed: {} ({})", worker.email, account_id);
                        info!("Shutting down sync worker for: {}", worker.email);

                        // Send shutdown signal
                        let _ = worker.shutdown_tx.send(());

                        // Note: We don't wait for the thread to finish here
                        // The worker will shut down within SYNC_INTERVAL_SECS at most
                    }
                }

                // Log current status
                if workers.is_empty() {
                    info!("No accounts configured. Waiting for accounts to be added...");
                } else if !new_account_ids.is_empty() || !deleted_account_ids.is_empty() {
                    // Only log when there are changes
                    info!("Currently managing {} account(s)", workers.len());
                }

                Ok(())
            }
            Err(e) => {
                error!("Failed to load accounts: {}", e);
                Err(e)
            }
        }
    };

    // Load initial accounts
    info!("Loading initial accounts...");
    let _ = reload_accounts(&mut workers);

    // Create interval for tray event processing
    let mut tray_interval = tokio::time::interval(Duration::from_millis(100));

    // Main account management loop - wait for reload and sync trigger signals
    info!("Waiting for account reload signals...");
    loop {
        tokio::select! {
            reload_msg = reload_rx.recv() => {
                match reload_msg {
                    Some(_) => {
                        info!("Received account reload signal");
                        let _ = reload_accounts(&mut workers);
                    }
                    None => {
                        info!("Account reload channel closed, shutting down");
                        break;
                    }
                }
            }
            sync_msg = sync_trigger_rx.recv() => {
                match sync_msg {
                    Some(account_id) => {
                        if account_id.is_empty() {
                            // Trigger sync for all accounts
                            info!("Triggering sync for all accounts");
                            for (_, worker) in workers.iter() {
                                if let Err(e) = worker.sync_trigger_tx.send(()) {
                                    error!("Failed to send sync trigger to {}: {}", worker.email, e);
                                }
                            }
                        } else {
                            // Trigger sync for specific account
                            if let Some(worker) = workers.get(&account_id) {
                                info!("Triggering sync for account: {}", worker.email);
                                if let Err(e) = worker.sync_trigger_tx.send(()) {
                                    error!("Failed to send sync trigger to {}: {}", worker.email, e);
                                }
                            } else {
                                error!("Account not found for sync trigger: {}", account_id);
                            }
                        }
                    }
                    None => {
                        info!("Sync trigger channel closed");
                    }
                }
            }
            _ = tray_interval.tick() => {
                // Process Qt events for tray icon
                if let Some(ref tray) = tray_manager {
                    tray.process_events();

                    // Check if quit was requested
                    if tray.is_quit_requested() {
                        info!("Quit requested via system tray, shutting down...");
                        break;
                    }
                }
            }
        }
    }

    // Clean shutdown - send shutdown signal to all workers
    info!("Shutting down {} worker(s)...", workers.len());
    for (_, worker) in workers.iter() {
        let _ = worker.shutdown_tx.send(());
    }

    Ok(())
}

/// Get the data directory path (matches Qt's GenericDataLocation + "/raven")
fn get_data_dir() -> Result<PathBuf> {
    let base = dirs::data_dir().ok_or_else(|| anyhow::anyhow!("Could not find data directory"))?;
    Ok(base.join("raven"))
}

/// Get the config directory path (matches Qt's ConfigLocation + "/raven")
fn get_config_dir() -> Result<PathBuf> {
    let base =
        dirs::config_dir().ok_or_else(|| anyhow::anyhow!("Could not find config directory"))?;
    Ok(base.join("raven"))
}
