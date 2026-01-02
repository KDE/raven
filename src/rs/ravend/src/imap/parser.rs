// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Mail parsing utilities for IMAP worker
//!
//! This module handles parsing of email content including:
//! - RFC 2047 encoded headers
//! - Email address extraction and serialization
//! - MIME body parsing (HTML and plain text)
//! - Attachment extraction
//! - Snippet generation for previews

use crate::models::{Message, ParsedAttachment, default_filename, IMMEDIATE_DOWNLOAD_THRESHOLD};

/// Result of parsing a complete email message
#[derive(Debug, Clone)]
pub struct ParsedMessage {
    pub html_body: String,
    pub text_body: String,
    pub snippet: String,
    pub is_plaintext: bool,
    pub attachments: Vec<ParsedAttachment>,
}

impl Default for ParsedMessage {
    fn default() -> Self {
        Self {
            html_body: String::new(),
            text_body: String::new(),
            snippet: String::new(),
            is_plaintext: false,
            attachments: Vec::new(),
        }
    }
}

/// Decode a MIME encoded header (RFC 2047)
/// Handles encoded words like: =?UTF-8?Q?Hello_World?= or =?UTF-8?B?SGVsbG8=?=
pub fn decode_header(s: &str) -> String {
    rfc2047_decoder::decode(s.as_bytes())
        .unwrap_or_else(|_| s.to_string())
}

/// Serialize email addresses to JSON
pub fn serialize_addresses(addrs: &Option<Vec<imap_proto::types::Address>>) -> String {
    let contacts: Vec<serde_json::Value> = addrs
        .as_ref()
        .map(|addrs: &Vec<imap_proto::types::Address>| {
            addrs
                .iter()
                .filter_map(|addr| {
                    let mailbox = addr.mailbox.as_ref()?;
                    let host = addr.host.as_ref()?;
                    let email = format!(
                        "{}@{}",
                        String::from_utf8_lossy(mailbox),
                        String::from_utf8_lossy(host)
                    );
                    let name = addr
                        .name
                        .as_ref()
                        .map(|n| decode_header(&String::from_utf8_lossy(n)));

                    Some(serde_json::json!({
                        "email": email,
                        "name": name
                    }))
                })
                .collect()
        })
        .unwrap_or_default();

    serde_json::to_string(&contacts).unwrap_or_else(|_| "[]".to_string())
}

/// Generate a snippet from a message (for thread preview)
pub fn get_snippet_from_message(message: &Message) -> String {
    // For now, use the subject as a basic snippet
    // In the future, this could be the first few words of the body
    let snippet = message.subject.clone();
    let chars: Vec<char> = snippet.chars().collect();
    if chars.len() > 100 {
        let truncated: String = chars[..97].iter().collect();
        format!("{}...", truncated)
    } else {
        snippet
    }
}

