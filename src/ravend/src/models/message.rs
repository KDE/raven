// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Contact {
    pub email: String,
    pub name: Option<String>,
}

/// Structured data stored in message.data JSON field (for Qt frontend compatibility)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MessageData {
    pub from: Contact,
    pub to: Vec<Contact>,
    pub cc: Vec<Contact>,
    pub bcc: Vec<Contact>,
    pub reply_to: Vec<Contact>,
    pub synced_at: i64,
    pub labels: Vec<String>,
    pub snippet: String,
    pub plaintext: bool,
}

impl MessageData {
    pub fn to_json_string(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(self)
    }

    pub fn from_json_string(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    pub id: String,
    pub account_id: String,
    pub folder_id: String,
    pub thread_id: Option<String>,
    pub header_message_id: Option<String>,
    pub remote_uid: u32,
    pub subject: String,
    pub date: DateTime<Utc>,
    pub draft: bool,
    pub unread: bool,
    pub starred: bool,
    pub from_contacts: String,
    pub to_contacts: String,
    pub cc_contacts: String,
    pub bcc_contacts: String,
    pub reply_to_contacts: String,
    pub snippet: String,
    pub is_plaintext: bool,
}

impl Message {
    pub fn new(id: String, account_id: String, folder_id: String, remote_uid: u32) -> Self {
        Self {
            id,
            account_id,
            folder_id,
            thread_id: None,
            header_message_id: None,
            remote_uid,
            subject: String::new(),
            date: Utc::now(),
            draft: false,
            unread: true,
            starred: false,
            from_contacts: "[]".to_string(),
            to_contacts: "[]".to_string(),
            cc_contacts: "[]".to_string(),
            bcc_contacts: "[]".to_string(),
            reply_to_contacts: "[]".to_string(),
            snippet: String::new(),
            is_plaintext: false,
        }
    }

    pub fn get_from_contact(&self) -> Contact {
        serde_json::from_str(&self.from_contacts)
            .or_else(|_| {
                serde_json::from_str::<Vec<Contact>>(&self.from_contacts)
                    .map(|mut v| if !v.is_empty() { v.remove(0) } else { Contact { email: String::new(), name: None } })
            })
            .unwrap_or(Contact { email: String::new(), name: None })
    }

    pub fn get_data(&self) -> String {
        let to: Vec<Contact> = serde_json::from_str(&self.to_contacts).unwrap_or_default();
        let cc: Vec<Contact> = serde_json::from_str(&self.cc_contacts).unwrap_or_default();
        let bcc: Vec<Contact> = serde_json::from_str(&self.bcc_contacts).unwrap_or_default();
        let reply_to: Vec<Contact> = serde_json::from_str(&self.reply_to_contacts).unwrap_or_default();

        let message_data = MessageData {
            from: self.get_from_contact(),
            to,
            cc,
            bcc,
            reply_to,
            synced_at: Utc::now().timestamp(),
            labels: Vec::new(),
            snippet: self.snippet.clone(),
            plaintext: self.is_plaintext,
        };

        message_data.to_json_string().unwrap_or_else(|_| "{}".to_string())
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MessageBody {
    pub id: String,
    pub value: String,
    pub fetched_at: DateTime<Utc>,
}

impl MessageBody {
    pub fn new(id: String, value: String) -> Self {
        Self { id, value, fetched_at: Utc::now() }
    }
}
