// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use ksni::{menu::StandardItem, MenuItem, Tray, TrayService};
use std::process::Command;
use tokio::sync::mpsc;

struct RavenTray {
    quit_tx: mpsc::UnboundedSender<()>,
}

impl Tray for RavenTray {
    fn icon_name(&self) -> String { "org.kde.raven".into() }
    fn id(&self) -> String { "ravend".into() }
    fn title(&self) -> String { "Raven Mail".into() }
    fn category(&self) -> ksni::Category { ksni::Category::Communications }

    fn tool_tip(&self) -> ksni::ToolTip {
        ksni::ToolTip {
            icon_name: "org.kde.raven".into(),
            icon_pixmap: vec![],
            title: "Raven Mail".into(),
            description: "Email sync service".into(),
        }
    }

    fn menu(&self) -> Vec<MenuItem<Self>> {
        vec![MenuItem::Standard(StandardItem {
            label: "Quit".into(),
            activate: Box::new(|tray: &mut Self| { let _ = tray.quit_tx.send(()); }),
            ..Default::default()
        })]
    }

    fn activate(&mut self, _x: i32, _y: i32) {
        // Launch raven
        let _ = Command::new("raven").spawn();
    }
}

fn status_notifier_available() -> bool {
    Command::new("dbus-send")
        .args([
            "--session",
            "--dest=org.freedesktop.DBus",
            "--type=method_call",
            "--print-reply",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus.NameHasOwner",
            "string:org.kde.StatusNotifierWatcher",
        ])
        .output()
        .map(|o| String::from_utf8_lossy(&o.stdout).contains("boolean true"))
        .unwrap_or(false)
}

pub fn init() -> Option<mpsc::UnboundedReceiver<()>> {
    if !status_notifier_available() { return None; }

    let (quit_tx, quit_rx) = mpsc::unbounded_channel();
    TrayService::new(RavenTray { quit_tx }).spawn();
    Some(quit_rx)
}
