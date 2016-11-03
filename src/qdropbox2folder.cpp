#include <QDir>

#include "qdropbox2folder.h"

QDropbox2Folder::QDropbox2Folder(QObject *parent)
    : QObject(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(nullptr, "");
}

QDropbox2Folder::QDropbox2Folder(QDropbox2 *api, QObject *parent)
    : QObject(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(api, "");
}

QDropbox2Folder::QDropbox2Folder(const QString& foldername, QDropbox2 *api, QObject *parent)
    : QObject(parent),
      IQDropbox2Entity(),
      QNAM(this)
{
    init(api, foldername);
}

QDropbox2Folder::QDropbox2Folder(const QDropbox2Folder& source)
    : QObject(source.parent()),
      IQDropbox2Entity(source)
{
    init(source._api, source._foldername);
}

QDropbox2Folder::~QDropbox2Folder()
{
    if(eventLoop)
        delete eventLoop;
}

void QDropbox2Folder::init(QDropbox2 *api, const QString& foldername)
{
    _api              = api;
    _foldername       = foldername;
    eventLoop         = nullptr;
    rename            = false;
    _metadata         = nullptr;
    lastErrorCode     = 0;
    lastErrorMessage  = "";

    if(api)
        accessToken = api->accessToken();

    connect(&QNAM, &QNetworkAccessManager::finished, this, &QDropbox2Folder::slot_networkRequestFinished);

    getLatestCursor(latestCursor);
}

int QDropbox2Folder::error()
{
    return lastErrorCode;
}

QString QDropbox2Folder::errorString()
{
    return lastErrorMessage;
}

void QDropbox2Folder::setApi(QDropbox2 *dropbox)
{
    _api = dropbox;
}

void QDropbox2Folder::setFoldername(const QString& foldername)
{
    _foldername = foldername;
}

void QDropbox2Folder::setRenaming(bool rename)
{
    this->rename = rename;
}

void QDropbox2Folder::slot_networkRequestFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    QByteArray buff = reply->readAll();
    lastResponse = QString(buff);

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::slot_networkRequestFinished(...)" << endl;
    qDebug() << "request was: " << reply->url().toString() << endl;
    qDebug() << "response: " << reply->bytesAvailable() << "bytes" << endl;
    qDebug() << "status code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() << endl;
    qDebug() << "== begin response ==" << endl << lastResponse << endl << "== end response ==" << endl;
#endif

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
        if (lastErrorCode != QNetworkReply::NoError)
        {
#ifdef QTDROPBOX_DEBUG
            // debug information only - this should not happen, but if it does we 
            // ignore replies when not waiting for anything
#endif
            lastErrorMessage = reply->errorString();
            emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
        }
        stopEventLoop();
    }

}

QNetworkReply* QDropbox2Folder::sendPOST(QNetworkRequest& rq, QByteArray& postdata)
{
    QNetworkReply *reply = QNAM.post(rq, postdata);
    connect(this, &QDropbox2Folder::signal_operationAborted, reply, &QNetworkReply::abort);
    //connect(reply, &QNetworkReply::uploadProgress, this, &QDropbox2Folder::signal_uploadProgress);
    return reply;
}

void QDropbox2Folder::startEventLoop()
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::startEventLoop()" << endl;
#endif
    if(!eventLoop)
        eventLoop = new QEventLoop(this);
    eventLoop->exec();
}

void QDropbox2Folder::stopEventLoop()
{
#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::stopEventLoop()" << endl;
#endif
    if(!eventLoop)
        return;
    eventLoop->exit();
}

QDropbox2EntityInfo QDropbox2Folder::metadata()
{
    obtainMetadata();
    return _metadata ? *_metadata : QDropbox2EntityInfo();
}

bool QDropbox2Folder::getLatestCursor(QString& cursor, bool include_deleted)
{
   bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::getLatestCursor()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/list_folder/get_latest_cursor");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json;
    // TODO: the APIv2 /list_folder/longpoll call does not currently support cursors generated with the "include_media_info" value set to true.
    // https://www.dropboxforum.com/t5/API-support/Getting-400-HTTP-error-for-list-folder-longpoll/td-p/164086/page/2
    json = QString("{\"path\": \"%1\", \"recursive\": false, \"include_media_info\": false, \"include_deleted\": %2, \"include_has_explicit_shared_members\": true}")
                                .arg((_foldername.compare("/") == 0) ? "" : _foldername)
                                .arg(include_deleted ? "true" : "false");

#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0);
    if(result)
    {
        QDropbox2Json json;
        json.parseString(lastResponse);
        cursor = json.getString("cursor");
    }
    return result;
}