/// Parse a raw email body and extract HTML, text, snippet, and attachments
/// This is the full parsing function that also extracts file attachments
pub fn parse_message_body_full(body_bytes: &[u8]) -> ParsedMessage {
    use mail_parser::{PartType, MimeHeaders};

    let parsed = match mail_parser::MessageParser::default().parse(body_bytes) {
        Some(msg) => msg,
        None => return ParsedMessage::default(),
    };

    let mut html_body = String::new();
    let mut text_body = String::new();
    let mut attachments = Vec::new();
    let mut has_html_part = false;

    // Iterate through all parts
    for (part_index, part) in parsed.parts.iter().enumerate() {
        let part_id = part_index.to_string();

        match &part.body {
            PartType::Html(html) => {
                has_html_part = true;
                if html_body.is_empty() {
                    html_body = html.to_string();
                }
            }
            PartType::Text(text) => {
                if text_body.is_empty() {
                    text_body = text.to_string();
                }
            }
            PartType::Binary(data) | PartType::InlineBinary(data) => {
                let is_inline = matches!(&part.body, PartType::InlineBinary(_));

                // Get content type
                let content_type = part.content_type()
                    .map(|ct| {
                        let main = ct.c_type.as_ref();
                        let sub = ct.subtype().unwrap_or("octet-stream");
                        format!("{}/{}", main, sub)
                    })
                    .unwrap_or_else(|| "application/octet-stream".to_string());

                // Get filename from Content-Disposition or Content-Type
                let filename = part.attachment_name()
                    .map(|s| decode_header(s))
                    .unwrap_or_else(|| default_filename(&content_type, part_index));

                // Get Content-ID for inline attachments (strip angle brackets)
                let content_id = part.content_id()
                    .map(|s| s.trim_start_matches('<').trim_end_matches('>').to_string());

                let size = data.len();

                // Only include data if it should be downloaded immediately
                let include_data = size < IMMEDIATE_DOWNLOAD_THRESHOLD;

                attachments.push(ParsedAttachment {
                    filename,
                    content_type,
                    content_id,
                    part_id,
                    size,
                    is_inline,
                    data: if include_data { Some(data.to_vec()) } else { None },
                });
            }
            _ => {}
        }
    }

    let is_plaintext = !has_html_part && !text_body.is_empty();
    let snippet = generate_snippet(&text_body);

    ParsedMessage {
        html_body,
        text_body,
        snippet,
        is_plaintext,
        attachments,
    }
}

/// Replace cid: URLs in HTML with file:// URLs
/// Takes a list of (content_id, file_path) pairs
pub fn replace_cid_urls(html: &str, replacements: &[(String, String)]) -> String {
    let mut result = html.to_string();

    for (cid, file_url) in replacements {
        // Handle various cid: URL formats
        // cid:content-id (without angle brackets, most common in HTML)
        // "cid:content-id" (quoted in attributes)
        // 'cid:content-id' (single-quoted)
        let patterns = [
            format!("cid:{}", cid),
            format!("\"cid:{}\"", cid),
            format!("'cid:{}'", cid),
        ];

        for pattern in patterns {
            if result.contains(&pattern) {
                let replacement = if pattern.starts_with('"') {
                    format!("\"{}\"", file_url)
                } else if pattern.starts_with('\'') {
                    format!("'{}'", file_url)
                } else {
                    file_url.to_string()
                };
                result = result.replace(&pattern, &replacement);
            }
        }
    }

    result
}

/// Format a sender JSON array as a display string (e.g., "John Doe" or "john@example.com")
pub fn format_sender(from_contacts: &str) -> String {
    if let Ok(contacts) = serde_json::from_str::<Vec<serde_json::Value>>(from_contacts) {
        if let Some(first) = contacts.first() {
            if let Some(name) = first.get("name").and_then(|n| n.as_str()) {
                if !name.is_empty() {
                    return name.to_string();
                }
            }
            if let Some(email) = first.get("email").and_then(|e| e.as_str()) {
                return email.to_string();
            }
        }
    }
    from_contacts.to_string()
}

/// Generate a clean snippet from message text
pub fn generate_snippet(text: &str) -> String {
    // Clean up the text: collapse whitespace, remove excess newlines
    let cleaned: String = text
        .chars()
        .filter(|c| !c.is_control() || c.is_whitespace())
        .map(|c| if c.is_whitespace() { ' ' } else { c })
        .collect::<String>()
        .split_whitespace()
        .collect::<Vec<&str>>()
        .join(" ");

    // Take first ~150 characters, respecting char boundaries and word boundaries
    let chars: Vec<char> = cleaned.chars().collect();
    if chars.len() <= 150 {
        cleaned
    } else {
        // Take first 150 chars
        let truncated: String = chars[..150].iter().collect();
        // Find a good break point (space) near the end
        if let Some(last_space) = truncated.rfind(' ') {
            format!("{}...", &truncated[..last_space])
        } else {
            format!("{}...", truncated)
        }
    }
}
