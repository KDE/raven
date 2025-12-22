# Copyright 2023 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set(ravend_SRCS
    dbmanager.cpp
    main.cpp
    accountworker.cpp
    mailprocessor.cpp
    progresscollectors.cpp
    workermanager.cpp
    mailcorelogger.cpp
    dbuslistener.cpp
)

add_executable(ravend ${ravend_SRCS})

target_link_libraries(ravend
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    Qt::Svg
    Qt::Xml
    Qt::Sql
    Qt::DBus
    KF5::I18n
    KF5::CoreAddons
    KF5::KIOCore
    KF5::Wallet
    KF5::Mime
    KF5::DBusAddons

    ${LIBXML2_LIBRARIES}
    OpenSSL::Crypto
    ICU::i18n
    ICU::uc
    PkgConfig::GLIB
    PkgConfig::UUID
    LibLZMA::LibLZMA
    ZLIB::ZLIB
    ${CTEMPLATE_LIB}
    ${ETPAN_LIB}
    ${TIDY_LIB}
    MailCore
    
    libraven
)

target_include_directories(ravend PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS ravend ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

