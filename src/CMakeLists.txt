# Copyright 2021 Carl Schwan <carl@carlschwan.eu>
# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

set(raven_SRCS
    main.cpp
    raven.cpp
    abouttype.cpp
    
    mailmanager.cpp
    mailmanager.h
    mailmodel.cpp
    mailmodel.h

    messagestatus.h
    messagestatus.cpp

    crypto.cpp
    
    accounts/ispdb/autoconfigkolabfreebusy.cpp
    accounts/ispdb/autoconfigkolabldap.cpp
    accounts/ispdb/autoconfigkolabmail.cpp
    accounts/ispdb/ispdb.cpp
    
    accounts/setup/configfile.cpp
    accounts/setup/identity.cpp
    accounts/setup/key.cpp
    accounts/setup/ldap.cpp
    accounts/setup/resource.cpp
    accounts/setup/restoreldapsettingsjob.cpp
    accounts/setup/setupautoconfigkolabfreebusy.cpp
    accounts/setup/setupautoconfigkolabldap.cpp
    accounts/setup/setupautoconfigkolabmail.cpp
    accounts/setup/setupispdb.cpp
    accounts/setup/setupmanager.cpp
    accounts/setup/setupobject.cpp
    accounts/setup/transport.cpp
    
    accounts/mailaccounts.cpp
    accounts/newaccount.cpp
    
    mimetreeparser/messagepart.cpp
    mimetreeparser/bodypartformatter.cpp
    mimetreeparser/bodypartformatter_impl.cpp
    mimetreeparser/bodypartformatterbasefactory.cpp
    mimetreeparser/bodypartformatterbasefactory.cpp
    mimetreeparser/cryptohelper.cpp
    mimetreeparser/messagepart.cpp
    mimetreeparser/mimetreeparser_debug.cpp
    mimetreeparser/objecttreeparser.cpp
    mimetreeparser/utils.cpp
    
    mime/attachmentmodel.cpp
    mime/htmlutils.cpp
    mime/mailcrypto.cpp
    mime/mailtemplates.cpp
    mime/messageparser.cpp
    mime/partmodel.cpp
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
    KF5::I18n
    KF5::CoreAddons
    KF5::AkonadiCore
    KF5::AkonadiContact
    KF5::AkonadiAgentBase
    KF5::AkonadiXml
    KF5::AkonadiMime
    KF5::KCMUtils
    KF5::MailTransport
    KF5::MailTransportAkonadi
    KF5::IdentityManagement
    KF5::ConfigWidgets
    KF5::Contacts
    KF5::ItemModels
    KF5::MailCommon 
    KF5::I18n
    KF5::Ldap
    KF5::KIOCore
    KF5::Mime
    KF5::Wallet
)
target_include_directories(raven PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS raven ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
