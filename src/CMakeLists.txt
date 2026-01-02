# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(liboauthproviders)
add_subdirectory(raven)

# Rust daemon (requires Corrosion)
if(Corrosion_FOUND)
    add_subdirectory(rs)
endif()
