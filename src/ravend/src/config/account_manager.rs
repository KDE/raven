// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use crate::secrets;
use crate::models::{Account, AuthenticationType, ConnectionType};
use anyhow::{Context, Result};
use configparser::ini::Ini;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::{Arc, OnceLock, RwLock};

static INSTANCE: OnceLock<AccountManager> = OnceLock::new();

pub struct AccountManager {
    config_dir: PathBuf,
    accounts: Arc<RwLock<Vec<Account>>>,
}

impl AccountManager {
    pub fn init(config_dir: &Path) -> Result<()> {
        INSTANCE.set(Self {
            config_dir: config_dir.to_path_buf(),
            accounts: Arc::new(RwLock::new(Vec::new())),
        }).map_err(|_| anyhow::anyhow!("AccountManager already initialized"))
    }

    pub fn global() -> &'static AccountManager {
        INSTANCE.get().expect("AccountManager not initialized")
    }

    pub fn accounts(&self) -> Vec<Account> {
        self.accounts.read().unwrap().clone()
    }

    pub fn reload(&self) -> Result<Vec<Account>> {
        let new_accounts = self.load_accounts_from_disk()?;
        *self.accounts.write().unwrap() = new_accounts.clone();
        Ok(new_accounts)
    }

    fn load_accounts_from_disk(&self) -> Result<Vec<Account>> {
        Ok(self.list_account_ids()?
            .into_iter()
            .filter_map(|id| self.load_account(&id).ok())
            .filter(|a| a.valid)
            .collect())
    }

    fn load_account(&self, account_id: &str) -> Result<Account> {
        let config_path = self.config_dir.join("accounts").join(account_id).join("account.ini");
        if !config_path.exists() {
            return Ok(Account::new(account_id.to_string()));
        }

        let mut ini = Ini::new_cs();
        ini.load(&config_path).map_err(|e| anyhow::anyhow!("{e}"))?;

        let mut a = Account::new(account_id.to_string());

        // Metadata
        a.valid = ini.get("Metadata", "valid").and_then(|v| v.parse().ok()).unwrap_or(false);

        // Account info
        a.email = ini.get("Account", "email").unwrap_or_default();
        a.name = ini.get("Account", "name").unwrap_or_default();

        // IMAP
        a.imap_host = ini.get("Account", "imapHost").unwrap_or_default();
        a.imap_port = ini.getint("Account", "imapPort").ok().flatten().unwrap_or(993) as u16;
        a.imap_username = ini.get("Account", "imapUsername").unwrap_or_default();
        a.imap_connection_type = ConnectionType::from_str(&ini.getint("Account", "imapConnectionType").ok().flatten().unwrap_or(0).to_string());
        a.imap_authentication_type = AuthenticationType::from_str(&ini.getint("Account", "imapAuthenticationType").ok().flatten().unwrap_or(0).to_string());

        // SMTP
        a.smtp_host = ini.get("Account", "smtpHost").unwrap_or_default();
        a.smtp_port = ini.getint("Account", "smtpPort").ok().flatten().unwrap_or(587) as u16;
        a.smtp_username = ini.get("Account", "smtpUsername").unwrap_or_default();
        a.smtp_connection_type = ConnectionType::from_str(&ini.getint("Account", "smtpConnectionType").ok().flatten().unwrap_or(0).to_string());
        a.smtp_authentication_type = AuthenticationType::from_str(&ini.getint("Account", "smtpAuthenticationType").ok().flatten().unwrap_or(0).to_string());

        // OAuth2
        a.oauth2_provider_id = ini.get("OAuth2", "providerId").filter(|s| !s.is_empty());
        a.oauth2_token_expiry = ini.getint("OAuth2", "tokenExpiry").ok().flatten().filter(|&t| t > 0);

        // Secrets
        a.imap_password = secrets::read_password(&secrets::imap_password_key(account_id)).unwrap_or_default();
        a.smtp_password = secrets::read_password(&secrets::smtp_password_key(account_id)).unwrap_or_default();
        a.oauth2_access_token = secrets::read_password(&secrets::oauth2_access_token_key(account_id));
        a.oauth2_refresh_token = secrets::read_password(&secrets::oauth2_refresh_token_key(account_id));

        Ok(a)
    }

    fn list_account_ids(&self) -> Result<Vec<String>> {
        let accounts_dir = self.config_dir.join("accounts");
        if !accounts_dir.exists() { return Ok(vec![]); }

        Ok(fs::read_dir(&accounts_dir)
            .with_context(|| format!("Failed to read {}", accounts_dir.display()))?
            .filter_map(|e| e.ok())
            .filter(|e| e.path().is_dir())
            .filter_map(|e| e.file_name().to_str().map(String::from))
            .filter(|n| !n.starts_with('.'))
            .collect())
    }

    pub fn delete_account(&self, account_id: &str) -> Result<()> {
        let account_dir = self.config_dir.join("accounts").join(account_id);
        if account_dir.exists() {
            fs::remove_dir_all(&account_dir)
                .with_context(|| format!("Failed to delete {}", account_dir.display()))?;
        }

        for key in [
            secrets::imap_password_key(account_id),
            secrets::smtp_password_key(account_id),
            secrets::oauth2_access_token_key(account_id),
            secrets::oauth2_refresh_token_key(account_id),
        ] {
            secrets::delete_password(&key);
        }
        Ok(())
    }
}
