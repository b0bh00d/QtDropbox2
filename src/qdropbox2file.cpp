#include "qdropbox2file.h"

QDropbox2File::QDropbox2File(QObject *parent)
    : QIODevice(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(nullptr, "");
}

QDropbox2File::QDropbox2File(QDropbox2 *api, QObject *parent)
    : QIODevice(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(api, "");
}

QDropbox2File::QDropbox2File(const QString& filename, QDropbox2 *api, QObject *parent)
    : QIODevice(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(api, filename);
}

QDropbox2File::QDropbox2File(const QDropbox2File& source)
    : QIODevice(source.parent()),
      IQDropbox2Entity(source)
{
    init(source._api, source._filename);
}

QDropbox2File::~QDropbox2File()
{
    if(_buffer)
        delete _buffer;
    if(eventLoop)
        delete eventLoop;
}

void QDropbox2File::init(QDropbox2 *api, const QString& filename, qint64 threshold)
{
    if(filename.compare("/") == 0 || filename.isEmpty())
    {
        lastErrorCode = QDropbox2::APIError;
        lastErrorMessage = "Filename cannot be root ('/')";
    }
    else
    {
        _api              = api;
        _buffer           = nullptr;
        _filename         = filename;
        eventLoop         = nullptr;
        bufferThreshold   = threshold;
        overwrite_        = true;
        rename            = false;
        _metadata         = nullptr;
        lastErrorCode     = 0;
        lastErrorMessage  = "";
        position          = 0;
        currentThreshold  = 0;
        fileExists        = false;

        if(api)
            accessToken = api->accessToken();

        connect(&QNAM, &QNetworkAccessManager::finished, this, &QDropbox2File::slot_networkRequestFinished);
    }
}

int QDropbox2File::error()
{
    return lastErrorCode;
}

QString QDropbox2File::errorString()
{
    return lastErrorMessage;
}

bool QDropbox2File::isSequential() const
{
    return true;
}

bool QDropbox2File::open(QIODevice::OpenMode mode)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::open(...)" << endl;
#endif
    if(!QIODevice::open(mode))
        return result;

  /*  if(isMode(QIODevice::NotOpen))
        return true; */

    if(!_buffer)
        _buffer = new QByteArray();

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File: opening file" << endl;
#endif

    // clear buffer and reset position if this file was opened in write mode
    // with truncate - or if append was not set
    if(isMode(QIODevice::WriteOnly) && 
       (isMode(QIODevice::Truncate) || !isMode(QIODevice::Append))
      )
    {
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File: _buffer cleared." << endl;
#endif
        _buffer->clear();
        position = 0;
        result = true;
    }
    else
    {
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File: reading file content" << endl;
#endif
        if(getFile(_filename))  // this will return true if the file doesn't already exist
        {
            if(isMode(QIODevice::WriteOnly)) // write mode here means append
                position = _buffer->size();
            else if(isMode(QIODevice::ReadOnly)) // read mode here means start at the beginning
                position = 0;

            result = (lastErrorCode == 0 || lastErrorCode == 200 || fileExists);
            if(fileExists)
                obtainMetadata();
        }
    }

    return result;
}

void QDropbox2File::close()
{
    if(isMode(QIODevice::WriteOnly) && _buffer->length())
        flush();
    QIODevice::close();
}

void QDropbox2File::setApi(QDropbox2 *dropbox)
{
    _api = dropbox;
}

void QDropbox2File::setFilename(const QString& filename)
{
    _filename = filename;
}

bool QDropbox2File::flush()
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::flush()" << endl;
#endif

    return putFile();
}

bool QDropbox2File::event(QEvent *event)
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "processing event: " << event->type() << endl;
#endif
    return QIODevice::event(event);
}

//void QDropbox2File::setFlushThreshold(qint64 num)
//{
//    if(num < 0)
//        num = 150*1024*1024;
//    bufferThreshold = num;
//}

