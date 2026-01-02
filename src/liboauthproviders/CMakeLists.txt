# SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set(liboauthproviders_SRCS
    oauthprovider.cpp
    # oauthprovider_bridge.cpp is for cxx bridge - not currently used
)

add_library(liboauthproviders STATIC ${liboauthproviders_SRCS})

target_link_libraries(liboauthproviders
    Qt::Core
)

target_include_directories(liboauthproviders PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
