// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <MailCore/MailCore.h>

using namespace mailcore;

class IMAPProgress : public IMAPProgressCallback {
public:
    virtual ~IMAPProgress();
    void bodyProgress(IMAPSession * session, unsigned int current, unsigned int maximum) override;
    void itemsProgress(IMAPSession * session, unsigned int current, unsigned int maximum) override;
};

class SMTPProgress : public SMTPProgressCallback {
public:
    virtual ~SMTPProgress();
    void bodyProgress(IMAPSession * session, unsigned int current, unsigned int maximum);
};
