// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "rust/cxx.h"

// C++ bridge functions for Rust to access OAuth providers

/// Get the number of registered OAuth providers
int oauth_provider_count();

/// Get provider ID at index
rust::String oauth_provider_id(int index);

/// Get provider name at index
rust::String oauth_provider_name(int index);

/// Get provider client ID at index
rust::String oauth_provider_client_id(int index);

/// Get provider auth endpoint at index
rust::String oauth_provider_auth_endpoint(int index);

/// Get provider token endpoint at index
rust::String oauth_provider_token_endpoint(int index);

/// Get provider scope at index
rust::String oauth_provider_scope(int index);

/// Find provider by ID, returns index or -1 if not found
int oauth_provider_find_by_id(rust::Str id);

/// Find provider by domain, returns index or -1 if not found
int oauth_provider_find_by_domain(rust::Str domain);

/// Find provider by email, returns index or -1 if not found
int oauth_provider_find_by_email(rust::Str email);

/// Check if provider at index is valid (has client ID configured)
bool oauth_provider_is_valid(int index);