void QDropbox2File::setOverwrite(bool overwrite)
{
    overwrite_ = overwrite;
}

void QDropbox2File::setRenaming(bool rename)
{
    this->rename = rename;
}

qint64 QDropbox2File::readData(char *data, qint64 maxlen)
{
    if(!maxlen)
        return maxlen;      // we do no "post-reading operations"

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::readData(...), maxlen = " << maxlen << endl;
    QString buff_str = QString(*_buffer);
    //qDebug() << "old bytes = " << _buffer->toHex() << ", str: " << buff_str <<  endl;
    qDebug() << "old size = " << _buffer->size() << endl;
#endif

    if(_buffer->size() == 0 || position >= _buffer->size())
        return 0;

    if(_buffer->size() < maxlen)
        maxlen = _buffer->size();

    QByteArray tmp = _buffer->mid(position, maxlen);
    const qint64 read = tmp.size();
    memcpy(data, tmp.data(), read);
   
#ifdef QTDROPBOX_DEBUG
    qDebug() << "new size = " << _buffer->size() << endl;
    //qDebug() << "new bytes = " << _buffer->toHex() << endl;
#endif

    position += read;

    return read;
}

qint64 QDropbox2File::writeData(const char *data, qint64 len)
{
    int written_bytes = 0;

    // if we exceed the APIv2 single-call "/upload" threshold of
    // 150MB, we fail...
    if((currentThreshold + len) < bufferThreshold)
    {
        qint64 oldlen = _buffer->size();
        _buffer->insert(position, data, len);

        //// flush if the threshold is reached
        //if(currentThreshold > bufferThreshold)
        //    flush();

        currentThreshold += len;
        written_bytes = len;

        if(_buffer->size() != oldlen+len)
            written_bytes = (oldlen-_buffer->size());

        position += written_bytes;
    }

    return written_bytes;
}

void QDropbox2File::slot_networkRequestFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    lastErrorCode = reply->error();
    if(replyMap.contains(reply))
    {
        CallbackPtr async_data(replyMap[reply]);
        if(async_data->callback)
            (this->*async_data->callback)(reply, async_data);
        replyMap.remove(reply);
    }
    else
    {
        if(lastErrorCode == QNetworkReply::NoError)
        {
            QByteArray buff = reply->readAll();
            lastResponse = QString(buff);
    #ifdef QTDROPBOX_DEBUG
            qDebug() << "QDropbox2Folder::slot_networkRequestFinished(...)" << endl;
            qDebug() << "request was: " << reply->url().toString() << endl;
            qDebug() << "response: " << reply->bytesAvailable() << "bytes" << endl;
            qDebug() << "status code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() << endl;
            qDebug() << "== begin response ==" << endl << lastResponse << endl << "== end response ==" << endl;
    #endif
        }
        else
            lastErrorMessage = reply->errorString();

        stopEventLoop();
    }
}

bool QDropbox2File::isMode(QIODevice::OpenMode mode)
{
    return ((openMode() & mode) == mode);
}

QNetworkReply* QDropbox2File::sendPOST(QNetworkRequest& rq, QByteArray& postdata)
{
    QNetworkReply *reply = QNAM.post(rq, postdata);
    connect(this, &QDropbox2File::signal_operationAborted, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::uploadProgress, this, &QDropbox2File::signal_uploadProgress);
    return reply;
}

QNetworkReply* QDropbox2File::sendGET(QNetworkRequest& rq)
{
    QNetworkReply *reply = QNAM.get(rq);
    connect(this, &QDropbox2File::signal_operationAborted, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::downloadProgress, this, &QDropbox2File::signal_downloadProgress);
    return reply;
}

