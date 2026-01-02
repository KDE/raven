// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! KWallet integration for secure password storage via cxx bridge
//!
//! This module provides access to KWallet via a C++ bridge using cxx-rs.

use anyhow::Result;
use log::{debug, info};

#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("kwallet_bridge.h");

        fn kwallet_open() -> bool;
        fn kwallet_close();
        fn kwallet_read_password(key: &str) -> String;
        fn kwallet_write_password(key: &str, password: &str) -> bool;
        fn kwallet_remove_entry(key: &str) -> bool;
    }
}

/// KWallet client for reading/writing passwords
pub struct KWalletClient {
    _private: (),
}

impl KWalletClient {
    /// Create a new KWallet client
    pub fn new() -> Result<Self> {
        if !ffi::kwallet_open() {
            return Err(anyhow::anyhow!("Failed to open KWallet"));
        }

        info!("Connected to KWallet");
        Ok(Self { _private: () })
    }

    /// Read a password for the given key
    pub fn read_password(&self, key: &str) -> Result<Option<String>> {
        debug!("Reading password for key: {}", key);

        let password = ffi::kwallet_read_password(key);
        if password.is_empty() {
            debug!("No password found for key: {}", key);
            Ok(None)
        } else {
            debug!("Found password for key: {}", key);
            Ok(Some(password))
        }
    }

    /// Write a password for the given key
    ///
    /// FUTURE USE: For storing passwords during account setup or password changes
    /// Currently passwords are written by the frontend during account creation
    #[allow(dead_code)]
    pub fn write_password(&self, key: &str, password: &str) -> Result<()> {
        debug!("Writing password for key: {}", key);

        if ffi::kwallet_write_password(key, password) {
            Ok(())
        } else {
            Err(anyhow::anyhow!("Failed to write password to KWallet"))
        }
    }

    /// Remove an entry from KWallet
    pub fn remove_entry(&self, key: &str) -> Result<()> {
        debug!("Removing entry for key: {}", key);

        if ffi::kwallet_remove_entry(key) {
            Ok(())
        } else {
            Err(anyhow::anyhow!("Failed to remove entry from KWallet"))
        }
    }
}

impl Drop for KWalletClient {
    fn drop(&mut self) {
        ffi::kwallet_close();
        debug!("Closed KWallet connection");
    }
}

/// Helper function to get password key for an account
pub fn imap_password_key(account_id: &str) -> String {
    format!("{}-imapPassword", account_id)
}

/// Helper function to get SMTP password key for an account
pub fn smtp_password_key(account_id: &str) -> String {
    format!("{}-smtpPassword", account_id)
}

/// Helper function to get OAuth2 access token key for an account
pub fn oauth2_access_token_key(account_id: &str) -> String {
    format!("{}-oauthAccessToken", account_id)
}

/// Helper function to get OAuth2 refresh token key for an account
pub fn oauth2_refresh_token_key(account_id: &str) -> String {
    format!("{}-oauthRefreshToken", account_id)
}
