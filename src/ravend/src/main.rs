// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Raven mail daemon - background email sync service

mod config;
mod constants;
mod db;
mod dbus;
mod imap;
mod models;
mod notifications;
mod oauth2;
mod portal;
mod secrets;
mod tray;

use anyhow::Result;
use config::AccountManager;
use constants::{POLL_INTERVAL_SECS, WORKER_NICE_VALUE};
use db::Database;
use dbus::{DBusNotifier, DbusInitError};
use imap::ImapWorker;
use log::{debug, error, info};
use models::Account;
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::process::ExitCode;
use std::sync::mpsc::{self, Sender};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

struct WorkerHandle {
    email: String,
    shutdown_tx: Sender<()>,
    sync_trigger_tx: Sender<()>,
}

#[tokio::main(flavor = "multi_thread", worker_threads = 2)]
async fn main() -> ExitCode {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    if let Err(e) = run_daemon().await {
        error!("{}", e);
        return ExitCode::FAILURE;
    }
    ExitCode::SUCCESS
}

async fn run_daemon() -> Result<()> {
    info!("Starting ravend v{}", env!("CARGO_PKG_VERSION"));

    let data_dir = get_data_dir()?;
    let config_dir = get_config_dir()?;
    let files_dir = data_dir.join("files");

    std::fs::create_dir_all(&data_dir)?;
    std::fs::create_dir_all(&config_dir)?;
    std::fs::create_dir_all(&files_dir)?;

    info!("Data directory: {:?}", data_dir);
    info!("Config directory: {:?}", config_dir);

    let db = Arc::new(Mutex::new(Database::new(&data_dir.join("raven.sqlite"))?));
    db.lock().unwrap().migrate()?;
    info!("Database initialized");

    let (dbus_notifier, dbus_receiver) = DBusNotifier::new();
    let (reload_tx, mut reload_rx) = tokio::sync::mpsc::unbounded_channel();
    let (sync_trigger_tx, mut sync_trigger_rx) = tokio::sync::mpsc::unbounded_channel::<String>();

    init_dbus_service(
        dbus_receiver,
        Arc::clone(&db),
        dbus_notifier.clone(),
        reload_tx,
        sync_trigger_tx,
    )
    .await?;

    request_background_permission().await;

    // Initialize tray (may fail if StatusNotifierWatcher is not available)
    let mut tray_quit_rx = tray::init();

    // Initialize global account manager
    AccountManager::init(&config_dir)?;
    let mut workers: HashMap<String, WorkerHandle> = HashMap::new();

    info!("Loading initial accounts...");
    reload_accounts(
        &mut workers,
        &db,
        &files_dir,
        &dbus_notifier,
    );

    info!("Entering main loop...");
    loop {
        tokio::select! {
            Some(_) = reload_rx.recv() => {
                info!("Received account reload signal");
                reload_accounts(&mut workers, &db, &files_dir, &dbus_notifier);
            }
            Some(account_id) = sync_trigger_rx.recv() => {
                trigger_sync(&workers, &account_id);
            }
            Some(_) = async {
                match &mut tray_quit_rx {
                    Some(rx) => rx.recv().await,
                    None => std::future::pending().await,
                }
            } => {
                info!("Quit requested via system tray");
                break;
            }
        }
    }

    info!("Shutting down {} worker(s)...", workers.len());
    for worker in workers.values() {
        let _ = worker.shutdown_tx.send(());
    }

    Ok(())
}

async fn init_dbus_service(
    dbus_receiver: tokio::sync::mpsc::UnboundedReceiver<dbus::NotificationEvent>,
    db: Arc<Mutex<Database>>,
    notifier: DBusNotifier,
    reload_tx: tokio::sync::mpsc::UnboundedSender<()>,
    sync_trigger_tx: tokio::sync::mpsc::UnboundedSender<String>,
) -> Result<()> {
    let (dbus_init_tx, dbus_init_rx) = tokio::sync::oneshot::channel();

    tokio::spawn(async move {
        if let Err(e) = dbus::run_dbus_service(
            dbus_receiver,
            db,
            notifier,
            reload_tx,
            sync_trigger_tx,
            dbus_init_tx,
        )
        .await
        {
            error!("D-Bus service failed: {}", e);
        }
    });

    match dbus_init_rx.await {
        Ok(Ok(())) => Ok(()),
        Ok(Err(DbusInitError::AnotherInstanceRunning)) => {
            Err(anyhow::anyhow!("Another instance of ravend is already running"))
        }
        Ok(Err(DbusInitError::DbusError(e))) => {
            Err(anyhow::anyhow!("Failed to initialize D-Bus: {}", e))
        }
        Err(_) => Err(anyhow::anyhow!("D-Bus initialization channel closed")),
    }
}

