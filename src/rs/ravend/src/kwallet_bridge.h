// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "rust/cxx.h"

bool kwallet_open();
void kwallet_close();
rust::String kwallet_read_password(rust::Str key);
bool kwallet_write_password(rust::Str key, rust::Str password);
bool kwallet_remove_entry(rust::Str key);
