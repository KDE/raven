#ifndef MAILMODEL_H
#define MAILMODEL_H

#include <QObject>
#include <QtCore/QSortFilterProxyModel>

class MailModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum AnimalRoles {
        TitleRole = Qt::UserRole + 1,
        SenderRole,
        DateRole,
        PlainTextRole,
    };

    explicit MailModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    virtual QVariant data(const QModelIndex &index, int role) const override;

signals:
};

#endif // MAILMODEL_H
