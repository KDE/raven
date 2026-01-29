// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Thread {
    pub id: String,
    pub account_id: String,
    pub subject: String,
    pub snippet: String,
    pub unread_count: i32,
    pub starred_count: i32,
    pub first_message_timestamp: DateTime<Utc>,
    pub last_message_timestamp: DateTime<Utc>,
    pub data: Option<String>,
}

impl Thread {
    pub fn new(id: String, account_id: String, subject: String) -> Self {
        let now = Utc::now();
        Self {
            id,
            account_id,
            subject,
            snippet: String::new(),
            unread_count: 0,
            starred_count: 0,
            first_message_timestamp: now,
            last_message_timestamp: now,
            data: None,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThreadReference {
    pub thread_id: String,
    pub account_id: String,
    pub header_message_id: String,
}

impl ThreadReference {
    pub fn new(thread_id: String, account_id: String, header_message_id: String) -> Self {
        Self { thread_id, account_id, header_message_id }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThreadFolder {
    pub id: i64,
    pub account_id: String,
    pub thread_id: String,
    pub folder_id: String,
}

impl ThreadFolder {
    pub fn new(account_id: String, thread_id: String, folder_id: String) -> Self {
        Self { id: 0, account_id, thread_id, folder_id }
    }
}
