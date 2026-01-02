// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Secure credential storage using the keyring crate
//!
//! This module provides secure password storage via the system's
//! secret service (KWallet on KDE, GNOME Keyring on GNOME, etc.)

use keyring::Entry;
use log::debug;

const SERVICE_NAME: &str = "raven";

/// Read a password from the secret store
pub fn read_password(key: &str) -> Option<String> {
    let entry = Entry::new(SERVICE_NAME, key).ok()?;
    match entry.get_password() {
        Ok(password) => {
            debug!("Read password for key: {}", key);
            Some(password)
        }
        Err(keyring::Error::NoEntry) => {
            debug!("No password found for key: {}", key);
            None
        }
        Err(e) => {
            debug!("Failed to read password for key {}: {}", key, e);
            None
        }
    }
}

/// Write a password to the secret store
pub fn write_password(key: &str, password: &str) -> bool {
    let entry = match Entry::new(SERVICE_NAME, key) {
        Ok(e) => e,
        Err(e) => {
            debug!("Failed to create keyring entry for {}: {}", key, e);
            return false;
        }
    };

    match entry.set_password(password) {
        Ok(()) => {
            debug!("Wrote password for key: {}", key);
            true
        }
        Err(e) => {
            debug!("Failed to write password for key {}: {}", key, e);
            false
        }
    }
}

/// Delete a password from the secret store
pub fn delete_password(key: &str) -> bool {
    let entry = match Entry::new(SERVICE_NAME, key) {
        Ok(e) => e,
        Err(e) => {
            debug!("Failed to create keyring entry for {}: {}", key, e);
            return false;
        }
    };

    match entry.delete_credential() {
        Ok(()) => {
            debug!("Deleted password for key: {}", key);
            true
        }
        Err(keyring::Error::NoEntry) => {
            debug!("No password to delete for key: {}", key);
            true // Not an error if it doesn't exist
        }
        Err(e) => {
            debug!("Failed to delete password for key {}: {}", key, e);
            false
        }
    }
}

// Key helpers for account passwords

pub fn imap_password_key(account_id: &str) -> String {
    format!("{}-imapPassword", account_id)
}

pub fn smtp_password_key(account_id: &str) -> String {
    format!("{}-smtpPassword", account_id)
}

pub fn oauth2_access_token_key(account_id: &str) -> String {
    format!("{}-oauthAccessToken", account_id)
}

pub fn oauth2_refresh_token_key(account_id: &str) -> String {
    format!("{}-oauthRefreshToken", account_id)
}
