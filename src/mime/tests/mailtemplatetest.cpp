// SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDebug>
#include <QDir>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <functional>

#include "../mailcrypto.h"
#include "../mailtemplates.h"

static KMime::Content *getSubpart(KMime::Content *msg, const QByteArray &mimeType)
{
    for (const auto c : msg->contents()) {
        if (c->contentType(false)->mimeType() == mimeType) {
            return c;
        }
    }
    return nullptr;
}

static QByteArray readMailFromFile(const QString &mailFile)
{
    Q_ASSERT(!QString::fromLatin1(MAIL_DATA_DIR).isEmpty());
    QFile file(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + mailFile);
    file.open(QIODevice::ReadOnly);
    Q_ASSERT(file.isOpen());
    return file.readAll();
}

static KMime::Message::Ptr readMail(const QString &mailFile)
{
    auto msg = KMime::Message::Ptr::create();
    msg->setContent(readMailFromFile(mailFile));
    msg->parse();
    return msg;
}

static QString removeFirstLine(const QString &s)
{
    return s.mid(s.indexOf(QLatin1String("\n")) + 1);
}

static QString normalize(const QString &s)
{
    auto text = s;
    text.replace(QLatin1String(">"), QString());
    text.replace(QLatin1String("\n"), QString());
    text.replace(QLatin1String("="), QString());
    text.replace(QLatin1String(" "), QString());
    return text;
}

static QString unquote(const QString &s)
{
    auto text = s;
    text.replace(QLatin1String("> "), QString());
    return text;
}

class MailTemplateTest : public QObject
{
    Q_OBJECT

    bool validate(KMime::Message::Ptr msg)
    {
        const auto data = msg->encodedContent();
        // IMAP compat: The ASCII NUL character, %x00, MUST NOT be used at any time.
        if (data.contains('\0')) {
            return false;
        }
        return true;
    }

private Q_SLOTS:

    // Ensures we don't crash on garbage
    void testEmpty()
    {
        MailTemplates::reply(KMime::Message::Ptr::create(), [&](const KMime::Message::Ptr &) {});
    }

