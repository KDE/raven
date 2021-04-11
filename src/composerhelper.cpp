// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "composerhelper.h"

#include <MessageComposer/Composer>
#include <MessageComposer/AttachmentModel>
#include <MessageComposer/TextPart>
#include <MessageComposer/InfoPart>

#include <QDebug>

using namespace MessageComposer;

ComposerHelper::ComposerHelper(QObject* parent)
    : QObject(parent)
    , m_composer(new Composer(this))
    , m_attachementModel(new AttachmentModel(this))
{
}

ComposerHelper::~ComposerHelper()
{
}


TextPart *ComposerHelper::textPart() const
{
    return m_composer->textPart();
}

InfoPart *ComposerHelper::infoPart() const
{
    return m_composer->infoPart();
}

AttachmentModel *ComposerHelper::attachementModel() const
{
    return m_attachementModel;
}

void ComposerHelper::send()
{
    m_composer->start();
    connect(m_composer, &Composer::result,
            this, [this](KJob *) {
                qDebug() << "Finished";
                qDebug() << m_composer->resultMessages(); 
            });
}