bool QDropbox2Folder::hasChanged(ContentsList& changes)
{
    if(latestCursor.isEmpty())
    {
        getLatestCursor(latestCursor, true);
        return false;
    }

    QNetworkReply* reply;
    if(!getContents(reply, latestCursor, true))
        return false;

    QDropbox2Json json;
    json.parseString(lastResponse);
    latestCursor = json.getString("cursor");
    if(!json.hasKey("entries"))
        return false;

    QStringList data = json.getArray("entries");
    if(!data.count())
        return false;

    foreach(const QString& entry_str, data)
        changes.append(QDropbox2EntityInfo(entry_str));

    return true;
}

bool QDropbox2Folder::hasChanged()
{
    if(latestCursor.isEmpty())
        getLatestCursor(latestCursor, true);

    QNetworkReply* reply;
    bool result = getContents(reply, latestCursor, true, true);
    if(result)
    {
        CallbackPtr reply_data(new CallbackData);
        reply_data->callback = &QDropbox2Folder::hasChangedCallback;
        replyMap[reply] = reply_data;
    }

    return result;
}

void QDropbox2Folder::hasChangedCallback(QNetworkReply* /*reply*/, CallbackPtr /*reply_data*/)
{
    QDropbox2Json json;
    json.parseString(lastResponse);
    latestCursor = json.getString("cursor");
    if(!json.hasKey("entries"))
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::hasChangedCallback error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        QDropbox2Folder::ContentsList changes;

        QStringList data = json.getArray("entries");
        if(data.count())
        {
            foreach(const QString& entry_str, data)
                changes.append(QDropbox2EntityInfo(entry_str));
        }

        emit signal_hasChangedResults(changes);
    }
}

bool QDropbox2Folder::waitForChanged(int timeout)
{
    bool result = false;

    if(latestCursor.isEmpty())
        getLatestCursor(latestCursor);

    while(true)
    {
        if(requestLongpoll(timeout))
        {
            QDropbox2Json json;
            json.parseString(lastResponse);
            result = json.getBool("changes");
            break;
        }

        if(!lastErrorMessage.contains("reset/"))
            break;

        // we need to refresh the cursor, and try again
        getLatestCursor(latestCursor);
    }

    return result;
}

bool QDropbox2Folder::requestLongpoll(int timeout)
{
    bool result = false;

    Q_ASSERT(!latestCursor.isEmpty());

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::requestLongpoll()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_NOTIFY_URL, QUrl::StrictMode);
    url.setPath("/2/files/list_folder/longpoll");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req, false))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"cursor\": \"%1\", \"timeout\": %2}")
                                .arg(latestCursor)
                                .arg(timeout);
#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0 || lastErrorCode == 206);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::requestCreation error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