async fn request_background_permission() {
    if let Some((bg, auto)) = portal::request_background_permission().await {
        info!("Background portal: background={bg}, autostart={auto}");
    }
}

fn reload_accounts(
    workers: &mut HashMap<String, WorkerHandle>,
    db: &Arc<Mutex<Database>>,
    files_dir: &PathBuf,
    dbus_notifier: &DBusNotifier,
) {
    // Atomically reload accounts from disk
    let accounts = match AccountManager::global().reload() {
        Ok(accounts) => accounts,
        Err(e) => {
            error!("Failed to load accounts: {}", e);
            return;
        }
    };

    let current_ids: HashSet<_> = accounts.iter().map(|a| a.id.clone()).collect();
    let existing_ids: HashSet<_> = workers.keys().cloned().collect();

    // Start workers for new accounts
    for account in &accounts {
        if !existing_ids.contains(&account.id) {
            info!("Starting sync worker for: {}", account.email);
            if let Some(handle) = spawn_worker(account.clone(), db, files_dir, dbus_notifier) {
                workers.insert(account.id.clone(), handle);
            }
        }
    }

    // Stop workers for removed accounts
    for account_id in existing_ids.difference(&current_ids) {
        if let Some(worker) = workers.remove(account_id) {
            info!("Stopping sync worker for: {}", worker.email);
            let _ = worker.shutdown_tx.send(());
        }
    }

    if workers.is_empty() {
        info!("No accounts configured");
    }
}

fn spawn_worker(
    account: Account,
    db: &Arc<Mutex<Database>>,
    files_dir: &PathBuf,
    dbus_notifier: &DBusNotifier,
) -> Option<WorkerHandle> {
    let db = Arc::clone(db);
    let files_dir = files_dir.clone();
    let dbus_notifier = dbus_notifier.clone();
    let email = account.email.clone();
    let account_email = account.email.clone(); // For panic hook

    let (shutdown_tx, shutdown_rx) = mpsc::channel();
    let (sync_trigger_tx, sync_trigger_rx) = mpsc::channel();

    thread::spawn(move || {
        // Set up panic hook to log panics before thread dies
        let default_panic = std::panic::take_hook();
        std::panic::set_hook(Box::new(move |panic_info| {
            error!("WORKER THREAD PANIC [{}]: {}", account_email, panic_info);
            if let Some(location) = panic_info.location() {
                error!("  at {}:{}:{}", location.file(), location.line(), location.column());
            }
            // Call the default panic handler
            default_panic(panic_info);
        }));

        set_thread_priority();

        let mut worker = ImapWorker::new(account, db, files_dir, dbus_notifier);

        loop {
            if shutdown_rx.try_recv().is_ok() {
                info!("Worker shutting down for: {}", worker.account_email());
                break;
            }

            if sync_trigger_rx.try_recv().is_ok() {
                info!("Manual sync triggered for: {}", worker.account_email());
            }

            match worker.sync_with_idle(&shutdown_rx, &sync_trigger_rx, POLL_INTERVAL_SECS) {
                Ok(_) => debug!("Sync cycle completed for: {}", worker.account_email()),
                Err(e) => {
                    error!("Sync failed for {}: {}", worker.account_email(), e);
                    // Wait before retrying, checking for shutdown
                    for _ in 0..60 {
                        if shutdown_rx.try_recv().is_ok() {
                            return;
                        }
                        thread::sleep(Duration::from_secs(1));
                    }
                }
            }
        }
    });

    Some(WorkerHandle {
        email,
        shutdown_tx,
        sync_trigger_tx,
    })
}

fn set_thread_priority() {
    unsafe {
        let result = libc::nice(WORKER_NICE_VALUE);
        if result == -1 && *libc::__errno_location() != 0 {
            debug!("Failed to set nice value");
        }
    }
}

fn trigger_sync(workers: &HashMap<String, WorkerHandle>, account_id: &str) {
    if account_id.is_empty() {
        info!("Triggering sync for all accounts");
        for worker in workers.values() {
            let _ = worker.sync_trigger_tx.send(());
        }
    } else if let Some(worker) = workers.get(account_id) {
        info!("Triggering sync for: {}", worker.email);
        let _ = worker.sync_trigger_tx.send(());
    } else {
        error!("Account not found: {}", account_id);
    }
}

fn get_data_dir() -> Result<PathBuf> {
    dirs::data_dir()
        .map(|p| p.join("raven"))
        .ok_or_else(|| anyhow::anyhow!("Could not find data directory"))
}

fn get_config_dir() -> Result<PathBuf> {
    dirs::config_dir()
        .map(|p| p.join("raven"))
        .ok_or_else(|| anyhow::anyhow!("Could not find config directory"))
}