bool QDropbox2File::getFile(const QString& filename)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::getFileContent(...)" << endl;
#endif
    QUrl url;
    url.setUrl(QDROPBOX2_CONTENT_URL, QUrl::StrictMode);
    url.setPath("/2/files/download");

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setRawHeader("Dropbox-API-arg", QString("{ \"path\": \"%1\" }")
                                                .arg((filename.compare("/") == 0) ? "" : filename).toUtf8());

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::getFileContent " << url.toString() << endl;
#endif

    QNetworkReply* reply = sendGET(req);

    CallbackPtr reply_data(new CallbackData);
    reply_data->callback = &QDropbox2File::resultGetFile;
    replyMap[reply] = reply_data;

    startEventLoop();

    fileExists = !lastErrorMessage.contains("path/not_found");
    result = (lastErrorCode == 0 || lastErrorCode == 200 || lastErrorCode == 206 || !fileExists);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::getFileContent ReadError: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

void QDropbox2File::resultGetFile(QNetworkReply *reply, CallbackPtr /*reply_data*/)
{
    lastErrorCode = 0;

    QByteArray response = reply->readAll();
    QString resp_str;
    QDropbox2Json json;

#ifdef QTDROPBOX_DEBUG
    //resp_str = QString(response.toHex());
    //qDebug() << "QDropbox2File::replyFileContent response = " << resp_str << endl;

#endif

    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == QDROPBOX_V2_ERROR)
    {
        resp_str = QString(response);
        json.parseString(response.trimmed());
        lastErrorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::replyFileContent jason.valid = " << json.isValid() << endl;
#endif
        if(json.isValid())
        {
            if(json.hasKey("user_message"))
                lastErrorMessage = json.getString("user_message");
            else if(json.hasKey("error_summary"))
                lastErrorMessage = json.getString("error_summary");
        }
        else
            lastErrorMessage = "";

        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        _buffer->clear();
        _buffer->append(response);
        emit readyRead();
    }

    stopEventLoop();
}

bool QDropbox2File::putFile()
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::putFile()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_CONTENT_URL, QUrl::StrictMode);
    url.setPath("/2/files/upload");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    QString json = QString("{ \"path\": \"%1\", \"mode\": \"%2\", \"autorename\": %3, \"mute\": true }")
                                        .arg(_filename)
                                        .arg(overwrite_ ? "overwrite" : "add")
                                        .arg(rename ? "true" : "false");
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::Dropbox-API-arg " << json << endl;
#endif
    req.setRawHeader("Dropbox-API-arg", json.toUtf8());

    // "{ \"path\": \"%1\", \"mode\": \"overwrite\", \"autorename\": %2, \"mute\": true }"
    // "{ \"path\": \"%1\", \"mode\": \"update\", \"autorename\": %2, \"mute\": true }"

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::putFile " << url.toString() << endl;
#endif

    QNetworkReply* reply = sendPOST(req, *_buffer);

    CallbackPtr reply_data(new CallbackData);
    reply_data->callback = &QDropbox2File::resultPutFile;
    replyMap[reply] = reply_data;

    startEventLoop();

    result = (lastErrorCode == 0);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::putFile WriteError: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        // we wrote the whole file, so reset
        _buffer->clear();
        position = 0;
        currentThreshold = 0;
    }

    return result;
}

void QDropbox2File::resultPutFile(QNetworkReply *reply, CallbackPtr /*reply_data*/)
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::replyFileWrite(...)" << endl;
#endif

    lastErrorCode = 0;

    QByteArray response = reply->readAll();
    QString resp_str;
    QDropbox2Json json;

#ifdef QTDROPBOX_DEBUG
    resp_str = response;
    qDebug() << "QDropbox2File::replyFileWrite response = " << resp_str << endl;
#endif

    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == QDROPBOX_V2_ERROR)
    {
        resp_str = QString(response);
        json.parseString(response.trimmed());
        lastErrorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::replyFileWrite jason.valid = " << json.isValid() << endl;
#endif
        if(json.isValid())
        {
            if(json.hasKey("user_message"))
                lastErrorMessage = json.getString("user_message");
            else if(json.hasKey("error_summary"))
                lastErrorMessage = json.getString("error_summary");
        }
        else
            lastErrorMessage = "";

        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        delete _metadata;

        _metadata = new QDropbox2EntityInfo{QString{response}.trimmed(), this};
        if (!_metadata->isValid())
            _metadata->clear();

        emit bytesWritten(_buffer->size());
    }

    stopEventLoop();
}

