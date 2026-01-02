// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Account model representing an email account configuration

use serde::{Deserialize, Serialize};

/// Connection security type
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ConnectionType {
    SSL,
    StartTLS,
    None,
}

impl Default for ConnectionType {
    fn default() -> Self {
        Self::SSL
    }
}

impl ConnectionType {
    /// Parse from string representation (used in config files)
    pub fn from_str(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "ssl" | "0" => Self::SSL,
            "starttls" | "1" => Self::StartTLS,
            "none" | "2" => Self::None,
            _ => Self::SSL,
        }
    }
}

/// Authentication type
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum AuthenticationType {
    Plain,
    OAuth2,
    NoAuth,
}

impl Default for AuthenticationType {
    fn default() -> Self {
        Self::Plain
    }
}

impl AuthenticationType {
    /// Parse from string representation (used in config files)
    pub fn from_str(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "plain" | "0" => Self::Plain,
            "oauth2" | "1" => Self::OAuth2,
            "noauth" | "2" => Self::NoAuth,
            _ => Self::Plain,
        }
    }
}

/// Email account configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Account {
    pub id: String,
    pub email: String,
    pub name: String,
    pub valid: bool,

    // IMAP settings
    pub imap_host: String,
    pub imap_port: u16,
    pub imap_username: String,
    pub imap_password: String,
    pub imap_connection_type: ConnectionType,
    pub imap_authentication_type: AuthenticationType,

    // SMTP settings
    pub smtp_host: String,
    pub smtp_port: u16,
    pub smtp_username: String,
    pub smtp_password: String,
    pub smtp_connection_type: ConnectionType,
    pub smtp_authentication_type: AuthenticationType,

    // OAuth2 tokens (if using OAuth2 authentication)
    pub oauth2_access_token: Option<String>,
    pub oauth2_refresh_token: Option<String>,
    pub oauth2_token_expiry: Option<i64>,
    /// OAuth2 provider ID (e.g., "gmail", "outlook", "yahoo")
    pub oauth2_provider_id: Option<String>,
}

impl Account {
    /// Create a new account with default settings
    pub fn new(id: String) -> Self {
        Self {
            id,
            email: String::new(),
            name: String::new(),
            valid: false,
            imap_host: String::new(),
            imap_port: 993,
            imap_username: String::new(),
            imap_password: String::new(),
            imap_connection_type: ConnectionType::SSL,
            imap_authentication_type: AuthenticationType::Plain,
            smtp_host: String::new(),
            smtp_port: 587,
            smtp_username: String::new(),
            smtp_password: String::new(),
            smtp_connection_type: ConnectionType::StartTLS,
            smtp_authentication_type: AuthenticationType::Plain,
            oauth2_access_token: None,
            oauth2_refresh_token: None,
            oauth2_token_expiry: None,
            oauth2_provider_id: None,
        }
    }

    /// Check if this account uses OAuth2 authentication
    pub fn uses_oauth2(&self) -> bool {
        self.imap_authentication_type == AuthenticationType::OAuth2
            || self.smtp_authentication_type == AuthenticationType::OAuth2
    }

    /// Check if OAuth2 token needs refresh
    pub fn needs_token_refresh(&self) -> bool {
        if !self.uses_oauth2() {
            return false;
        }

        match self.oauth2_token_expiry {
            Some(expiry) => {
                let now = chrono::Utc::now().timestamp();
                // Refresh if token expires in less than 5 minutes
                expiry - now < 300
            }
            None => true,
        }
    }
}
