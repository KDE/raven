// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progresscollectors.h"

IMAPProgress::~IMAPProgress()
{
}

void IMAPProgress::bodyProgress(IMAPSession * session, unsigned int current, unsigned int maximum) {
    //        cout << "Progress: " << current << "\n";
}

void IMAPProgress::itemsProgress(IMAPSession * session, unsigned int current, unsigned int maximum) {
    //        cout << "Progress on Item: " << current << "\n";
}

SMTPProgress::~SMTPProgress()
{
}

void SMTPProgress::bodyProgress(IMAPSession * session, unsigned int current, unsigned int maximum) {
    //        cout << "Progress: " << current << "\n";
}