void QDropbox2File::startEventLoop()
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::startEventLoop()" << endl;
#endif
    if(!eventLoop)
        eventLoop = new QEventLoop(this);
    eventLoop->exec();
}

void QDropbox2File::stopEventLoop()
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::stopEventLoop()" << endl;
#endif
    if(!eventLoop)
        return;
    eventLoop->exit();
}

QDropbox2EntityInfo QDropbox2File::metadata()
{
    obtainMetadata();

    if(_metadata && _metadata->isValid())
        lastHash = _metadata->revisionHash();

    return _metadata ? *_metadata : QDropbox2EntityInfo();
}

void QDropbox2File::obtainMetadata()
{
    if(_metadata)
        _metadata->deleteLater();
    _metadata = nullptr;

    // APIv2 Note: Metadata for the root folder is unsupported. 
    if(!_filename.compare("/") || _filename.isEmpty())
    {
        lastErrorCode = QDropbox2::APIError;
        lastErrorMessage = "Metadata for the root folder is unsupported.";
#ifdef QTDROPBOX_DEBUG
        qDebug() << "error: " << errorText << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        QUrl url;
        url.setUrl(QDROPBOX2_API_URL);
        url.setPath(QString("/2/files/get_metadata"));

#ifdef QTDROPBOX_DEBUG
        qDebug() << "metadata = \"" << url.toEncoded() << "\"" << endl;;
#endif

        QNetworkRequest req;
        if(!_api->createAPIv2Reqeust(url, req))
            return;
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QString json = QString("{\"path\": \"%1\", \"include_media_info\": true, \"include_deleted\": true, \"include_has_explicit_shared_members\": true}")
                                    .arg(_filename);
#ifdef QTDROPBOX_DEBUG
        qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
        QByteArray postdata = json.toUtf8();
        (void)sendPOST(req, postdata);
    
        startEventLoop();

        if(lastErrorCode != 0)
        {
#ifdef QTDROPBOX_DEBUG
            qDebug() << "QDropbox2File::requestRemoval error: " << lastErrorCode << lastErrorMessage << endl;
#endif
            emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
        }
        else
        {
            _metadata = new QDropbox2EntityInfo(lastResponse);
            if(!_metadata->isValid())
            {
                lastErrorCode = QDropbox2::APIError;
                lastErrorMessage = "Dropbox API did not send correct answer for file/directory metadata.";
#ifdef QTDROPBOX_DEBUG
                qDebug() << "error: " << errorText << endl;
#endif
                emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
            }
        }
    }
}

bool QDropbox2File::hasChanged()
{
    if(lastHash.isEmpty())
    {
        obtainMetadata();
        if(_metadata && _metadata->isValid())
            lastHash = _metadata->revisionHash();
        return false;
    }

    // get updated information
    obtainMetadata();

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::hasChanged() local  revision hash = " << lastHash << endl;
    qDebug() << "QDropbox2File::hasChanged() remote revision hash = " << _metadata->revisionHash() << endl;
#endif
    bool result = lastHash.compare(_metadata->revisionHash()) != 0;
    lastHash = _metadata->revisionHash();
    return result;
}

//void QDropbox2File::obtainMetadata()
//{
//    // get metadata of this file
//    if(_metadata)
//        _metadata->deleteLater();
//    _metadata = new QDropbox2EntityInfo(_api->requestMetadataAndWait(_filename).strContent(), this);
//    if(!_metadata->isValid())
//        _metadata->clear();
//}