void QDropbox2Folder::obtainMetadata()
{
    if(_metadata)
        _metadata->deleteLater();
    _metadata = nullptr;

    // APIv2 Note: Metadata for the root folder is unsupported. 
    if(!_foldername.compare("/") || _foldername.isEmpty())
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
                                    .arg(_foldername);
#ifdef QTDROPBOX_DEBUG
        qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
        QByteArray postdata = json.toUtf8();
        (void)sendPOST(req, postdata);
    
        startEventLoop();

        if(lastErrorCode != 0)
        {
#ifdef QTDROPBOX_DEBUG
            qDebug() << "QDropbox2File::obtainMetadata error: " << lastErrorCode << lastErrorMessage << endl;
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

void QDropbox2Folder::slot_abort()
{
    emit signal_operationAborted();
}

bool QDropbox2Folder::create()
{
    return requestCreation();
}

bool QDropbox2Folder::requestCreation()
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::requestCreation()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/create_folder");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json = QString("{\"path\": \"%1\", \"autorename\": %2}")
                                .arg((_foldername.compare("/") == 0) ? "" : _foldername)
                                .arg(rename ? "true" : "false");
#ifdef QTDROPBOX_DEBUG
    qDebug() << "postdata = \"" << json << "\"" << endl;;
#endif
    QByteArray postdata = json.toUtf8();
    (void)sendPOST(req, postdata);

    startEventLoop();

    result = (lastErrorCode == 0 || lastErrorCode == 206);
    if(!result)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::requestCreation error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

bool QDropbox2Folder::remove(bool permanently)
{
    return requestRemoval(permanently);
}

bool QDropbox2Folder::requestRemoval(bool permanently)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::requestRemoval()" << endl;
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
                                .arg((_foldername.compare("/") == 0) ? "" : _foldername);
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
        qDebug() << "QDropbox2Folder::requestRemoval error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

bool QDropbox2Folder::move(const QString& to_path)
{
    return requestMove(to_path);
}

bool QDropbox2Folder::requestMove(const QString& to_path)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::requestMove()" << endl;
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
                                    .arg(_foldername)
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
        qDebug() << "QDropbox2Folder::requestMove error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

// TODO: Collapse copy into move
bool QDropbox2Folder::copy(const QString& to_path)
{
    return requestCopy(to_path);
}

bool QDropbox2Folder::requestCopy(const QString& to_path)
{
    bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::requestCopy()" << endl;
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
                                    .arg(_foldername)
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
        qDebug() << "QDropbox2Folder::requestCopy error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }

    return result;
}

//--------------------------------------
// List Folder

bool QDropbox2Folder::contents(QDropbox2Folder::ContentsList& contents, bool include_folders, bool include_deleted)
{
    bool result = false;
    contents.clear();
    latestCursor.clear();       // make sure we get a "current" listing, not a differential

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::contents()" << endl;
#endif

    bool has_more = true;
    do
    {
        QNetworkReply* reply;
        if(!(result = getContents(reply, latestCursor, include_deleted)))
        {
#ifdef QTDROPBOX_DEBUG
            qDebug() << "QDropbox2Folder::contents error: " << lastErrorCode << lastErrorMessage << endl;
#endif
            emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
            break;
        }

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("entries"))
        {
            QStringList data = json.getArray("entries");
            foreach(const QString& entry_str, data)
            {
                if(!include_folders)
                {
                    QDropbox2Json entry;
                    entry.parseString(entry_str);
                    if(!entry.getString(".tag").compare("folder"))
                        continue;
                }

                contents.append(QDropbox2EntityInfo(entry_str));
            }
        }

        latestCursor = json.getString("cursor");
        has_more = json.getBool("has_more");
    } while(has_more);

    return result;
}

bool QDropbox2Folder::contents(bool include_folders, bool include_deleted)
{
    lastErrorCode = 0;
    latestCursor.clear();       // make sure we get a "current" listing, not a differential

    QNetworkReply* reply;
    bool result = getContents(reply, latestCursor, include_deleted, true);
    if(result)
    {
        CallbackPtr reply_data(new ContentsData());
        ContentsData* content_data = reinterpret_cast<ContentsData*>(reply_data.data());
        content_data->callback = &QDropbox2Folder::contentsCallback;
        content_data->include_folders = include_folders;
        replyMap[reply] = reply_data;
    }

    return result;
}

void QDropbox2Folder::contentsCallback(QNetworkReply* /*reply*/, CallbackPtr reply_data)
{
    if(lastErrorCode)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::contents error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        ContentsList contents_results;
        ContentsData* contents_data = reinterpret_cast<ContentsData*>(reply_data.data());

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("entries"))
        {
            QStringList data = json.getArray("entries");
            foreach(const QString& entry_str, data)
            {
                if(!contents_data->include_folders)
                {
                    QDropbox2Json entry;
                    entry.parseString(entry_str);
                    if(!entry.getString(".tag").compare("folder"))
                        continue;
                }

                contents_results.append(QDropbox2EntityInfo(entry_str));
            }
        }

        latestCursor = json.getString("cursor");
        // TODO: figure out how to handle 'has_more' asynchronously
        //has_more = json.getBool("has_more");

        emit signal_contentsResults(contents_results);
    }
}

