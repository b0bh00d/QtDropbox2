#include "FoldersModel.h"

FoldersModel::FoldersModel(QAbstractListModel *parent): QAbstractListModel(parent)
{

}

FoldersModel::~FoldersModel()
{

}

QVariant FoldersModel::data(const QModelIndex &index, int role) const
{
    qDebug()<<" Data asked for "<<index.row()<<" and role "<<role;
    // should be unreachable code
//    return m_contentList.at(index.row();
    switch (role) {
    case NameRole: return m_contentList.at(index.row())->filename();

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
    return roles;
}

void FoldersModel::append(QDropbox2EntityInfo *item)
{
    m_contentList.append(item);
}

void FoldersModel::clear()
{
    m_contentList.clear();
}
