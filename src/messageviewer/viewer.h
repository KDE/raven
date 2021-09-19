// SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>
// SPDX-FileCopyrightText: 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
// SPDX-FileCopyrightText: 2009 Andras Mantia <andras@kdab.net>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <KMime/Message>
#include <MimeTreeParser/Enums>
#include <Akonadi/Item>
#include <Akonadi/Monitor>

class QAbstractItemModel;

namespace Akonadi
{
class ItemFetchJob;
}


class ViewerHelper : public QObject
{
    Q_OBJECT

    /// This property holds the path to the message in terms of Akonadi collection hierarchy.
    Q_PROPERTY(QString messagePath READ messagePath WRITE setMessagePath NOTIFY messagePathChanged)

    /// This property holds the current message displayed in the viewer.
    Q_PROPERTY(KMime::Message::Ptr message READ message WRITE setMessage NOTIFY messageChanged)

    /// This property holds current message item displayed in the viewer.
    Q_PROPERTY(Akonadi::Item messageItem READ messageItem WRITE setMessageItem NOTIFY messageItemChanged)

    Q_PROPERTY(QString messagePath READ messagePath WRITE setMessagePath NOTIFY messagePathChanged)

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

    Q_PROPERTY(QString from READ from NOTIFY messageChanged);
    Q_PROPERTY(QStringList to READ to NOTIFY messageChanged);
    Q_PROPERTY(QStringList cc READ cc NOTIFY messageChanged);
    Q_PROPERTY(QString sender READ sender NOTIFY messageChanged);
    Q_PROPERTY(QString subject READ subject NOTIFY messageChanged);
    Q_PROPERTY(QDateTime date READ date NOTIFY messageChanged);
    Q_PROPERTY(QString content READ content NOTIFY messageChanged);
    Q_PROPERTY(Akonadi::Item::Id itemId READ itemId NOTIFY messageItemChanged);

public:
    /// Create a mail viewer helper
    explicit ViewerHelper(QObject *parent = nullptr);
    ~ViewerHelper() override;

    /// Returns the current message displayed in the viewer.
    Q_REQUIRED_RESULT KMime::Message::Ptr message() const;

    /// @returns the current message item displayed in the viewer.
    Q_REQUIRED_RESULT Akonadi::Item messageItem() const;

    Q_REQUIRED_RESULT bool loading() const;

    enum DisplayFormatMessage { UseGlobalSetting = 0, Text = 1, Html = 2, Unknown = 3, ICal = 4 };

    enum AttachmentAction { Open = 1, OpenWith, View, Save, Properties, Delete, Copy, ScrollTo, ReplyMessageToAuthor, ReplyMessageToAll };

    enum ResourceOnlineMode { AllResources = 0, SelectedResource = 1 };

    /**
     * Set the message that shall be shown.
     * @param message - the message to be shown. If 0, an empty page is displayed.
     * @param updateMode - update the display immediately or not. See UpdateMode.
     */
    void setMessage(const KMime::Message::Ptr &message, MimeTreeParser::UpdateMode updateMode = MimeTreeParser::Delayed);

    /**
     * Set the Akonadi item that will be displayed.
     * @param item - the Akonadi item to be displayed. If it doesn't hold a mail (KMime::Message::Ptr as payload data),
     *               an empty page is shown.
     * @param updateMode - update the display immediately or not. See UpdateMode.
     */
    void setMessageItem(const Akonadi::Item &item, MimeTreeParser::UpdateMode updateMode = MimeTreeParser::Delayed);

    Q_REQUIRED_RESULT QString messagePath() const;

    /// Set the path to the message in terms of Akonadi collection hierarchy.
    void setMessagePath(const QString &path);

    /**
     * Convenience method to clear the reader and discard the current message. Sets the internal message pointer
     * returned by message() to 0.
     * @param updateMode - update the display immediately or not. See UpdateMode.
     */
    void clear(MimeTreeParser::UpdateMode updateMode = MimeTreeParser::Delayed);

    void update(MimeTreeParser::UpdateMode updateMode = MimeTreeParser::Delayed);

    /** Get the load external references override setting */
    bool htmlLoadExtOverride() const;

    /** Default behavior for loading external references.
     *  Use this for specifying the external reference loading behavior as
     *  specified in the user settings.
     *  @see setHtmlLoadExtOverride
     */
    void setHtmlLoadExtDefault(bool loadExtDefault);

    /** Override default load external references setting
     *  @warning This must only be called when the user has explicitly
     *  been asked to retrieve external references!
     *  @see setHtmlLoadExtDefault
     */
    void setHtmlLoadExtOverride(bool loadExtOverride);

    /** Is html mail to be supported? Takes into account override */
    Q_REQUIRED_RESULT bool htmlMail() const;

    /** Is loading ext. references to be supported? Takes into account override */
    Q_REQUIRED_RESULT bool htmlLoadExternal() const;

    void writeConfig(bool withSync = true);
    void readConfig();

    /// A QAIM tree model of the message structure.
    QAbstractItemModel *messageTreeModel() const;

    /// Create an item fetch job that is suitable for using to fetch the message item that will
    /// be displayed on this viewer.
    /// It will set the correct fetch scope.
    /// You still need to connect to the job's result signal.
    Akonadi::ItemFetchJob *createFetchJob(const Akonadi::Item &item);

    /// Enforce message decryption.
    void setDecryptMessageOverwrite(bool overwrite = true);

    QString from() const;
    QStringList to() const;
    QStringList cc() const;
    QString sender() const;
    QString subject() const;
    QDateTime date() const;
    QString content() const;
    Akonadi::Item::Id itemId() const;

Q_SIGNALS:
    void messagePathChanged();
    void messageChanged();
    void messageItemChanged();
    void loadingChanged();

    void displayLoadingError(const QString &errorMessage);

private:
    KMime::Message::Ptr m_message; // the current message, if it was set manually
    Akonadi::Item m_messageItem; // the message item from Akonadi
    Akonadi::Session *m_session = nullptr;
    Akonadi::Monitor m_monitor;

    bool m_loading = true;
    QString m_messagePath;
};
