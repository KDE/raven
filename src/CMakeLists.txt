# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(raven)

# Rust daemon (requires Corrosion)
# Import the Rust crate using Corrosion
corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ravend/Cargo.toml)

# Install the ravend binary
install(PROGRAMS $<TARGET_FILE:ravend> DESTINATION ${KDE_INSTALL_BINDIR})
