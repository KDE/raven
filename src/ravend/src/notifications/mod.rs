// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Cross-platform desktop notifications using notify-rust

use log::info;
use notify_rust::{Notification, Urgency};

const BATCH_THRESHOLD: usize = 5;

/// Initialize the notification system
pub fn init() {
    info!("Notification system initialized");
}

fn send_notification(title: &str, body: &str) {
    let result = Notification::new()
        .appname("Raven Mail")
        .summary(title)
        .body(body)
        .icon("org.kde.raven")
        .urgency(Urgency::Normal)
        .show();

    match result {
        Ok(handle) => info!("Notification sent (ID: {}): {}", handle.id(), title),
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
