# Copyright 2021 Carl Schwan <carl@carlschwan.eu>
# Copyright 2022 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

set(raven_SRCS
    main.cpp
    raven.cpp
    mailmodel.cpp
    messagewrapper.cpp
    composerhelper.cpp
    identitymodel.cpp
    messageviewer/viewer.cpp
)

qt_add_resources(RESOURCES resources.qrc)

add_executable(raven ${raven_SRCS} ${RESOURCES})
target_link_libraries(raven
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
    Qt5::Svg
    KF5::I18n
    KF5::ItemModels
    KF5::Mime
    KF5::KIOCore
    KF5::MailCommon
    KF5::MailTransport
    KF5::MailTransportAkonadi
    KF5::AkonadiMime
    KF5::AkonadiCore
    KF5::AkonadiAgentBase
    KF5::AkonadiWidgets
    KF5::AkonadiXml
    KF5::MessageViewer
    KF5::MessageList

    KF5::MimeTreeParser
)
target_include_directories(raven PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS raven ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
