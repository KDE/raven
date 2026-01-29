// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use serde::{Deserialize, Serialize};

/// Attachments smaller than 1MB are downloaded immediately during sync
pub const IMMEDIATE_DOWNLOAD_THRESHOLD: usize = 1_048_576;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Attachment {
    pub id: String,
    pub account_id: String,
    pub message_id: String,
    pub filename: String,
    pub part_id: String,
    pub content_id: Option<String>,
    pub content_type: String,
    pub size: usize,
    pub is_inline: bool,
    pub downloaded: bool,
}

impl Attachment {
    pub fn new(
        account_id: String,
        message_id: String,
        filename: String,
        part_id: String,
        content_type: String,
        size: usize,
    ) -> Self {
        let id = uuid::Uuid::new_v4().to_string().replace("-", "");
        Self {
            id,
            account_id,
            message_id,
            filename,
            part_id,
            content_id: None,
            content_type,
            size,
            is_inline: false,
            downloaded: false,
        }
    }

    #[allow(dead_code)]
    pub fn should_download_immediately(&self) -> bool {
        self.size < IMMEDIATE_DOWNLOAD_THRESHOLD
    }

    /// Returns filename for disk storage: {message_id}_{sanitized_filename}
    pub fn disk_filename(&self) -> String {
        let msg_id_safe = self.message_id.replace(":", "_").replace("/", "_");
        let sanitized = sanitize_filename(&self.filename);
        format!("{}_{}", msg_id_safe, sanitized)
    }
}

pub fn sanitize_filename(name: &str) -> String {
    let sanitized: String = name
        .chars()
        .map(|c| match c {
            '/' | '\\' | ':' | '*' | '?' | '"' | '<' | '>' | '|' | '\0' => '_',
            c if c.is_control() => '_',
            c => c,
        })
        .collect();

    let trimmed = sanitized.trim();
    if trimmed.is_empty() {
        "attachment".to_string()
    } else {
        trimmed.to_string()
    }
}

pub fn default_filename(content_type: &str, index: usize) -> String {
    let lower = content_type.to_lowercase();

    if lower.contains("text/calendar") || lower.contains("text/x-vcalendar") {
        format!("event_{}.ics", index)
    } else if lower.contains("image/png") {
        format!("image_{}.png", index)
    } else if lower.contains("image/jpeg") || lower.contains("image/jpg") {
        format!("image_{}.jpg", index)
    } else if lower.contains("image/gif") {
        format!("image_{}.gif", index)
    } else if lower.contains("image/webp") {
        format!("image_{}.webp", index)
    } else if lower.contains("image/svg") {
        format!("image_{}.svg", index)
    } else if lower.contains("application/pdf") {
        format!("document_{}.pdf", index)
    } else if lower.contains("text/plain") {
        format!("text_{}.txt", index)
    } else if lower.contains("text/html") {
        format!("document_{}.html", index)
    } else if lower.contains("application/zip") {
        format!("archive_{}.zip", index)
    } else if lower.contains("message/rfc822") {
        format!("message_{}.eml", index)
    } else {
        format!("attachment_{}", index)
    }
}

/// Transient attachment data extracted during MIME parsing (before database storage)
#[derive(Debug, Clone)]
pub struct ParsedAttachment {
    pub filename: String,
    pub content_type: String,
    pub content_id: Option<String>,
    pub part_id: String,
    pub size: usize,
    pub is_inline: bool,
    /// Only populated for small attachments that should be saved immediately
    pub data: Option<Vec<u8>>,
}

impl ParsedAttachment {
    pub fn to_attachment(&self, account_id: &str, message_id: &str) -> Attachment {
        let mut attachment = Attachment::new(
            account_id.to_string(),
            message_id.to_string(),
            self.filename.clone(),
            self.part_id.clone(),
            self.content_type.clone(),
            self.size,
        );
        attachment.content_id = self.content_id.clone();
        attachment.is_inline = self.is_inline;
        attachment
    }
}
