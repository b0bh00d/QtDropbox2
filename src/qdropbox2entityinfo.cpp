#include <QFileInfo>
#include <QLocale>

#include "qdropbox2entityinfo.h"

QDropbox2EntityInfo::QDropbox2EntityInfo(QObject *parent) :
    QDropbox2Json(parent)
{
  init();
}

QDropbox2EntityInfo::QDropbox2EntityInfo(QString jsonStr, QObject *parent) :
    QDropbox2Json(jsonStr, parent)
{
    init();
    dataFromJson();
}

QDropbox2EntityInfo::QDropbox2EntityInfo(const QDropbox2EntityInfo &other) :
    QDropbox2Json(0)
{
    init();
    copyFrom(other);
}

QDropbox2EntityInfo::~QDropbox2EntityInfo()
{
}

void QDropbox2EntityInfo::copyFrom(const QDropbox2EntityInfo &other)
{
    parseString(other.strContent());
    dataFromJson();
    setParent(other.parent());
}

QDropbox2EntityInfo &QDropbox2EntityInfo::operator=(const QDropbox2EntityInfo &other)
{
    copyFrom(other);
    return *this;
}

void QDropbox2EntityInfo::dataFromJson()
{
    if(isValid())
    {
        _id             = getString("id");
        _clientModified = getTimestamp("client_modified");
        _serverModified = getTimestamp("server_modified");
        _revisionHash   = getString("rev");
        _bytes          = getInt("size");
        _size           = QString::number(_bytes);
        _path           = getString("path_display");
        _isShared       = hasKey("sharing_info");
        _isDir          = getString(".tag").compare("folder") == 0;
        _isDeleted      = getString(".tag").compare("deleted") == 0;

        QFileInfo info(_path);
        _filename = info.fileName();
    }
}

void QDropbox2EntityInfo::init()
{
    _size           = "";
    _bytes          = 0;
    _serverModified = QDateTime::currentDateTime();
    _clientModified = QDateTime::currentDateTime();
    _path           = "";
    _filename       = "";
    _revisionHash   = "";
    _isDir          = false;
    _isShared       = false;
    _isDeleted      = false;
}

QString QDropbox2EntityInfo::size() const
{
    QLocale local;
    return local.toString(_bytes);
}
