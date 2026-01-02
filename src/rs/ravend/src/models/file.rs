// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! File attachment model for email attachments

use serde::{Deserialize, Serialize};

/// Size threshold for immediate download during sync (1MB)
pub const IMMEDIATE_DOWNLOAD_THRESHOLD: usize = 1_048_576;

/// File attachment extracted from an email
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Attachment {
    pub id: String,
    pub account_id: String,
    pub message_id: String,
    pub filename: String,
    pub part_id: String,            // IMAP MIME part ID for on-demand fetch
    pub content_id: Option<String>, // For inline attachments (cid:)
    pub content_type: String,
    pub size: usize,
    pub is_inline: bool,
    pub downloaded: bool,           // Whether file exists on disk
}

impl Attachment {
    /// Create a new attachment
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

    /// Should this attachment be downloaded immediately during sync?
    ///
    /// FUTURE USE: For auto-downloading small attachments (< 1MB) in the background
    /// This improves UX by having small attachments ready when user views the email
    #[allow(dead_code)]
    pub fn should_download_immediately(&self) -> bool {
        self.size < IMMEDIATE_DOWNLOAD_THRESHOLD
    }

    /// Generate the on-disk filename
    /// Format: {message_id_safe}_{sanitized_filename}
    pub fn disk_filename(&self) -> String {
        let msg_id_safe = self.message_id.replace(":", "_").replace("/", "_");
        let sanitized = sanitize_filename(&self.filename);
        format!("{}_{}", msg_id_safe, sanitized)
    }
}

/// Sanitize a filename for safe disk storage
pub fn sanitize_filename(name: &str) -> String {
    let sanitized: String = name
        .chars()
        .map(|c| match c {
            '/' | '\\' | ':' | '*' | '?' | '"' | '<' | '>' | '|' | '\0' => '_',
            c if c.is_control() => '_',
            c => c,
        })
        .collect();

    // Trim whitespace and ensure not empty
    let trimmed = sanitized.trim();
    if trimmed.is_empty() {
        "attachment".to_string()
    } else {
        trimmed.to_string()
    }
}

/// Generate a default filename based on content type
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

/// Attachment data extracted during parsing (before database storage)
#[derive(Debug, Clone)]
pub struct ParsedAttachment {
    pub filename: String,
    pub content_type: String,
    pub content_id: Option<String>,
    pub part_id: String,
    pub size: usize,
    pub is_inline: bool,
    pub data: Option<Vec<u8>>, // Only populated for attachments that should be saved immediately
}

impl ParsedAttachment {
    /// Convert to Attachment for database storage
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sanitize_filename() {
        assert_eq!(sanitize_filename("hello.txt"), "hello.txt");
        assert_eq!(sanitize_filename("file/with/slashes.pdf"), "file_with_slashes.pdf");
        assert_eq!(sanitize_filename("file:with:colons.doc"), "file_with_colons.doc");
        assert_eq!(sanitize_filename("  spaces  "), "spaces");
        assert_eq!(sanitize_filename(""), "attachment");
    }

    #[test]
    fn test_default_filename() {
        assert_eq!(default_filename("image/png", 0), "image_0.png");
        assert_eq!(default_filename("application/pdf", 1), "document_1.pdf");
        assert_eq!(default_filename("unknown/type", 2), "attachment_2");
    }

    #[test]
    fn test_should_download_immediately() {
        let mut attachment = Attachment::new(
            "acc".to_string(),
            "msg".to_string(),
            "test.txt".to_string(),
            "1".to_string(),
            "text/plain".to_string(),
            1000,
        );
        assert!(attachment.should_download_immediately());

        attachment.size = 2_000_000; // 2MB
        assert!(!attachment.should_download_immediately());
    }
}
