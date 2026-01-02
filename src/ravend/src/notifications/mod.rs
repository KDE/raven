// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Desktop notifications via D-Bus (org.freedesktop.Notifications)

use log::info;
use std::collections::HashMap;
use zbus::blocking::Connection;

const BATCH_THRESHOLD: usize = 5;

/// Initialize the notification system
pub fn init() {
    info!("Notification system initialized");
}

fn send_notification(title: &str, body: &str) {
    let conn = match Connection::session() {
        Ok(c) => c,
        Err(e) => {
            log::warn!("Failed to connect to session bus: {}", e);
            return;
        }
    };

    let hints: HashMap<&str, zbus::zvariant::Value> = HashMap::from([
        ("desktop-entry", zbus::zvariant::Value::from("org.kde.raven")),
        ("urgency", zbus::zvariant::Value::from(1u8)), // Normal urgency
        ("category", zbus::zvariant::Value::from("email.arrived")),
    ]);

    // expire_timeout: 0 means use server default, -1 means never expire
    let result: Result<u32, _> = conn.call_method(
        Some("org.freedesktop.Notifications"),
        "/org/freedesktop/Notifications",
        Some("org.freedesktop.Notifications"),
        "Notify",
        &("Raven Mail", 0u32, "org.kde.raven", title, body, Vec::<&str>::new(), hints, 0i32),
    ).and_then(|reply| reply.body().deserialize());

    match result {
        Ok(id) => info!("Notification sent (ID: {}): {}", id, title),
        Err(e) => log::warn!("Failed to send notification: {}", e),
    }
}

#[derive(Clone, Debug)]
pub struct NewEmailInfo {
    pub sender: String,
    pub subject: String,
    pub preview: String,
}

pub struct NotificationBatch {
    account_email: String,
    emails: Vec<NewEmailInfo>,
}

impl NotificationBatch {
    pub fn new(account_email: &str) -> Self {
        Self {
            account_email: account_email.to_string(),
            emails: Vec::new(),
        }
    }

    pub fn add_email(&mut self, info: NewEmailInfo) {
        self.emails.push(info);
    }

    pub fn send_notifications(&self) {
        if self.emails.is_empty() {
            return;
        }

        info!("NotificationBatch: {} new emails for {}", self.emails.len(), self.account_email);

        if self.emails.len() >= BATCH_THRESHOLD {
            let title = format!("{} new emails", self.emails.len());
            let body = format!("New emails for {}", self.account_email);
            send_notification(&title, &body);
        } else {
            for email in &self.emails {
                let title = email.sender.clone();
                let body = format!("{}\n{}", email.subject, email.preview);
                send_notification(&title, &body);
            }
        }
    }
}
