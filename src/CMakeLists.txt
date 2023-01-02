# Copyright 2021 Carl Schwan <carl@carlschwan.eu>
# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

set(raven_SRCS
    abouttype.cpp
    accountworker.cpp
    constants.h
    dbmanager.cpp
    dbwatcher.cpp
    main.cpp
    raven.cpp
    utils.cpp

    models/account.cpp
    models/folder.cpp
    models/label.cpp
    models/message.cpp
    models/messagecontact.cpp

    modelviews/accountmodel.cpp
    modelviews/mailboxmodel.cpp

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
)

target_include_directories(raven PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS raven ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
