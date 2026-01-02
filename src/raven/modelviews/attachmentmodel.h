// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QDBusInterface>
#include <QHash>
#include <QList>
#include <QSqlDatabase>

#include "../models/file.h"
#include "../models/message.h"

#include <qqmlintegration.h>

class AttachmentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AttachmentModel not creatable in QML")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int nonInlineCount READ nonInlineCount NOTIFY countChanged)

public:
    AttachmentModel(QObject *parent = nullptr);

    enum {
        FileIdRole,
        FileNameRole,
        ContentTypeRole,
        SizeRole,
        FormattedSizeRole,
        IconNameRole,
        IsInlineRole,
        DownloadedRole,
        FilePathRole
    };

    static AttachmentModel *self();

    // Set the database connection to use for operations
    void setDatabase(const QSqlDatabase &db);

    Q_INVOKABLE void loadMessage(Message *message);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void openAttachment(int index);
    Q_INVOKABLE void saveAttachment(int index);

    // Ensures attachment is downloaded, returns file path (empty on failure)
    QString ensureDownloaded(int index);

    // Get attachment count
    int count() const;
    int nonInlineCount() const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void countChanged();

private:
    QString fetchAttachmentViaDbus(const QString &fileId) const;

    QSqlDatabase m_db;
    QList<File *> m_files;
};