bool QDropbox2File::revisions(QDropbox2File::RevisionsList& revisions, quint64 max_results)
{
    bool result = false;
    revisions.clear();

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::revisions()" << endl;
#endif

    QNetworkReply* reply;
    result = getRevisions(reply, max_results);

    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::contents error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        result = true;

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("entries"))
        {
            QStringList data = json.getArray("entries");
            if(data.count())
            {
                foreach(const QString& entry_str, data)
                    revisions.append(QDropbox2EntityInfo(entry_str));
            }
        }
    }

    return result;
}

bool QDropbox2File::revisions(quint64 max_results)
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::revisions()" << endl;
#endif

    QNetworkReply* reply;
    bool result = getRevisions(reply, max_results);

    CallbackPtr reply_data(new CallbackData);
    reply_data->callback = &QDropbox2File::revisionsCallback;
    replyMap[reply] = reply_data;

    return result;
}

void QDropbox2File::revisionsCallback(QNetworkReply* /*reply*/, CallbackPtr /*reply_data*/)
{
    if(lastErrorCode)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::revisionsCallback error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        RevisionsList revisions_results;

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("entries"))
        {
            QStringList data = json.getArray("entries");
            if(data.count())
            {
                foreach(const QString& entry_str, data)
                    revisions_results.append(QDropbox2EntityInfo(entry_str));
            }
        }

        emit signal_revisionsResult(revisions_results);
    }
}

bool QDropbox2File::getRevisions(QNetworkReply*& reply, quint64 max_results, bool async)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::getRevisions()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/list_revisions");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"path\": \"%1\", \"limit\": %2}")
                            .arg(_filename)
                            .arg(max_results);

#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    reply = sendPOST(req, postdata);

    if(async)
        return true;

    startEventLoop();

    result = (lastErrorCode == 0);
    return result;
}

bool QDropbox2File::seek(qint64 pos)
{
    if(pos > _buffer->size())
        return false;

    QIODevice::seek(pos);
    position = pos;
    return true;
}

bool QDropbox2File::reset()
{
    QIODevice::reset();
    position = 0;
    return true;
}

void QDropbox2File::slot_abort()
{
    emit signal_operationAborted();
}

qint64 QDropbox2File::bytesAvailable() const
{
    return _buffer->size();
}

bool QDropbox2File::remove(bool permanently)
{
    return requestRemoval(permanently);
}

bool QDropbox2File::requestRemoval(bool permanently)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::requestRemoval()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    if(permanently)
        url.setPath("/2/files/permanently_delete");
    else
        url.setPath("/2/files/delete");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"path\": \"%1\"}")
                                .arg((_filename.compare("/") == 0) ? "" : _filename);
#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::requestRemoval error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

bool QDropbox2File::move(const QString& to_path)
{
    return requestMove(to_path);
}

bool QDropbox2File::requestMove(const QString& to_path)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::requestMove()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/move");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"from_path\": \"%1\", \"to_path\": \"%2\", \"autorename\": %3, \"allow_shared_folder\": false}")
                                    .arg(_filename)
                                    .arg(to_path)
                                    .arg(rename ? "true" : "false");

#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::requestMove error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

// TODO: Collapse copy into move
bool QDropbox2File::copy(const QString& to_path)
{
    return requestCopy(to_path);
}

bool QDropbox2File::requestCopy(const QString& to_path)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::requestCopy()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/copy");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"from_path\": \"%1\", \"to_path\": \"%2\", \"autorename\": %3, \"allow_shared_folder\": false}")
                                    .arg(_filename)
                                    .arg(to_path)
                                    .arg(rename ? "true" : "false");

#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::requestCopy error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

QUrl QDropbox2File::temporaryLink()
{
    return requestStreamingLink();
}

QUrl QDropbox2File::requestStreamingLink()
{
    QUrl result;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2File::requestStreamingLink()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/get_temporary_link");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"path\": \"%1\"}").arg(_filename);

#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    if(lastErrorCode != 0)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2File::requestStreamingLink error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        QDropbox2Json json;
        json.parseString(lastResponse);
        result.setUrl(json.getString("link"));
    }

    return result;
}
