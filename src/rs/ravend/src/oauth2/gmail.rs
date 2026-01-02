// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Gmail-specific OAuth2 types

/// OAuth2 token response from refresh
#[derive(Debug)]
pub struct RefreshedToken {
    pub access_token: String,
    pub expires_in: Option<u64>,
}
