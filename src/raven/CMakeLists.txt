# Copyright 2023 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set(raven_SRCS
    abouttype.cpp
    main.cpp
    raven.cpp

    models/account.cpp
    models/file.cpp
    models/folder.cpp
    models/message.cpp
    models/messagecontact.cpp
    models/thread.cpp

    accountmodel.cpp
    daemonstatus.cpp
    dbmanager.cpp
    dbwatcher.cpp
    threadviewbridge.cpp
    utils.cpp

    modelviews/attachmentmodel.cpp
    modelviews/mailboxmodel.cpp
    modelviews/maillistmodel.cpp
    modelviews/threadviewmodel.cpp

    accounts/ispdb.cpp
    accounts/newaccountmanager.cpp
    accounts/oauthmanager.cpp

    oauthprovider.cpp
)

# Generate D-Bus interface from shared XML definition
qt_add_dbus_interface(raven_SRCS
    ${CMAKE_SOURCE_DIR}/src/dbus/org.kde.raven.daemon.xml
    ravendaemoninterface
)

# kconfig_add_kcfg_files(raven_SRCS ravensettings.kcfgc GENERATE_MOC)

add_executable(raven ${raven_SRCS})

# Add thread view web resources
qt_add_resources(raven "threadview"
    PREFIX "/threadview"
    BASE "resources/threadview"
    FILES
        resources/threadview/index.html
        resources/threadview/threadview.css
        resources/threadview/threadview.js
)

# Embed OAuth providers JSON as Qt resource
qt_add_resources(raven "oauth_providers"
    PREFIX "/oauth"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/.."
    FILES
        ../oauth_providers.json
)

ecm_add_qml_module(raven URI org.kde.raven QML_FILES
    qml/accounts/AddAccountPage.qml
    qml/accounts/FormPasswordInputDelegate.qml
    qml/accounts/FormTextInputDelegate.qml
    qml/accounts/ManualAccountForm.qml

    qml/mailboxselector/MailBoxList.qml
    qml/mailboxselector/MailBoxListSidebar.qml

    qml/AboutPage.qml
    qml/FolderView.qml
    qml/MailDelegate.qml
    qml/Main.qml
    qml/SettingsPage.qml
    qml/ThreadViewer.qml
)

target_link_libraries(raven PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    Qt::Svg
    Qt::Xml
    Qt::Sql
    Qt::Network
    Qt::DBus
    Qt::WebChannel
    Qt::WebEngineQuick
    KF6::I18n
    KF6::CoreAddons
    KF6::KIOCore
    KF6::Crash
    KF6::ConfigCore
    KF6::DBusAddons

    PkgConfig::UUID
)

target_include_directories(raven PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/accounts
    ${CMAKE_CURRENT_SOURCE_DIR}/modelviews
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)
install(TARGETS raven ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# Install notification configuration
install(FILES raven.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
