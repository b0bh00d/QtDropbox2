#include "FoldersModel.h"

FoldersModel::FoldersModel(QAbstractListModel *parent): QAbstractListModel(parent)
{

}

FoldersModel::~FoldersModel()
{

}

QVariant FoldersModel::data(const QModelIndex &index, int role) const
{
    qDebug()<<" Data asked for "<<index.row()<<" and role "<<role<<"from "<<m_contentList.count();
    // should be unreachable code
    //    return m_contentList.at(index.row();
    if (index.row()<m_contentList.count()){
        switch (role) {
        case NameRole: return m_contentList.at(index.row())->filename();
        case DirectoryRole: return  m_contentList.at(index.row())->isDirectory();
        case PathRole:return m_contentList.at(index.row())->path();
        }
    }
    return QVariant();
}

int FoldersModel::rowCount(const QModelIndex &parent) const
{
    return m_contentList.count();
}

int FoldersModel::count()
{
    return m_contentList.count();
}

QDropbox2EntityInfo *FoldersModel::at(const int index)
{
    return m_contentList.at(index);
}

QHash<int, QByteArray> FoldersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "fileId";
    roles[NameRole] = "name";
    roles[DirectoryRole] = "isDirectory";
    roles[PathRole] = "path";
    return roles;
}

void FoldersModel::append(QDropbox2EntityInfo *item)
{
    beginInsertRows(QModelIndex(), count(),count());
    m_contentList.append(item);
    endInsertRows();
}

void FoldersModel::clear()
{
    beginResetModel();
    m_contentList.clear();
    endResetModel();
}
