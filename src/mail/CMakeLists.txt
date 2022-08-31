# SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(raven_mail_plugin URI "org.kde.kalendar.mail" VERSION 1.0)

target_sources(raven_mail_plugin PUBLIC
    mailplugin.cpp
    mailplugin.h
    mailmanager.cpp
    mailmanager.h
    mailmodel.cpp
    mailmodel.h

    messagestatus.h
    messagestatus.cpp

    crypto.cpp
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

ecm_target_qml_sources(raven_mail_plugin SOURCES
    qml/MailSidebar.qml
    qml/FolderView.qml
    qml/MailViewer.qml
    qml/ConversationViewer.qml
)
ecm_target_qml_sources(raven_mail_plugin
    PRIVATE PATH private SOURCES
    qml/private/AttachmentDelegate.qml
)

ecm_target_qml_sources(raven_mail_plugin
    PRIVATE PATH mailpartview SOURCES
    qml/mailpartview/HtmlPart.qml
    qml/mailpartview/ICalPart.qml
    qml/mailpartview/MailPart.qml
    qml/mailpartview/MailPartModel.qml
    qml/mailpartview/MailPartView.qml
    qml/mailpartview/TextPart.qml
)

target_link_libraries(raven_mail_plugin
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::QuickControls2
    Qt::Svg
    KF5::I18n
    KF5::CoreAddons
    KF5::AkonadiCore
    KF5::AkonadiContact
    KF5::AkonadiAgentBase
    KF5::AkonadiXml
    KF5::ConfigWidgets
    KF5::Contacts
    KF5::ItemModels
    KF5::MailCommon 
    KF5::AkonadiMime
    KF5::I18n
)

ecm_finalize_qml_module(raven_mail_plugin
    DESTINATION ${KDE_INSTALL_QMLDIR}
    BUILD_SHARED_LIBS OFF)
