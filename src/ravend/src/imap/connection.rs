// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! IMAP connection and authentication (SSL/STARTTLS, Plain/OAuth2)

use crate::secrets::{self, imap_password_key, oauth2_access_token_key, oauth2_refresh_token_key};
use crate::models::{Account, AuthenticationType, ConnectionType};
use crate::oauth2;
use native_tls::TlsConnector;
use std::net::TcpStream;

/// Type alias for an authenticated IMAP session
pub type ImapSession = imap::Session<native_tls::TlsStream<TcpStream>>;

/// OAuth2 XOAUTH2 authenticator for IMAP
pub struct OAuth2Authenticator(pub String);

impl imap::Authenticator for OAuth2Authenticator {
    type Response = String;

    fn process(&self, _data: &[u8]) -> Self::Response {
        self.0.clone()
    }
}

/// Connect to an IMAP server and authenticate with the given credentials
pub fn connect_and_authenticate(
    account: &Account,
    password: Option<&str>,
    access_token: Option<&str>,
) -> Result<ImapSession, String> {
    let host = &account.imap_host;
    let port = account.imap_port;

    // Build TLS connector
    let tls = TlsConnector::builder()
        .build()
        .map_err(|e| format!("Failed to build TLS connector: {}", e))?;

    // Connect based on connection type
    let client = match account.imap_connection_type {
        ConnectionType::SSL => {
            imap::connect((host.as_str(), port), host, &tls)
                .map_err(|e| format!("Failed to connect to IMAP server: {}", e))?
        }
        ConnectionType::StartTLS => {
            let stream = TcpStream::connect((host.as_str(), port))
                .map_err(|e| format!("Failed to connect to IMAP server: {}", e))?;
            let client = imap::Client::new(stream);
            client
                .secure(host, &tls)
                .map_err(|e| format!("STARTTLS handshake failed: {}", e))?
        }
        ConnectionType::None => {
            return Err("Unencrypted connections not supported".to_string());
        }
    };

    // Authenticate based on authentication type
    match account.imap_authentication_type {
        AuthenticationType::Plain => {
            let pass = password.ok_or_else(|| "Password required for Plain authentication".to_string())?;
            if pass.is_empty() {
                return Err("IMAP password is empty".to_string());
            }
            client
                .login(&account.imap_username, pass)
                .map_err(|(e, _)| format!("IMAP login failed: {}", e))
        }
        AuthenticationType::OAuth2 => {
            let token = access_token.ok_or_else(|| "Access token required for OAuth2 authentication".to_string())?;
            let auth_string = format!(
                "user={}\x01auth=Bearer {}\x01\x01",
                account.imap_username, token
            );
            client
                .authenticate("XOAUTH2", &OAuth2Authenticator(auth_string))
                .map_err(|(e, _)| format!("IMAP OAuth2 authentication failed: {}", e))
        }
        AuthenticationType::NoAuth => {
            Err("NoAuth authentication type not supported".to_string())
        }
    }
}

/// Connect to IMAP, retrieving credentials from the secret store (refreshes OAuth2 if needed)
pub fn connect_with_secrets(account: &Account) -> Result<ImapSession, String> {
    // Get credentials based on authentication type
    let (access_token, password) = match account.imap_authentication_type {
        AuthenticationType::OAuth2 => {
            let provider_id = account.oauth2_provider_id.as_ref()
                .ok_or_else(|| "No OAuth2 provider configured".to_string())?;

            let refresh_token = secrets::read_password(&oauth2_refresh_token_key(&account.id))
                .ok_or_else(|| "No refresh token available".to_string())?;

            let token_result = oauth2::refresh_oauth2_token(provider_id, &refresh_token)
                .map_err(|e| format!("Failed to refresh token: {}", e))?;

            // Store new access token
            secrets::write_password(
                &oauth2_access_token_key(&account.id),
                &token_result.access_token,
            );

            (Some(token_result.access_token), None)
        }
        AuthenticationType::Plain => {
            let pass = secrets::read_password(&imap_password_key(&account.id))
                .ok_or_else(|| "No password available".to_string())?;
            (None, Some(pass))
        }
        AuthenticationType::NoAuth => {
            return Err("NoAuth authentication not supported".to_string());
        }
    };

    // Use the low-level connection function
    connect_and_authenticate(
        account,
        password.as_deref(),
        access_token.as_deref(),
    )
}
