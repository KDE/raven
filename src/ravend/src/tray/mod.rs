// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! System tray integration using ksni (StatusNotifierItem D-Bus protocol)
//!
//! This module provides a system tray icon for the Raven daemon with:
//! - Left click: Opens/raises the Raven client app
//! - Right click: Shows a menu with Quit option

use ksni::{menu::StandardItem, MenuItem, Tray, TrayService};
use log::{error, info, warn};
use std::process::Command;
use tokio::sync::mpsc;

/// The tray icon implementation
struct RavenTray {
    quit_tx: mpsc::UnboundedSender<()>,
}

impl Tray for RavenTray {
    fn icon_name(&self) -> String {
        "org.kde.raven".to_string()
    }

    fn id(&self) -> String {
        "ravend".to_string()
    }

    fn title(&self) -> String {
        "Raven Mail".to_string()
    }

    fn tool_tip(&self) -> ksni::ToolTip {
        ksni::ToolTip {
            icon_name: "org.kde.raven".to_string(),
            icon_pixmap: vec![],
            title: "Raven Mail".to_string(),
            description: "Email sync service".to_string(),
        }
    }

    fn category(&self) -> ksni::Category {
        ksni::Category::Communications
    }

    fn menu(&self) -> Vec<MenuItem<Self>> {
        vec![MenuItem::Standard(StandardItem {
            label: "Quit".to_string(),
            activate: Box::new(|tray: &mut Self| {
                let _ = tray.quit_tx.send(());
            }),
            ..Default::default()
        })]
    }

    fn activate(&mut self, _x: i32, _y: i32) {
        // Open the Raven client application on left click
        open_raven_client();
    }
}

/// Open the Raven client application
fn open_raven_client() {
    // Try to launch the raven executable
    // On KDE, the app should be registered via its .desktop file
    match Command::new("raven").spawn() {
        Ok(_) => {
            info!("Launched Raven client");
        }
        Err(e) => {
            error!("Failed to launch Raven client: {}", e);
        }
    }
}

/// Check if StatusNotifierWatcher is available on D-Bus
/// Uses dbus-send command to avoid blocking runtime issues with zbus
fn is_status_notifier_watcher_available() -> bool {
    let output = Command::new("dbus-send")
        .args([
            "--session",
            "--dest=org.freedesktop.DBus",
            "--type=method_call",
            "--print-reply",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus.NameHasOwner",
            "string:org.kde.StatusNotifierWatcher",
        ])
        .output();

    match output {
        Ok(output) => {
            // Check if the response contains "boolean true"
            let stdout = String::from_utf8_lossy(&output.stdout);
            stdout.contains("boolean true")
        }
        Err(e) => {
            warn!("Failed to check StatusNotifierWatcher availability: {}", e);
            false
        }
    }
}

/// Initialize the system tray icon
/// Returns a receiver that signals when quit is requested
/// Returns None if the tray could not be initialized (e.g., no StatusNotifierWatcher)
pub fn init() -> Option<mpsc::UnboundedReceiver<()>> {
    // Check if StatusNotifierWatcher is available before trying to use ksni
    if !is_status_notifier_watcher_available() {
        warn!("StatusNotifierWatcher not available, skipping system tray initialization");
        warn!("On GNOME, install the AppIndicator extension for tray icon support");
        return None;
    }

    let (quit_tx, quit_rx) = mpsc::unbounded_channel();

    let tray = RavenTray { quit_tx };

    // spawn() starts the tray service in its own thread
    TrayService::new(tray).spawn();

    info!("System tray icon initialized");
    Some(quit_rx)
}
