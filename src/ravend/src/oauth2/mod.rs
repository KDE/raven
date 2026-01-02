// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! OAuth2 authentication module for email providers
//!
//! This module handles OAuth2 authentication flows, including:
//! - Provider-agnostic token refresh
//! - Token storage
//!
//! ## Supported Providers
//!
//! - Gmail (Google)
//! - Outlook (Microsoft)
//! - Yahoo
//!
//! Provider configurations are sourced from the shared liboauthproviders C++ library
//! to avoid duplication between the Qt frontend and Rust backend.
//!
//! ## XOAUTH2 for IMAP/SMTP
//!
//! Email providers use XOAUTH2 authentication mechanism for IMAP/SMTP.
//! The authentication string format is:
//! ```text
//! user={email}\x01auth=Bearer {access_token}\x01\x01
//! ```

mod gmail;
mod provider_bridge;

pub use gmail::RefreshedToken;
pub use provider_bridge::{find_provider_by_id, find_provider_by_email};

use anyhow::{Context, Result};
use log::{debug, info};

/// Refresh an OAuth2 access token using a refresh token
///
/// This function looks up the provider configuration and uses the appropriate
/// token endpoint to refresh the access token.
///
/// # Arguments
/// * `provider_id` - The OAuth provider ID (e.g., "gmail", "outlook", "yahoo")
/// * `refresh_token` - The refresh token to use
///
/// # Returns
/// A `RefreshedToken` containing the new access token and expiry
pub fn refresh_oauth2_token(
    provider_id: &str,
    refresh_token: &str,
) -> Result<RefreshedToken> {
    let provider = find_provider_by_id(provider_id)
        .ok_or_else(|| anyhow::anyhow!("Unknown OAuth provider: {}", provider_id))?;

    if !provider.is_valid() {
        return Err(anyhow::anyhow!(
            "OAuth provider {} is not configured (missing client ID)",
            provider.name
        ));
    }

    debug!("Refreshing OAuth2 token for provider: {}", provider.name);

    let client = reqwest::blocking::Client::new();

    // Build refresh token request
    // Note: We don't send client_secret when using PKCE
    let params = [
        ("client_id", provider.client_id.as_str()),
        ("refresh_token", refresh_token),
        ("grant_type", "refresh_token"),
    ];

    let response = client
        .post(&provider.token_endpoint)
        .form(&params)
        .send()
        .context("Failed to send token refresh request")?;

    if !response.status().is_success() {
        let status = response.status();
        let body = response.text().unwrap_or_default();
        return Err(anyhow::anyhow!(
            "Token refresh failed with status {}: {}",
            status,
            body
        ));
    }

    let token_response: serde_json::Value = response
        .json()
        .context("Failed to parse token refresh response")?;

    let access_token = token_response["access_token"]
        .as_str()
        .ok_or_else(|| anyhow::anyhow!("No access_token in response"))?
        .to_string();

    let expires_in = token_response["expires_in"].as_u64();

    info!("Successfully refreshed OAuth2 token for provider: {}", provider.name);

    Ok(RefreshedToken {
        access_token,
        expires_in,
    })
}
