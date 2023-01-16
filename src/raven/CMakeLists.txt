# Copyright 2023 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set(raven_SRCS
    abouttype.cpp
    dbmanager.cpp
    dbwatcher.cpp
    main.cpp
    raven.cpp

    modelviews/mailboxmodel.cpp
    modelviews/maillistmodel.cpp

    accounts/ispdb.cpp
    accounts/newaccountmanager.cpp
)

qt_add_resources(RESOURCES resources.qrc)

add_executable(raven ${raven_SRCS} ${RESOURCES})

target_link_libraries(raven
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    Qt::Svg
    Qt::Xml
    Qt::Sql
    KF5::I18n
    KF5::CoreAddons
    KF5::KIOCore
    KF5::Wallet
    KF5::Mime

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

target_include_directories(raven PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS raven ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
