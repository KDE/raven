// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! OAuth2 provider configuration loaded from embedded JSON
//!
//! This module provides access to OAuth provider configurations without
//! requiring FFI bridge to C++ code. Both C++ and Rust read from the same
//! JSON configuration file.

use serde::{Deserialize, Serialize};
use std::sync::OnceLock;

/// OAuth2 provider configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OAuthProvider {
    pub id: String,
    pub name: String,
    pub client_id: String,
    pub auth_endpoint: String,
    pub token_endpoint: String,
    pub scope: String,
    pub domains: Vec<String>,
}

impl OAuthProvider {
    /// Check if this provider has valid configuration (has client ID)
    pub fn is_valid(&self) -> bool {
        !self.client_id.is_empty()
            && !self.auth_endpoint.is_empty()
            && !self.token_endpoint.is_empty()
    }
}

#[derive(Debug, Deserialize)]
struct ProvidersConfig {
    providers: Vec<OAuthProvider>,
}

/// Global provider registry (lazy-initialized)
static PROVIDERS: OnceLock<Vec<OAuthProvider>> = OnceLock::new();

/// Load providers from embedded JSON
fn load_providers() -> Vec<OAuthProvider> {
    const PROVIDERS_JSON: &str = include_str!("../../../oauth_providers.json");

    let config: ProvidersConfig = serde_json::from_str(PROVIDERS_JSON)
        .expect("Failed to parse OAuth providers JSON - this is a build-time error");

    config.providers
}

/// Get all providers (lazy-loaded on first access)
fn get_providers() -> &'static [OAuthProvider] {
    PROVIDERS.get_or_init(load_providers)
}

/// Find a provider by ID
pub fn find_provider_by_id(id: &str) -> Option<OAuthProvider> {
    get_providers()
        .iter()
        .find(|p| p.id == id)
        .cloned()
}

/// Find a provider by email address (extracts domain and matches)
pub fn find_provider_by_email(email: &str) -> Option<OAuthProvider> {
    let domain = email.split('@').nth(1)?.to_lowercase();
    find_provider_by_domain(&domain)
}

/// Find a provider by email domain
pub fn find_provider_by_domain(domain: &str) -> Option<OAuthProvider> {
    let domain_lower = domain.to_lowercase();
    get_providers()
        .iter()
        .find(|p| {
            p.domains.iter().any(|d| {
                let d_lower = d.to_lowercase();
                domain_lower == d_lower || domain_lower.ends_with(&format!(".{}", d_lower))
            })
        })
        .cloned()
}
