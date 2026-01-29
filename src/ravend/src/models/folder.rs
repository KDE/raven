// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

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

    /// Detect folder role from IMAP path (handles Gmail, common names, and localized variants)
    pub fn detect_from_path(path: &str) -> Self {
        let lower_path = path.to_lowercase();

        if lower_path == "inbox" {
            return Self::Inbox;
        }

        if lower_path.contains("trash") || lower_path.contains("deleted") || lower_path.contains("papierkorb") || lower_path.contains("papelera") {
            return Self::Trash;
        }

        if lower_path.contains("spam") || lower_path.contains("junk") || lower_path.contains("bulk") {
            return Self::Spam;
        }

        if lower_path.contains("sent") || lower_path.contains("gesendet") || lower_path.contains("postausgang") {
            return Self::Sent;
        }

        if lower_path.contains("draft") || lower_path.contains("brouillon") {
            return Self::Drafts;
        }

        if lower_path.contains("archive") || lower_path.contains("archiv") {
            return Self::Archive;
        }

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

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Folder {
    pub id: String,
    pub account_id: String,
    pub path: String,
    pub role: FolderRole,
    pub created_at: DateTime<Utc>,
    pub data: Option<String>,
    pub uid_validity: Option<u32>,
    pub uid_next: Option<u32>,
    pub highest_mod_seq: Option<u64>,
}

impl Folder {
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
