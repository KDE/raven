// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! IMAP email synchronization module

mod actions;
mod connection;
mod parser;
mod worker;

pub use actions::{perform_flag_action, move_to_trash, fetch_attachment_from_imap, ActionResult, FlagAction};
pub use connection::connect_with_secrets;
pub use worker::ImapWorker;
