// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Account configuration management
//!
//! Reads account configurations from ~/.config/raven/accounts/{account_id}/account.ini
//! Uses native Rust INI parsing. Passwords are stored in KWallet.

use crate::kwallet::{self, KWalletClient};
use crate::models::{Account, AuthenticationType, ConnectionType};
use anyhow::{Context, Result};
use configparser::ini::Ini;
use log::{debug, error, info, warn};
use std::fs;
use std::path::{Path, PathBuf};

/// Account configuration manager
pub struct AccountManager {
    config_dir: PathBuf,
}

impl AccountManager {
    /// Create a new account manager
    pub fn new(config_dir: &Path) -> Result<Self> {
        info!("Initialized account manager with config dir: {}", config_dir.display());
        Ok(Self { config_dir: config_dir.to_path_buf() })
    }

    /// Get the path to an account's config file
    fn account_config_path(&self, account_id: &str) -> PathBuf {
        self.config_dir
            .join("accounts")
            .join(account_id)
            .join("account.ini")
    }

    /// Load all configured accounts
    pub fn load_accounts(&self) -> Result<Vec<Account>> {
        let kwallet = match KWalletClient::new() {
            Ok(kw) => Some(kw),
            Err(e) => {
                error!("Failed to connect to KWallet: {}. Passwords will not be available.", e);
                None
            }
        };

        let account_ids = self.list_accounts()?;
        let mut accounts = Vec::new();

        for account_id in account_ids {
            match self.load_account(&account_id, kwallet.as_ref()) {
                Ok(account) => {
                    if account.valid {
                        info!("Loaded account: {} ({})", account.email, account.id);
                        accounts.push(account);
                    } else {
                        warn!("Skipping invalid account: {}", account_id);
                    }
                }
                Err(e) => {
                    warn!("Failed to load account {}: {}", account_id, e);
                }
            }
        }

        Ok(accounts)
    }

    /// Load a single account by ID
    fn load_account(&self, account_id: &str, kwallet: Option<&KWalletClient>) -> Result<Account> {
        debug!("Loading account config for: {}", account_id);

        let config_path = self.account_config_path(account_id);

        if !config_path.exists() {
            return Ok(Account::new(account_id.to_string()));
        }

        let mut ini = Ini::new_cs();
        ini.load(&config_path)
            .map_err(|e| anyhow::anyhow!("Failed to load config file {}: {}", config_path.display(), e))?;

        let mut account = Account::new(account_id.to_string());

        // Read Metadata section
        account.valid = ini.get("Metadata", "valid")
            .and_then(|v| v.parse::<bool>().ok())
            .unwrap_or(false);

        // Read Account section
        account.email = ini.get("Account", "email").unwrap_or_default();
        account.name = ini.get("Account", "name").unwrap_or_default();

        // IMAP settings
        account.imap_host = ini.get("Account", "imapHost").unwrap_or_default();
        account.imap_port = ini.getint("Account", "imapPort")
            .ok()
            .flatten()
            .unwrap_or(993) as u16;
        account.imap_username = ini.get("Account", "imapUsername").unwrap_or_default();

        let imap_conn = ini.getint("Account", "imapConnectionType")
            .ok()
            .flatten()
            .unwrap_or(0);
        account.imap_connection_type = ConnectionType::from_str(&imap_conn.to_string());

        let imap_auth = ini.getint("Account", "imapAuthenticationType")
            .ok()
            .flatten()
            .unwrap_or(0);
        account.imap_authentication_type = AuthenticationType::from_str(&imap_auth.to_string());

        // SMTP settings
        account.smtp_host = ini.get("Account", "smtpHost").unwrap_or_default();
        account.smtp_port = ini.getint("Account", "smtpPort")
            .ok()
            .flatten()
            .unwrap_or(587) as u16;
        account.smtp_username = ini.get("Account", "smtpUsername").unwrap_or_default();

        let smtp_conn = ini.getint("Account", "smtpConnectionType")
            .ok()
            .flatten()
            .unwrap_or(0);
        account.smtp_connection_type = ConnectionType::from_str(&smtp_conn.to_string());

        let smtp_auth = ini.getint("Account", "smtpAuthenticationType")
            .ok()
            .flatten()
            .unwrap_or(0);
        account.smtp_authentication_type = AuthenticationType::from_str(&smtp_auth.to_string());

        // OAuth2 settings
        if let Some(provider_id) = ini.get("OAuth2", "providerId") {
            if !provider_id.is_empty() {
                account.oauth2_provider_id = Some(provider_id);
            }
        }

        if let Some(token_expiry) = ini.getint("OAuth2", "tokenExpiry").ok().flatten() {
            if token_expiry > 0 {
                account.oauth2_token_expiry = Some(token_expiry);
            }
        }

        // Read passwords from KWallet
        if let Some(kw) = kwallet {
            if let Ok(Some(password)) = kw.read_password(&kwallet::imap_password_key(account_id)) {
                debug!("Loaded IMAP password from KWallet for account {}", account_id);
                account.imap_password = password;
            }

            if let Ok(Some(password)) = kw.read_password(&kwallet::smtp_password_key(account_id)) {
                debug!("Loaded SMTP password from KWallet for account {}", account_id);
                account.smtp_password = password;
            }

            if let Ok(Some(token)) = kw.read_password(&kwallet::oauth2_access_token_key(account_id)) {
                debug!("Loaded OAuth2 access token from KWallet for account {}", account_id);
                account.oauth2_access_token = Some(token);
            }

            if let Ok(Some(token)) = kw.read_password(&kwallet::oauth2_refresh_token_key(account_id)) {
                debug!("Loaded OAuth2 refresh token from KWallet for account {}", account_id);
                account.oauth2_refresh_token = Some(token);
            }
        }

        Ok(account)
    }

