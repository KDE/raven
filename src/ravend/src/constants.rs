// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Polling interval (seconds) for email sync when IDLE is not supported
pub const POLL_INTERVAL_SECS: u64 = 300;

/// Nice value for worker threads (higher = lower priority)
pub const WORKER_NICE_VALUE: i32 = 10;

/// Maximum messages to fetch per folder during sync
pub const MAX_MESSAGES_PER_BATCH: u32 = 100;

/// Time between IMAP IDLE checks (seconds)
pub const IDLE_CHECK_INTERVAL_SECS: u64 = 5;

/// IMAP IDLE timeout in seconds before attempting reconnection
pub const IDLE_TIMEOUT_SECS: u64 = 25 * 60;

/// Service name for keyring/secret storage
pub const SERVICE_NAME: &str = "raven";
