// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! System tray integration using KStatusNotifierItem via cxx bridge
//!
//! This module provides a system tray icon for the Raven daemon with:
//! - Left click: Opens/raises the Raven client app
//! - Right click: Shows a menu with Quit option

use log::{error, info};

#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("tray_bridge.h");

        fn tray_init() -> bool;
        fn tray_cleanup();
        fn tray_is_quit_requested() -> bool;
        fn tray_process_events();
    }
}

/// System tray manager
pub struct TrayManager {
    _private: (),
}

impl TrayManager {
    /// Initialize the system tray icon
    pub fn new() -> Option<Self> {
        if ffi::tray_init() {
            info!("System tray icon initialized");
            Some(Self { _private: () })
        } else {
            error!("Failed to initialize system tray icon");
            None
        }
    }

    /// Check if the user has requested to quit via the tray menu
    pub fn is_quit_requested(&self) -> bool {
        ffi::tray_is_quit_requested()
    }

    /// Process pending Qt events
    /// Call this periodically to keep the tray icon responsive
    pub fn process_events(&self) {
        ffi::tray_process_events();
    }
}

impl Drop for TrayManager {
    fn drop(&mut self) {
        ffi::tray_cleanup();
        info!("System tray icon cleaned up");
    }
}