bool QDropbox2Folder::getContents(QNetworkReply*& reply, const QString& cursor, bool include_deleted, bool async)
{
   bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::getContents()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    if(cursor.isEmpty())
        url.setPath("/2/files/list_folder");
    else
        url.setPath("/2/files/list_folder/continue");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json;
    if(cursor.isEmpty())
        // TODO: the APIv2 /list_folder/longpoll call does not currently support cursors generated with the "include_media_info" value set to true.
        // https://www.dropboxforum.com/t5/API-support/Getting-400-HTTP-error-for-list-folder-longpoll/td-p/164086/page/2
        json = QString("{\"path\": \"%1\", \"recursive\": false, \"include_media_info\": false, \"include_deleted\": %2, \"include_has_explicit_shared_members\": true}")
                                    .arg((_foldername.compare("/") == 0) ? "" : _foldername)
                                    .arg(include_deleted ? "true" : "false");
    else
        json = QString("{\"cursor\": \"%1\"}").arg(cursor);

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

//--------------------------------------
// Search

bool QDropbox2Folder::search(QDropbox2Folder::ContentsList& contents, const QString& query, quint64 max_results, const QString& mode)
{
    bool result = false;
    contents.clear();

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::search()" << endl;
#endif

    bool has_more = true;
    quint64 start = 0;
    do
    {
        QNetworkReply* reply;
        if(!(result = getSearch(reply, query, start, max_results, mode, false)))
        {
#ifdef QTDROPBOX_DEBUG
            qDebug() << "QDropbox2Folder::search error: " << lastErrorCode << lastErrorMessage << endl;
#endif
            emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
            break;
        }

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("matches"))
        {
            QStringList data = json.getArray("matches");
            foreach(const QString& entry_str, data)
            {
                QDropbox2Json entry;
                entry.parseString(entry_str);
                if(entry.hasKey("metadata"))
                    contents.append(QDropbox2EntityInfo(entry.getJson("metadata")->strContent()));
            }
        }

        start = json.getInt("start");
        has_more = json.getBool("more");
    } while(has_more);

    return result;
}

bool QDropbox2Folder::search(const QString& query, quint64 max_results, const QString& mode)
{
    lastErrorCode = 0;

    QNetworkReply* reply;
    bool result = getSearch(reply, query, 0, max_results, mode, true);
    if(result)
    {
        CallbackPtr reply_data(new CallbackData);
        reply_data->callback = &QDropbox2Folder::searchCallback;
        replyMap[reply] = reply_data;
    }
    return result;
}

void QDropbox2Folder::searchCallback(QNetworkReply* /*reply*/, CallbackPtr /*reply_data*/)
{
    if(lastErrorCode)
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox2Folder::search error: " << lastErrorCode << lastErrorMessage << endl;
#endif
        emit signal_errorOccurred(lastErrorCode, lastErrorMessage);
    }
    else
    {
        ContentsList search_results;

        QDropbox2Json json;
        json.parseString(lastResponse);
        if(json.hasKey("matches"))
        {
            QStringList data = json.getArray("matches");
            foreach(const QString& entry_str, data)
            {
                QDropbox2Json entry;
                entry.parseString(entry_str);
                if(entry.hasKey("metadata"))
                    search_results.append(QDropbox2EntityInfo(entry.getJson("metadata")->strContent()));
            }
        }

        //start = json.getInt("start");
        // TODO: figure out how to handle 'has_more' asynchronously
        //has_more = json.getBool("more");

        emit signal_searchResults(search_results);
    }
}

bool QDropbox2Folder::getSearch(QNetworkReply*& reply, const QString& query, quint64 start, quint64 max_results, const QString& mode, bool async)
{
   bool result = false;

#ifdef QTDROPBOX_DEBUG
    qDebug() << "QDropbox2Folder::getSearch()" << endl;
#endif

    QUrl url;
    url.setUrl(QDROPBOX2_API_URL, QUrl::StrictMode);
    url.setPath("/2/files/search");

    Q_ASSERT(url.isValid());

    QNetworkRequest req;
    if(!_api->createAPIv2Reqeust(url, req))
        return result;

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString json;
    json = QString("{\"path\": \"%1\", \"query\": \"%2\", \"start\": %3, \"max_results\": %4, \"mode\": \"%5\"}")
                                .arg((_foldername.compare("/") == 0) ? "" : _foldername)
                                .arg(query)
                                .arg(start)
                                .arg(max_results)
                                .arg(mode);

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
