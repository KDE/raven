// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <QObject>

namespace MessageComposer {
    class TextPart;
    class InfoPart;
    class AttachmentModel;
    class Composer;
}


/// Simple helper exposing the MessageComposer::Composer to QML
class ComposerHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MessageComposer::TextPart *textPart READ textPart CONSTANT)
    Q_PROPERTY(MessageComposer::InfoPart *infoPart READ infoPart CONSTANT)
    Q_PROPERTY(MessageComposer::AttachmentModel *attachementModel READ attachementModel CONSTANT)

public:
    explicit ComposerHelper(QObject *parent = nullptr);
    ~ComposerHelper();
    
    MessageComposer::TextPart *textPart() const;
    MessageComposer::InfoPart *infoPart() const;
    MessageComposer::AttachmentModel *attachementModel() const;
    
    Q_INVOKABLE void send();

private:
    MessageComposer::Composer *m_composer;
    MessageComposer::AttachmentModel *m_attachementModel;
};