    /// List all account IDs
    fn list_accounts(&self) -> Result<Vec<String>> {
        debug!("Listing all accounts");

        let accounts_dir = self.config_dir.join("accounts");

        if !accounts_dir.exists() {
            info!("Accounts directory doesn't exist, returning empty list");
            return Ok(Vec::new());
        }

        let mut accounts = Vec::new();

        let entries = fs::read_dir(&accounts_dir)
            .with_context(|| format!("Failed to read accounts directory: {}", accounts_dir.display()))?;

        for entry in entries {
            let entry = entry.with_context(|| "Failed to read directory entry")?;
            let path = entry.path();

            if path.is_dir() {
                if let Some(name) = path.file_name() {
                    if let Some(name_str) = name.to_str() {
                        if !name_str.starts_with('.') {
                            accounts.push(name_str.to_string());
                        }
                    }
                }
            }
        }

        info!("Found {} accounts", accounts.len());
        Ok(accounts)
    }

    /// Delete an account configuration and its KWallet entries
    pub fn delete_account(&self, account_id: &str) -> Result<()> {
        info!("Deleting account: {}", account_id);

        let account_dir = self.config_dir.join("accounts").join(account_id);

        if account_dir.exists() {
            fs::remove_dir_all(&account_dir)
                .with_context(|| format!("Failed to delete account directory: {}", account_dir.display()))?;
        }

        // Remove KWallet entries
        if let Ok(kw) = KWalletClient::new() {
            let keys = [
                kwallet::imap_password_key(account_id),
                kwallet::smtp_password_key(account_id),
                kwallet::oauth2_access_token_key(account_id),
                kwallet::oauth2_refresh_token_key(account_id),
            ];

            for key in &keys {
                if let Err(e) = kw.remove_entry(key) {
                    warn!("Failed to remove KWallet entry {}: {}", key, e);
                }
            }
        }

        info!("Account {} deleted successfully", account_id);
        Ok(())
    }
}
