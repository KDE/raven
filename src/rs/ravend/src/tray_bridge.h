// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "rust/cxx.h"

/// Initialize the tray icon
/// Must be called once at startup
/// Returns true if successful
bool tray_init();

/// Clean up the tray icon
void tray_cleanup();

/// Check if quit was requested via the tray menu
/// Returns true if the user clicked "Quit"
bool tray_is_quit_requested();

/// Process pending Qt events (call periodically from async context)
/// This is required for the tray icon to function properly
void tray_process_events();
