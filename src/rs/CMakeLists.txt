# Copyright 2025 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_custom_target(ravend
    SOURCES
    rs/ravend/Cargo.toml
    rs/ravend/build.rs
    rs/ravend/src/lib.rs
    rs/ravend/src/adblock.rs
    rs/ravend/src/domainresolver.rs
    rs/ravend/src/logging.rs)

corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rs/ravend/Cargo.toml)
target_compile_definitions(angelfish PRIVATE -DBUILD_ADBLOCK)

string(SHA1 cargo_path_hash ${CMAKE_CURRENT_SOURCE_DIR}/rs/adblock/Cargo.toml)
string(SUBSTRING "${cargo_path_hash}" 0 5 cargo_path_hash)
set(corrosion_build_dir "adblock_${cargo_path_hash}")

target_include_directories(ravend PRIVATE
    ${CMAKE_BINARY_DIR}/cargo/${corrosion_build_dir}/${Rust_CARGO_TARGET}/cxxbridge/ravend/src/)
