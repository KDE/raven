# Copyright 2023 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set(libraven_SRCS
    models/account.cpp
    models/file.cpp
    models/folder.cpp
    models/label.cpp
    models/message.cpp
    models/messagecontact.cpp
    models/thread.cpp
    accountmodel.cpp
    constants.h
    utils.cpp
)

add_library(libraven STATIC ${libraven_SRCS})
target_link_libraries(libraven
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    Qt::Svg
    Qt::Xml
    Qt::Sql
    KF6::I18n
    KF6::CoreAddons
    KF6::KIOCore
    KF6::Wallet
    KF6::ConfigCore
    KPim6::Mime

    # ${LIBXML2_LIBRARIES}
    # OpenSSL::Crypto
    # ICU::i18n
    # ICU::uc
    # PkgConfig::GLIB
    PkgConfig::UUID
    # LibLZMA::LibLZMA
    # ZLIB::ZLIB
    # ${CTEMPLATE_LIB}
    # ${ETPAN_LIB}
    # ${TIDY_LIB}
    # MailCore
)