    void testPlainReply()
    {
        auto msg = readMail(QLatin1String("plaintext.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(normalize(removeFirstLine(QString::fromUtf8(result->body()))), normalize(QString::fromUtf8(msg->body())));
        QCOMPARE(result->to()->addresses(), {{"konqi@example.org"}});
        QCOMPARE(result->subject()->asUnicodeString(), QLatin1String{"RE: A random subject with alternative contenttype"});
    }

    void testHtmlReply()
    {
        auto msg = readMail(QLatin1String("html.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(unquote(removeFirstLine(QString::fromUtf8(result->body()))), QLatin1String("HTML text"));
    }

    void testStripSignatureReply()
    {
        auto msg = readMail(QLatin1String("plaintext-with-signature.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QVERIFY(!result->body().contains("This is a signature"));
    }

    void testStripSignatureHtmlReply()
    {
        auto msg = readMail(QLatin1String("html-with-signature.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QVERIFY(!result->body().contains("This is a signature"));
    }

    // We can't test this because we can't commit a CRLF file due to commit hooks.
    //  void testStripSignatureCrlfReply()
    //  {
    //      auto msg = readMail("crlf-with-signature.mbox");
    //      KMime::Message::Ptr result;
    //      MailTemplates::reply(msg, [&] (const KMime::Message::Ptr &r) {
    //          result = r;
    //      });
    //      QTRY_VERIFY(result);
    //      QVERIFY(!result->body().contains("This is a signature"));
    //  }

    void testStripEncryptedCRLFReply()
    {
        auto msg = readMail(QLatin1String("crlf-encrypted-with-signature.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QVERIFY(!result->body().contains("This is a signature"));
    }

    void testHtml8BitEncodedReply()
    {
        auto msg = readMail(QLatin1String("8bitencoded.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QVERIFY(MailTemplates::plaintextContent(result).contains(QString::fromUtf8("Why Pisa’s Tower")));
    }

    void testMultipartSignedReply()
    {
        auto msg = readMail(QLatin1String("openpgp-signed-mailinglist.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        auto content = removeFirstLine(QString::fromUtf8(result->body()));
        QVERIFY(!content.isEmpty());
        QVERIFY(content.contains(QLatin1String("i noticed a new branch")));
    }

    void testMultipartAlternativeReply()
    {
        auto msg = readMail(QLatin1String("alternative.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        auto content = removeFirstLine(QString::fromUtf8(result->body()));
        QVERIFY(!content.isEmpty());
        QCOMPARE(unquote(content),
                 QLatin1String("If you can see this text it means that your email client couldn't display our newsletter properly.\nPlease visit this link to "
                               "view the newsletter on our website: http://www.gog.com/newsletter/\n"));
    }

    void testAttachmentReply()
    {
        auto msg = readMail(QLatin1String("plaintextattachment.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        auto content = removeFirstLine(QString::fromUtf8(result->body()));
        QVERIFY(!content.isEmpty());
        QCOMPARE(unquote(content), QLatin1String("sdlkjsdjf"));
    }

    void testMultiRecipientReply()
    {
        auto msg = readMail(QLatin1String("multirecipients.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        auto content = removeFirstLine(QString::fromUtf8(result->body()));
        QVERIFY(!content.isEmpty());
        QCOMPARE(unquote(content), QLatin1String("test"));
        QCOMPARE(result->to()->addresses(), {{"from@example.org"}});
        auto l = QVector<QByteArray>{{"to1@example.org"}, {"to2@example.org"}, {"cc1@example.org"}, {"cc2@example.org"}};
        QCOMPARE(result->cc()->addresses(), l);
    }

    void testMultiRecipientReplyFilteringMe()
    {
        KMime::Types::AddrSpecList me;
        KMime::Types::Mailbox mb;
        mb.setAddress("to1@example.org");
        me << mb.addrSpec();

        auto msg = readMail(QLatin1String("multirecipients.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(
            msg,
            [&](const KMime::Message::Ptr &r) {
                result = r;
            },
            me);
        QTRY_VERIFY(result);
        QCOMPARE(result->to()->addresses(), {{"from@example.org"}});
        auto l = QVector<QByteArray>{{"to2@example.org"}, {"cc1@example.org"}, {"cc2@example.org"}};
        QCOMPARE(result->cc()->addresses(), l);
    }

    void testMultiRecipientReplyOwnMessage()
    {
        KMime::Types::AddrSpecList me;
        KMime::Types::Mailbox mb;
        mb.setAddress("from@example.org");
        me << mb.addrSpec();

        auto msg = readMail(QLatin1String("multirecipients.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(
            msg,
            [&](const KMime::Message::Ptr &r) {
                result = r;
            },
            me);
        QTRY_VERIFY(result);

        auto to = QVector<QByteArray>{{"to1@example.org"}, {"to2@example.org"}};
        QCOMPARE(result->to()->addresses(), to);
        auto cc = QVector<QByteArray>{{"cc1@example.org"}, {"cc2@example.org"}};
        QCOMPARE(result->cc()->addresses(), cc);
    }

    void testReplyList()
    {
        KMime::Types::AddrSpecList me;
        KMime::Types::Mailbox mb;
        mb.setAddress("me@example.org");
        me << mb.addrSpec();

        auto msg = readMail(QLatin1String("listmessage.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(
            msg,
            [&](const KMime::Message::Ptr &r) {
                result = r;
            },
            me);
        QTRY_VERIFY(result);

        auto to = QVector<QByteArray>{{"list@example.org"}};
        QCOMPARE(result->to()->addresses(), to);
        auto cc = QVector<QByteArray>{{"to@example.org"}, {"cc1@example.org"}};
        QCOMPARE(result->cc()->addresses(), cc);
    }

    void testForwardAsAttachment()
    {
        auto msg = readMail(QString::fromUtf8("plaintext.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::forward(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(result->subject(false)->asUnicodeString(), QLatin1String{"FW: A random subject with alternative contenttype"});
        QCOMPARE(result->to()->addresses(), {});
        QCOMPARE(result->cc()->addresses(), {});
        QCOMPARE(result->references()->identifiers(), {"1505824.VT1nqpAGu0@vkpc5"});
        QCOMPARE(result->inReplyTo()->identifiers(), {});

        auto attachments = result->attachments();
        QCOMPARE(attachments.size(), 1);
        auto attachment = attachments[0];
        QCOMPARE(attachment->contentDisposition(false)->disposition(), KMime::Headers::CDinline);
        QCOMPARE(attachment->contentDisposition(false)->filename(), QLatin1String{"A random subject with alternative contenttype.eml"});
        QVERIFY(attachment->bodyIsMessage());

        attachment->parse();
        auto origMsg = attachment->bodyAsMessage();
        QCOMPARE(origMsg->subject(false)->asUnicodeString(), QLatin1String{"A random subject with alternative contenttype"});
    }

    void testEncryptedForwardAsAttachment()
    {
        auto msg = readMail(QLatin1String("openpgp-encrypted.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::forward(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(result->subject(false)->asUnicodeString(), QLatin1String{"FW: OpenPGP encrypted"});
        QCOMPARE(result->to()->addresses(), {});
        QCOMPARE(result->cc()->addresses(), {});
        QCOMPARE(result->references()->identifiers(), {"1505824.VT2nqpAGu0@vkpc5"});
        QCOMPARE(result->inReplyTo()->identifiers(), {});

        auto attachments = result->attachments();
        QCOMPARE(attachments.size(), 1);
        auto attachment = attachments[0];
        QCOMPARE(attachment->contentDisposition(false)->disposition(), KMime::Headers::CDinline);
        QCOMPARE(attachment->contentDisposition(false)->filename(), QLatin1String{"OpenPGP encrypted.eml"});
        QVERIFY(attachment->bodyIsMessage());

        attachment->parse();
        auto origMsg = attachment->bodyAsMessage();
        QCOMPARE(origMsg->subject(false)->asUnicodeString(), QLatin1String{"OpenPGP encrypted"});
    }

    void testEncryptedWithAttachmentsForwardAsAttachment()
    {
        auto msg = readMail(QLatin1String("openpgp-encrypted-two-attachments.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::forward(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(result->subject(false)->asUnicodeString(), QLatin1String{"FW: OpenPGP encrypted with 2 text attachments"});
        QCOMPARE(result->to()->addresses(), {});
        QCOMPARE(result->cc()->addresses(), {});
        QCOMPARE(result->references()->identifiers(), {"1505824.VT0nqpAGu0@vkpc5"});
        QCOMPARE(result->inReplyTo()->identifiers(), {});

        auto attachments = result->attachments();
        QCOMPARE(attachments.size(), 1);
        auto attachment = attachments[0];
        QCOMPARE(attachment->contentDisposition(false)->disposition(), KMime::Headers::CDinline);
        QCOMPARE(attachment->contentDisposition(false)->filename(), QLatin1String{"OpenPGP encrypted with 2 text attachments.eml"});
        QVERIFY(attachment->bodyIsMessage());

        attachment->parse();
        auto origMsg = attachment->bodyAsMessage();
        QCOMPARE(origMsg->subject(false)->asUnicodeString(), QLatin1String{"OpenPGP encrypted with 2 text attachments"});

        auto attattachments = origMsg->attachments();
        QCOMPARE(attattachments.size(), 2);
        QCOMPARE(attattachments[0]->contentDisposition(false)->filename(), QLatin1String{"attachment1.txt"});
        QCOMPARE(attattachments[1]->contentDisposition(false)->filename(), QLatin1String{"attachment2.txt"});
    }

    void testForwardAlreadyForwarded()
    {
        auto msg = readMail(QLatin1String("cid-links-forwarded-inline.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::forward(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(result->subject(false)->asUnicodeString(), QLatin1String{"FW: Html Hello (inlin)"});
        QCOMPARE(result->to()->addresses(), {});
        QCOMPARE(result->cc()->addresses(), {});
        const QVector references{QByteArray{"a1777ec781546ccc5dcd4918a5e4e03d@info"}, QByteArray{"46b164308eb6056361c866932a740a3c@info"}};
        QCOMPARE(result->references()->identifiers(), references);
        QCOMPARE(result->inReplyTo()->identifiers(), {});
    }

    void testCreatePlainMail()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments;

        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, false, attachments);

        QVERIFY(result);
        QVERIFY(validate(result));
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QCOMPARE(result->body(), body.toUtf8());
        QVERIFY(result->date(false)->dateTime().isValid());
        QVERIFY(result->contentType()->isMimeType("text/plain"));
        QVERIFY(result->messageID(false) && !result->messageID(false)->isEmpty());
    }

    void testCreateHtmlMail()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments;

        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, true, attachments);

        QVERIFY(result);
        QVERIFY(validate(result));
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QVERIFY(result->date(false)->dateTime().isValid());
        QVERIFY(result->contentType()->isMimeType("multipart/alternative"));
        const auto contents = result->contents();
        // 1 Plain + 1 Html
        QCOMPARE(contents.size(), 2);
    }

    void testCreatePlainMailWithAttachments()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments = {{QLatin1String("name"), QLatin1String("filename"), "mimetype", true, "inlineAttachment"},
                                         {QLatin1String("name"), QLatin1String("filename"), "mimetype", false, "nonInlineAttachment"}};

        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, false, attachments);

        QVERIFY(result);
        QVERIFY(validate(result));
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QVERIFY(result->contentType()->isMimeType("multipart/mixed"));
        QVERIFY(result->date(false)->dateTime().isValid());
        const auto contents = result->contents();
        // 1 Plain + 2 Attachments
        QCOMPARE(contents.size(), 3);
        auto p = getSubpart(result.data(), "text/plain");
        QVERIFY(p);
    }

    void testCreateHtmlMailWithAttachments()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments = {
            {QLatin1String("name"), QLatin1String("filename"), "mimetype", true, "inlineAttachment"},
            {QLatin1String("name"), QLatin1String("filename"), "mimetype", false, "nonInlineAttachment"},
        };

        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, true, attachments);

        QVERIFY(result);
        QVERIFY(validate(result));
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QVERIFY(result->contentType()->isMimeType("multipart/mixed"));
        QVERIFY(result->date(false)->dateTime().isValid());
        const auto contents = result->contents();
        // 1 alternative + 2 Attachments
        QCOMPARE(contents.size(), 3);
        auto p = getSubpart(result.data(), "multipart/alternative");
        QVERIFY(p);
    }

    void testCreatePlainMailSigned()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments;

        auto keys = Crypto::findKeys({}, true, false);
        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, false, attachments, keys, {}, keys[0]);

        QVERIFY(result);
        QVERIFY(validate(result));
        // qWarning() << "---------------------------------";
        // qWarning().noquote() << result->encodedContent();
        // qWarning() << "---------------------------------";
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QVERIFY(result->date(false)->dateTime().isValid());

        QCOMPARE(result->contentType()->mimeType(), QByteArray{"multipart/signed"});
        QCOMPARE(result->attachments().size(), 1);
        QCOMPARE(result->attachments()[0]->contentDisposition()->filename(), QLatin1String{"0x8F246DE6.asc"});
        QCOMPARE(result->contents().size(), 2);

        auto signedMessage = result->contents()[0];
        QVERIFY(signedMessage->contentType()->isMimeType("multipart/mixed"));
        const auto contents = signedMessage->contents();
        QCOMPARE(contents.size(), 2);
        QCOMPARE(contents[0]->contentType()->mimeType(), QByteArray{"text/plain"});
        QCOMPARE(contents[1]->contentType()->mimeType(), QByteArray{"application/pgp-keys"});
        QCOMPARE(contents[1]->contentDisposition()->filename(), QLatin1String{"0x8F246DE6.asc"});

        auto signature = result->contents()[1];
        QCOMPARE(signature->contentDisposition()->filename(), QLatin1String{"signature.asc"});
        QVERIFY(signature->contentType()->isMimeType("application/pgp-signature"));
    }

    void testCreatePlainMailWithAttachmentsSigned()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};

        KMime::Types::Mailbox from;
        from.fromUnicodeString(QLatin1String("from@example.org"));
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");
        QList<Attachment> attachments = {
            {QLatin1String("name"), QLatin1String("filename1"), "mimetype1", true, "inlineAttachment"},
            {QLatin1String("name"), QLatin1String("filename2"), "mimetype2", false, "nonInlineAttachment"},
        };

        auto signingKeys = Crypto::findKeys({}, true, false);
        auto result = MailTemplates::createMessage({}, to, cc, bcc, from, subject, body, false, attachments, signingKeys, {}, signingKeys[0]);

        QVERIFY(result);
        QVERIFY(validate(result));
        qWarning() << "---------------------------------";
        qWarning().noquote() << result->encodedContent();
        qWarning() << "---------------------------------";
        QCOMPARE(result->subject()->asUnicodeString(), subject);
        QVERIFY(result->date(false)->dateTime().isValid());

        QCOMPARE(result->contentType()->mimeType(), QByteArray{"multipart/signed"});
        QCOMPARE(result->attachments().size(), 3);
        QCOMPARE(result->attachments()[0]->contentDisposition()->filename(), QLatin1String{"filename1"});
        QCOMPARE(result->attachments()[1]->contentDisposition()->filename(), QLatin1String{"filename2"});
        QCOMPARE(result->attachments()[2]->contentDisposition()->filename(), QLatin1String{"0x8F246DE6.asc"});

        QCOMPARE(result->contents().size(), 2);

        auto signedMessage = result->contents()[0];
        QVERIFY(signedMessage->contentType()->isMimeType("multipart/mixed"));
        const auto contents = signedMessage->contents();
        QCOMPARE(contents.size(), 4);
        QCOMPARE(contents[0]->contentType()->mimeType(), QByteArray{"text/plain"});
        QCOMPARE(contents[1]->contentDisposition()->filename(), QLatin1String{"filename1"});
        QCOMPARE(contents[2]->contentDisposition()->filename(), QLatin1String{"filename2"});
        QCOMPARE(contents[3]->contentType()->mimeType(), QByteArray{"application/pgp-keys"});
        QCOMPARE(contents[3]->contentDisposition()->filename(), QLatin1String{"0x8F246DE6.asc"});
    }

    void testCreateIMipMessage()
    {
        QStringList to = {{QLatin1String("to@example.org")}};
        QStringList cc = {{QLatin1String("cc@example.org")}};
        QStringList bcc = {{QLatin1String("bcc@example.org")}};
        QString from = {QLatin1String("from@example.org")};
        QString subject = QLatin1String("subject");
        QString body = QLatin1String("body");

        QString ical = QLatin1String("ical");

        auto result = MailTemplates::createIMipMessage(from, {to, cc, bcc}, subject, body, ical);

        QVERIFY(result);
        QVERIFY(validate(result));
        qWarning() << "---------------------------------";
        qWarning().noquote() << result->encodedContent();
        qWarning() << "---------------------------------";

        QCOMPARE(result->contentType()->mimeType(), QByteArray{"multipart/alternative"});

        QCOMPARE(result->attachments().size(), 0);

        QCOMPARE(result->contents().size(), 2);
        QVERIFY(result->contents()[0]->contentType()->isMimeType("text/plain"));
        QVERIFY(result->contents()[1]->contentType()->isMimeType("text/calendar"));
        QCOMPARE(result->contents()[1]->contentType()->name(), QLatin1String{"event.ics"});
    }

    void testEncryptedWithProtectedHeadersReply()
    {
        KMime::Types::AddrSpecList me;
        KMime::Types::Mailbox mb;
        mb.setAddress("to1@example.org");
        me << mb.addrSpec();

        auto msg = readMail(QLatin1String("openpgp-encrypted-memoryhole2.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::reply(
            msg,
            [&](const KMime::Message::Ptr &r) {
                result = r;
            },
            me);
        QTRY_VERIFY(result);
        QCOMPARE(result->subject()->asUnicodeString(), QLatin1String{"RE: This is the subject"});
        QCOMPARE(result->to()->addresses(), {{"jane@doe.com"}});
        QCOMPARE(result->cc()->addresses(), {{"john@doe.com"}});
        QCOMPARE(result->inReplyTo()->asUnicodeString(), QLatin1String{"<03db3530-0000-0000-95a2-8a148f00000@example.com>"});
        QCOMPARE(result->references()->asUnicodeString(),
                 QLatin1String{"<dbe9d22b-0a3f-cb1e-e883-8a148f00000@example.com> <03db3530-0000-0000-95a2-8a148f00000@example.com>"});
        QCOMPARE(normalize(removeFirstLine(QString::fromUtf8(result->body()))),
                 QLatin1String{"FsdflkjdslfjHappyMonday!Belowyouwillfindaquickoverviewofthecurrenton-goings.Remember"});
    }

    void testEncryptedWithProtectedHeadersForwardAsAttachment()
    {
        auto msg = readMail(QLatin1String("openpgp-encrypted-memoryhole2.mbox"));
        KMime::Message::Ptr result;
        MailTemplates::forward(msg, [&](const KMime::Message::Ptr &r) {
            result = r;
        });
        QTRY_VERIFY(result);
        QCOMPARE(result->subject()->asUnicodeString(), QLatin1String{"FW: This is the subject"});
        QCOMPARE(result->to()->addresses(), {});
        QCOMPARE(result->cc()->addresses(), {});
        QCOMPARE(result->references()->asUnicodeString(),
                 QLatin1String{"<dbe9d22b-0a3f-cb1e-e883-8a148f00000@example.com> <03db3530-0000-0000-95a2-8a148f00000@example.com>"});
        QCOMPARE(result->inReplyTo()->identifiers(), {});

        auto attachments = result->attachments();
        QCOMPARE(attachments.size(), 1);
        auto attachment = attachments[0];
        QCOMPARE(attachment->contentDisposition(false)->disposition(), KMime::Headers::CDinline);
        QCOMPARE(attachment->contentDisposition(false)->filename(), QLatin1String{"This is the subject.eml"});
        QVERIFY(attachment->bodyIsMessage());

        attachment->parse();
        auto origMsg = attachment->bodyAsMessage();
        QCOMPARE(origMsg->subject(false)->asUnicodeString(), QLatin1String{"..."});
    }
};

QTEST_MAIN(MailTemplateTest)
#include "mailtemplatetest.moc"
