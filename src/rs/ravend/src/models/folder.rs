// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Folder model representing an IMAP mailbox/folder

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

/// Folder role/type for special folders
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum FolderRole {
    Inbox,
    Sent,
    Drafts,
    Trash,
    Spam,
    Archive,
    All,
    Starred,
    Important,
    Custom(String),
}

impl Default for FolderRole {
    fn default() -> Self {
        Self::Custom(String::new())
    }
}

impl FolderRole {
    /// Convert to string for storage
    pub fn to_string(&self) -> String {
        match self {
            Self::Inbox => "inbox".to_string(),
            Self::Sent => "sent".to_string(),
            Self::Drafts => "drafts".to_string(),
            Self::Trash => "trash".to_string(),
            Self::Spam => "spam".to_string(),
            Self::Archive => "archive".to_string(),
            Self::All => "all".to_string(),
            Self::Starred => "starred".to_string(),
            Self::Important => "important".to_string(),
            Self::Custom(name) => name.clone(),
        }
    }

    /// Detect folder role from folder path/name
    pub fn detect_from_path(path: &str) -> Self {
        let lower_path = path.to_lowercase();

        // Check for common folder names
        if lower_path == "inbox" {
            return Self::Inbox;
        }

        // Trash variants
        if lower_path.contains("trash")
            || lower_path.contains("deleted")
            || lower_path.contains("papierkorb")
            || lower_path.contains("papelera")
        {
            return Self::Trash;
        }

        // Spam variants
        if lower_path.contains("spam")
            || lower_path.contains("junk")
            || lower_path.contains("bulk")
        {
            return Self::Spam;
        }

        // Sent variants
        if lower_path.contains("sent")
            || lower_path.contains("gesendet")
            || lower_path.contains("postausgang")
        {
            return Self::Sent;
        }

        // Drafts variants
        if lower_path.contains("draft") || lower_path.contains("brouillon") {
            return Self::Drafts;
        }

        // Archive
        if lower_path.contains("archive") || lower_path.contains("archiv") {
            return Self::Archive;
        }

        // Gmail special folders
        if lower_path.contains("[gmail]") || lower_path.contains("[google mail]") {
            if lower_path.contains("all mail") {
                return Self::All;
            }
            if lower_path.contains("starred") {
                return Self::Starred;
            }
            if lower_path.contains("important") {
                return Self::Important;
            }
        }

        Self::Custom(String::new())
    }
}

/// Email folder/mailbox
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Folder {
    pub id: String,
    pub account_id: String,
    pub path: String,
    pub role: FolderRole,
    pub created_at: DateTime<Utc>,

    // Additional metadata stored as JSON
    pub data: Option<String>,

    // Sync state for CONDSTORE/QRESYNC
    /// UIDVALIDITY - changes when mailbox is recreated
    pub uid_validity: Option<u32>,
    /// UIDNEXT - next UID to be assigned, used for detecting new messages
    pub uid_next: Option<u32>,
    /// HIGHESTMODSEQ - for CONDSTORE incremental sync
    pub highest_mod_seq: Option<u64>,
}

impl Folder {
    /// Create a new folder
    pub fn new(id: String, account_id: String, path: String) -> Self {
        let role = FolderRole::detect_from_path(&path);

        Self {
            id,
            account_id,
            path,
            role,
            created_at: Utc::now(),
            data: None,
            uid_validity: None,
            uid_next: None,
            highest_mod_seq: None,
        }
    }
}
