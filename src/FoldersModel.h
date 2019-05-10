#ifndef FOLDERSMODEL_H
#define FOLDERSMODEL_H

#include <QObject>
#include <QAbstractListModel>

#include "qdropbox2entityinfo.h"

class FoldersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum FolderListRole {
        IdRole = Qt::UserRole, // 256
        NameRole,
        DirectoryRole,
        PathRole
    };
    //TODO: Change to no QDropbox2EntityInfo pointer as soon as a qml test exists
    typedef QList<QDropbox2EntityInfo*> ContentsList;

public:
    FoldersModel(QAbstractListModel *parent=nullptr);
    ~FoldersModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int count();
    QDropbox2EntityInfo *at(const int index);
    QHash<int, QByteArray> roleNames() const;

    void append(QDropbox2EntityInfo* item);
    void clear();
private:
    ContentsList m_contentList;
signals:

public slots:
};

#endif // FOLDERSMODEL_H
