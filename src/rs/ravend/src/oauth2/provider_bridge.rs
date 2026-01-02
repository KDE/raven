// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! CXX bridge to liboauthproviders C++ library
//!
//! This module provides access to OAuth provider configurations defined in the
//! shared liboauthproviders library, avoiding duplication between C++ and Rust.

#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("oauthprovider_bridge.h");

        /// Get the number of registered OAuth providers
        fn oauth_provider_count() -> i32;

        /// Get provider ID at index
        fn oauth_provider_id(index: i32) -> String;

        /// Get provider name at index
        fn oauth_provider_name(index: i32) -> String;

        /// Get provider client ID at index
        fn oauth_provider_client_id(index: i32) -> String;

        /// Get provider token endpoint at index
        fn oauth_provider_token_endpoint(index: i32) -> String;

        /// Find provider by ID, returns index or -1 if not found
        fn oauth_provider_find_by_id(id: &str) -> i32;

        /// Find provider by email, returns index or -1 if not found
        fn oauth_provider_find_by_email(email: &str) -> i32;
    }
}

/// OAuth2 provider information retrieved from C++ liboauthproviders
#[derive(Debug, Clone)]
pub struct OAuthProvider {
    pub id: String,
    pub name: String,
    pub client_id: String,
    pub token_endpoint: String,
}

impl OAuthProvider {
    /// Check if this provider has valid configuration (has client ID)
    pub fn is_valid(&self) -> bool {
        !self.client_id.is_empty()
    }
}

/// Get provider by index from C++ registry
fn get_provider_at(index: i32) -> Option<OAuthProvider> {
    if index < 0 || index >= ffi::oauth_provider_count() {
        return None;
    }

    Some(OAuthProvider {
        id: ffi::oauth_provider_id(index),
        name: ffi::oauth_provider_name(index),
        client_id: ffi::oauth_provider_client_id(index),
        token_endpoint: ffi::oauth_provider_token_endpoint(index),
    })
}

/// Find a provider by ID
pub fn find_provider_by_id(id: &str) -> Option<OAuthProvider> {
    let index = ffi::oauth_provider_find_by_id(id);
    get_provider_at(index)
}

/// Find a provider by email address (extracts domain and matches)
pub fn find_provider_by_email(email: &str) -> Option<OAuthProvider> {
    let index = ffi::oauth_provider_find_by_email(email);
    get_provider_at(index)
}

