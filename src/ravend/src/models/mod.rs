// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! Data models for the Raven mail daemon

mod account;
mod file;
mod folder;
mod message;
mod thread;

pub use account::{Account, AuthenticationType, ConnectionType};
pub use file::{Attachment, ParsedAttachment, IMMEDIATE_DOWNLOAD_THRESHOLD, default_filename};
pub use folder::{Folder, FolderRole};
pub use message::{Message, MessageBody, MessageData};
pub use thread::{Thread, ThreadFolder, ThreadReference};
