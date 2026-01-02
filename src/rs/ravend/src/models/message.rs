// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Message model representing an email message

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

/// Contact information for email addresses
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Contact {
    pub email: String,
    pub name: Option<String>,
}

/// MessageData represents the structured data stored in the message.data JSON field
/// This is primarily for C++ compatibility with the Qt frontend
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
    /// Convert MessageData to JSON string for storage
    pub fn to_json_string(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(self)
    }

    /// Parse MessageData from JSON string
    pub fn from_json_string(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    }
}

/// Email message
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    pub id: String,
    pub account_id: String,
    pub folder_id: String,
    pub thread_id: Option<String>,

    // Message identifiers
    pub header_message_id: Option<String>,
    pub gmail_message_id: Option<String>,
    pub gmail_thread_id: Option<String>,
    pub remote_uid: u32,

    // Message content
    pub subject: String,
    pub date: DateTime<Utc>,

    // Flags
    pub draft: bool,
    pub unread: bool,
    pub starred: bool,

    // Contacts (stored as JSON)
    pub from_contacts: String,
    pub to_contacts: String,
    pub cc_contacts: String,
    pub bcc_contacts: String,
    pub reply_to_contacts: String,

    // Message preview
    pub snippet: String,
    pub is_plaintext: bool,

    // Labels (for Gmail, stored as JSON array)
    pub labels: Option<String>,
}

impl Message {
    /// Create a new message with required fields
    pub fn new(id: String, account_id: String, folder_id: String, remote_uid: u32) -> Self {
        Self {
            id,
            account_id,
            folder_id,
            thread_id: None,
            header_message_id: None,
            gmail_message_id: None,
            gmail_thread_id: None,
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
            labels: None,
        }
    }

    /// Get the first contact from the from_contacts field
    pub fn get_from_contact(&self) -> Contact {
        // Try to parse as single object first, then as array
        serde_json::from_str(&self.from_contacts)
            .or_else(|_| {
                // Try to parse as array and take first element
                serde_json::from_str::<Vec<Contact>>(&self.from_contacts)
                    .map(|mut v| if !v.is_empty() { v.remove(0) } else { Contact {
                        email: String::new(),
                        name: None,
                    }})
            })
            .unwrap_or(Contact {
                email: String::new(),
                name: None,
            })
    }

    /// Build the data JSON field for the database
    pub fn get_data(&self) -> String {
        // Parse contact arrays, with fallback to empty arrays
        let to: Vec<Contact> = serde_json::from_str(&self.to_contacts).unwrap_or_default();
        let cc: Vec<Contact> = serde_json::from_str(&self.cc_contacts).unwrap_or_default();
        let bcc: Vec<Contact> = serde_json::from_str(&self.bcc_contacts).unwrap_or_default();
        let reply_to: Vec<Contact> = serde_json::from_str(&self.reply_to_contacts).unwrap_or_default();

        // Build MessageData
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

        // Serialize to JSON string (should never fail for valid data)
        message_data.to_json_string().unwrap_or_else(|_| "{}".to_string())
    }
}

/// Message body content (stored separately for efficiency)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MessageBody {
    pub id: String,
    pub value: String,
    pub fetched_at: DateTime<Utc>,
}

impl MessageBody {
    pub fn new(id: String, value: String) -> Self {
        Self {
            id,
            value,
            fetched_at: Utc::now(),
        }
    }
}
