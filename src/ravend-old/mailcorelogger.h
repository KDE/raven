// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <MailCore/MailCore.h>

class MailCoreLogger : public mailcore::ConnectionLogger
{
public:
    virtual ~MailCoreLogger() = default;
    void log(void *sender, mailcore::ConnectionLogType logType, mailcore::Data *buffer) override;
};
