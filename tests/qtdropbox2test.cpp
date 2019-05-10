#include <iostream>

#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QSharedPointer>
#include <QSignalSpy>

#include "qtdropbox2test.h"

typedef QMap<QString, QSharedPointer<QDropbox2EntityInfo>> QDropbox2EntityInfoMap;

QtDropbox2Test::QtDropbox2Test()
#if defined(QDROPBOX2_ACCOUNT_TESTS) || defined(QDROPBOX2_FOLDER_TESTS) || defined(QDROPBOX2_FILE_TESTS)
    : db2(nullptr)
#endif
{
}

void QtDropbox2Test::initTestCase()
{
#if defined(QDROPBOX2_ACCOUNT_TESTS) || defined(QDROPBOX2_FOLDER_TESTS) || defined(QDROPBOX2_FILE_TESTS)
    qRegisterMetaType<QDropbox2User>();
    qRegisterMetaType<QDropbox2Usage>();

  #if defined(QDROPBOX2_ACCESS_TOKEN)
    db2 = new QDropbox2(QDROPBOX2_ACCESS_TOKEN, this);
  #elif defined(QDROPBOX2_APP_KEY) && defined(QDROPBOX2_APP_SECRET)
    #error This interface does not currently work!
    // this requires an interaction with the Dropbox API...
    db2 = new QDropbox2(QDROPBOX2_APP_KEY, QDROPBOX2_APP_SECRET, this);
    // ...so check for success
    QVERIFY(db2->error() == QDropbox2::NoError);
  #else
    #error You must define either a QDROPBOX2_ACCESS_TOKEN or QDROPBOX2_APP_KEY/QDROPBOX2_APP_SECRET values!
  #endif
#endif
}

void QtDropbox2Test::cleanupTestCase()
{
#if defined(QDROPBOX2_ACCOUNT_TESTS) || defined(QDROPBOX2_FOLDER_TESTS) || defined(QDROPBOX2_FILE_TESTS)
    db2->deleteLater();
#endif
}

#if defined(QDROPBOX2_ACCOUNT_TESTS)
void QtDropbox2Test::accountUser_sync()
{
    QVERIFY(db2 != nullptr);

    // Retrieve APIv2 account user information (synchronous)
    QDropbox2User info;
    QCOMPARE(db2->userInfo(info), true);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << info.displayName() << ":\n";
    out << "\t            id: " << info.id() << "\n";
    out << "\t          type: " << info.type() << "\n";
    out << "\t          name: " << info.displayName() << "\n";
    out << "\t         email: " << info.email() << "\n";
    out << "\t emailVerified: " << (info.emailVerified() ? "true" : "false") << "\n";
    out << "\t    isDisabled: " << (info.isDisabled() ? "true" : "false") << "\n";
    out << "\t        locale: " << info.locale() << "\n";
    out << "\t  referralLink: " << info.referralLink().toString() << "\n";
    out << "\t      isPaired: " << (info.isPaired() ? "true" : "false") << "\n";
    out << "\t       country: " << info.country() << "\n";
#endif
}

void QtDropbox2Test::accountUser_async()
{
    QVERIFY(db2 != nullptr);

    QSignalSpy spy(db2, &QDropbox2::signal_userInfoReceived);

    // Retrieve APIv2 account user information (asynchronous)
    QCOMPARE(db2->userInfo(), true);

    QVERIFY(spy.wait());

    QTRY_COMPARE(spy.count(), 1);   // make sure the signal was emitted exactly one time
    QDropbox2User info = qvariant_cast<QDropbox2User>(spy.at(0).at(0));
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << info.displayName() << ":\n";
    out << "\t            id: " << info.id() << "\n";
    out << "\t          type: " << info.type() << "\n";
    out << "\t          name: " << info.displayName() << "\n";
    out << "\t         email: " << info.email() << "\n";
    out << "\t emailVerified: " << (info.emailVerified() ? "true" : "false") << "\n";
    out << "\t    isDisabled: " << (info.isDisabled() ? "true" : "false") << "\n";
    out << "\t        locale: " << info.locale() << "\n";
    out << "\t  referralLink: " << info.referralLink().toString() << "\n";
    out << "\t      isPaired: " << (info.isPaired() ? "true" : "false") << "\n";
    out << "\t       country: " << info.country() << "\n";
#endif
}

void QtDropbox2Test::accountUsage_sync()
{
    QVERIFY(db2 != nullptr);

    // Retrieve APIv2 account usage information (synchronous)
    QDropbox2Usage info;
    QCOMPARE(db2->usageInfo(info), true);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << "\t          used: " << info.used() << "\n";
    out << "\t     allocated: " << info.allocated() << "\n";
    out << "\tallocationType: " << info.allocationType() << "\n";
#endif
}

void QtDropbox2Test::accountUsage_async()
{
    QVERIFY(db2 != nullptr);

    QSignalSpy spy(db2, &QDropbox2::signal_usageInfoReceived);

    // Retrieve APIv2 account usage information (asynchronous)
    QCOMPARE(db2->usageInfo(), true);

    QVERIFY(spy.wait());

    QTRY_COMPARE(spy.count(), 1);   // make sure the signal was emitted exactly one time
    QDropbox2Usage info = qvariant_cast<QDropbox2Usage>(spy.at(0).at(0));
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << "\t          used: " << info.used() << "\n";
    out << "\t     allocated: " << info.allocated() << "\n";
    out << "\tallocationType: " << info.allocationType() << "\n";
#endif
}
#endif      // QDROPBOX2_ACCOUNT_TESTS

#if defined(QDROPBOX2_FOLDER_TESTS)
void QtDropbox2Test::createFolder()
{
    QVERIFY(db2 != nullptr);

    // Create a new folder
    QDropbox2Folder db_folder(QString("%1/%2").arg(QDROPBOX2_FOLDER).arg(QDROPBOX2_FOLDER_NEW), db2);
    qInfo()<<"Create Folder: "<<QString("%1/%2").arg(QDROPBOX2_FOLDER).arg(QDROPBOX2_FOLDER_NEW);
    QCOMPARE(db_folder.create(), true);
}

void QtDropbox2Test::copyFolder()
{
    QVERIFY(db2 != nullptr);

    // Copys the folder created in createFolder() to another folder
    QDropbox2Folder db_folder(QString("%1/%2").arg(QDROPBOX2_FOLDER).arg(QDROPBOX2_FOLDER_NEW), db2);
    qInfo()<<"Copy Folder: "<<QString("%1/%2").arg(QDROPBOX2_FOLDER).arg(QDROPBOX2_FOLDER_NEW);
    QCOMPARE(db_folder.copy("/QtDropbox2Folder"), true);
}

void QtDropbox2Test::removeFolder1()
{
    QVERIFY(db2 != nullptr);

    // Removes the folder created in createFolder()
    QDropbox2Folder db_folder("/QtDropbox2Folder", db2);
    QCOMPARE(db_folder.remove(), true);
}

void QtDropbox2Test::moveFolder()
{
    QVERIFY(db2 != nullptr);

    // Moves the folder created in copyFolder() to the deleted QDROPBOX2_FOLDER
    QDropbox2Folder db_folder("/QtDropbox2Folder", db2);
    QCOMPARE(db_folder.move(QDROPBOX2_FOLDER), true);
}

void QtDropbox2Test::getContents()
{
    QVERIFY(db2 != nullptr);

    // Retrieves the contents of the root folder
    QDropbox2Folder db_folder("", db2);
    FoldersModel *contents = new FoldersModel();
    QCOMPARE(db_folder.contents(contents), true);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out<<"\n\n";
//    foreach(const QDropbox2EntityInfo *entry, contents)
    for (int i=0; i<contents->count();i++)
    {
        const QDropbox2EntityInfo *entry = contents->at(i);
        out << entry->path() << ":\n";
        if(entry->isDeleted())
            out << "\t     isDeleted: true\n";
        else
        {
            out << "\t            id: " << entry->id() << "\n";
            out << "\tclientModified: " << entry->clientModified().toString() << "\n";
            out << "\tserverModified: " << entry->serverModified().toString() << "\n";
            out << "\t  revisionHash: " << entry->revisionHash() << "\n";
            out << "\t         bytes: " << entry->bytes() << "\n";
            out << "\t          size: " << entry->size() << "\n";
            out << "\t      isShared: " << (entry->isShared() ? "true" : "false") << "\n";
            out << "\t   isDirectory: " << (entry->isDirectory() ? "true" : "false") << "\n";
        }
        out.flush();
    }
#endif
}

void QtDropbox2Test::checkForChanges()
{
    QVERIFY(db2 != nullptr);

    // Check for changes in a folder (synchronous)
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);   // creation retrieves the latest cursor
    QThread::msleep(1000);
    FoldersModel *changes = new FoldersModel();
    QCOMPARE(db_folder.hasChanged(changes), false);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << "Changes were ";
//    QDropbox2Folder::ContentsList changes;
    if(!db_folder.hasChanged(changes))
        out << "not ";
    out << "detected in the folder.\n";
#endif
}

void QtDropbox2Test::waitForChanges()
{
    QVERIFY(db2 != nullptr);

    // Wait for changes in a folder
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
    QCOMPARE(db_folder.waitForChanged(5), false);
}

// If we are going to fall through to the file tests, leave the folder
// in place for our usage there...
#if !defined(QDROPBOX2_FILE_TESTS)
void QtDropbox2Test::removeFolder2()
{
    QVERIFY(db2 != nullptr);

    // Removes the folder created in moveFolder()
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
    QCOMPARE(db_folder.remove(), true);
}
#endif
#endif      // QDROPBOX2_FOLDER_TESTS

///**
// * @brief QDropbox: delta
// * This test connects to Dropbox and tests the delta API.
// *
// * <b>You are required to authorize
// * the application for access! The Authorization URI will be printed to you and manual interaction
// * is required to pass this test!</b>
// */
//void QtDropbox2Test::dropbox2Case2()
//{
//    QTextStream strout(stdout);
//    QDropbox dropbox(APP_KEY, APP_SECRET);
//    QVERIFY2(connectDropbox(&dropbox, QDropbox::Plaintext), "connection error");
//
//    QString cursor = "";
//    bool hasMore = true;
//    QDropbox2EntityInfoMap file_cache;
//
//    strout << "requesting delta...\n";
//    do
//    {
//        QDropboxDeltaResponse r = dropbox.requestDeltaAndWait(cursor, "");
//        cursor = r.getNextCursor();
//        hasMore = r.hasMore();
//
//        const QDropboxDeltaEntryMap entries = r.getEntries();
//        for(QDropboxDeltaEntryMap::const_iterator i = entries.begin(); i != entries.end(); i++)
//        {
//            if(i.value().isNull())
//            {
//                file_cache.remove(i.key());
//            }
//            else
//            {
//                strout << "inserting file " << i.key() << "\n";
//                file_cache.insert(i.key(), i.value());
//            }
//        }
//
//    } while (hasMore);
//    strout << "next cursor: " << cursor << "\n";
//    for(QDropbox2EntityInfoMap::const_iterator i = file_cache.begin(); i != file_cache.end(); i++)
//    {
//        strout << "file " << i.key() << " last modified " << i.value()->clientModified().toString() << "\n";
//    }
//
//    return;
//}

#if defined(QDROPBOX2_FILE_TESTS)
#if !defined(QDROPBOX2_FOLDER_TESTS)
#define QDROPBOX2_FOLDER "/QtDropbox2Folder"
#endif
void QtDropbox2Test::uploadFile()
{
//    return;
    QVERIFY(db2 != nullptr);

#if !defined(QDROPBOX2_FOLDER_TESTS)
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
    QCOMPARE(db_folder.create(), true);
#endif

    // Upload a file
    QCOMPARE(QFile::exists(QDROPBOX2_FILE), true);
    QFileInfo info(QDROPBOX2_FILE);
    filename = info.fileName();
    suffix = info.suffix();

    // Up to 150MB in size...
    QCOMPARE(info.size() < 150*1024*1024, true);

    QFile local_file(QDROPBOX2_FILE);
    QCOMPARE(local_file.open(QIODevice::ReadOnly), true);
    QByteArray data = local_file.readAll();
    local_file.close();

    // calculate MD5 and save it for the download test
    md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);

    db_path = QString("%1/%2").arg(QDROPBOX2_FOLDER).arg(filename);
    qInfo()<<"Upload to "<<db_path;
    QDropbox2File db_file(db_path, db2);
    QVERIFY(db_file.error() == 0);
    //connect(&db_file, &QDropbox2File::signal_uploadProgress, [](qint64 bytesSend, qint64 bytesTotal)
    //            {
    //                QString percent = QString::number((qint32)((bytesSent / (bytesTotal * 1.0)) * 100.0));
    //                std::cout << "Upload: " << percent.toUtf8().constData() << "%" << "\r";
    //                std::cout.flush();
    //            });
    db_file.setOverwrite();
    QCOMPARE(db_file.open(QIODevice::WriteOnly|QIODevice::Truncate), true);
    db_file.write(data);
    QCOMPARE(db_file.flush(), true);    // send buffered data to Dropbox
    db_file.close();
}

void QtDropbox2Test::copyFile()
{
//    return;
    QVERIFY(db2 != nullptr);

    // Copy a file
    QDropbox2File db_file(db_path, db2);
    qInfo()<<"Should Copy "<<db_path<<" to "<<QString("/QtDropbox2.%1").arg(suffix);
    Q_ASSERT(db_file.copy(QString("/QtDropbox2.%1").arg(suffix)));
}

void QtDropbox2Test::moveFile()
{
    QVERIFY(db2 != nullptr);

    // Move a file
    QString from_name = QString("/QtDropbox2.%1").arg(suffix);
    QString to_name = QString("/QtDropbox2_2.%1").arg(suffix);
    qInfo()<<"Move file "<<from_name<<" to "<<to_name;
    QDropbox2File db_file(from_name, db2);
    QVERIFY(db_file.error() == 0);
    QVERIFY(db_file.move(to_name));
}

void QtDropbox2Test::getRevisions()
{
//    return;
    QVERIFY(db2 != nullptr);

    // Retrieve file revisions
    QDropbox2File db_file(db_path, db2);
    QDropbox2File::RevisionsList revisions;
    QCOMPARE(db_file.revisions(revisions, 5), true);
//#ifdef SHOW_OUTPUT
//    QTextStream out(stdout);
//    foreach(const QDropbox2EntityInfo& entry, revisions)
//    {
//        out << db_file.filename() << ":\n";
//        if(entry.isDeleted())
//            out << "\t     isDeleted: true\n";
//        else
//        {
//            out << "\t            id: " << entry.id() << "\n";
//            out << "\tclientModified: " << entry.clientModified().toString() << "\n";
//            out << "\tserverModified: " << entry.serverModified().toString() << "\n";
//            out << "\t  revisionHash: " << entry.revisionHash() << "\n";
//            out << "\t         bytes: " << entry.bytes() << "\n";
//            out << "\t          size: " << entry.size() << "\n";
//            out << "\t          path: " << entry.path() << "\n";
//            out << "\t      isShared: " << (entry.isShared() ? "true" : "false") << "\n";
//            out << "\t   isDirectory: " << (entry.isDirectory() ? "true" : "false") << "\n";
//        }
//    }
//#endif
}

void QtDropbox2Test::getLink()
{
//    return;
    QVERIFY(db2 != nullptr);

    // Get a (temporary) streaming link for a file
    QDropbox2File db_file(db_path, db2);
    QUrl url = db_file.temporaryLink();
    QVERIFY(url.isValid());
    qDebug() << url.toDisplayString() << endl;
}

void QtDropbox2Test::search()
{
    QVERIFY(db2 != nullptr);

    // Search a folder (synchronous)
    QFileInfo info(db_path);
    QString path = info.path();

    QDropbox2Folder db_folder(path, db2);
    FoldersModel *contents = new FoldersModel();
    QCOMPARE(db_folder.search(contents, QString(".%1").arg(suffix)), true);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    for (int i=0;i<contents->count();i++){
//    foreach(const QDropbox2EntityInfo *entry, contents)
//    {
const QDropbox2EntityInfo *entry = contents->at(i);
        if(entry->isDeleted())
            out << "\t     isDeleted: true\n";
        else
        {
            out << "\t            id: " << entry->id() << "\n";
            out << "\tclientModified: " << entry->clientModified().toString() << "\n";
            out << "\tserverModified: " << entry->serverModified().toString() << "\n";
            out << "\t  revisionHash: " << entry->revisionHash() << "\n";
            out << "\t         bytes: " << entry->bytes() << "\n";
            out << "\t          size: " << entry->size() << "\n";
            out << "\t          path: " << entry->path() << "\n";
            out << "\t      isShared: " << (entry->isShared() ? "true" : "false") << "\n";
            out << "\t   isDirectory: " << (entry->isDirectory() ? "true" : "false") << "\n";
        }
        out.flush();
    }
#endif
}

void QtDropbox2Test::downloadFile()
{
    QVERIFY(db2 != nullptr);

    // Download a file without signals
    QDropbox2File db_file(db_path, db2);
#ifdef SHOW_OUTPUT
//    connect(&db_file, &QDropbox2File::signal_downloadProgress, [](qint64 bytesReceived, qint64 bytesTotal)
//                {
//                    QString percent = QString::number((qint32)((bytesReceived / (bytesTotal * 1.0)) * 100.0));
//                    std::cout << "Download: " << percent.toUtf8().constData() << "%" << "\r";
//                    std::cout.flush();
//                });
#endif
    // opening the file causes it to be downloaded and cached locally
    db_file.setFilename("/CT_v2.2.3 - Instant.zip");
    QCOMPARE(db_file.open(QIODevice::ReadOnly), true);
    QDropbox2EntityInfo info(db_file.metadata());
    QCOMPARE(info.id().isEmpty(), false);   // make sure metadata is valid
    QCOMPARE(info.path(), db_file.filename());

    QByteArray data = db_file.readAll();
    QByteArray local_md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
//    QCOMPARE(md5, local_md5);
#ifdef SHOW_OUTPUT
    QTextStream out(stdout);
    out << db_file.filename() << ":\n";
    if(info.isDeleted())
        out << "\t     isDeleted: true\n";
    else
    {
        out << "\t            id: " << info.id() << "\n";
        out << "\tclientModified: " << info.clientModified().toString() << "\n";
        out << "\tserverModified: " << info.serverModified().toString() << "\n";
        out << "\t  revisionHash: " << info.revisionHash() << "\n";
        out << "\t         bytes: " << info.bytes() << "\n";
        out << "\t          size: " << info.size() << "\n";
        out << "\t          path: " << info.path() << "\n";
        out << "\t      isShared: " << (info.isShared() ? "true" : "false") << "\n";
        out << "\t   isDirectory: " << (info.isDirectory() ? "true" : "false") << "\n";
    }
    out.flush();
#endif
}

void QtDropbox2Test::removeFile()
{
    QVERIFY(db2 != nullptr);

    // Remove all the files and folders we created (clean up)
    QString moved_name = QString("/QtDropbox2_2.%1").arg(suffix);
    qInfo()<<"Should Remove: "<<moved_name;
    QDropbox2File db_file(moved_name, db2);
    QCOMPARE(db_file.remove(), true);

    // In case we created a subdirectory chain, remove the whole thing
    // in one shot by killing the top folder
    QStringList items = QString(QDROPBOX2_FOLDER).split('/');
    while(items.front().isEmpty())
        items.pop_front();
    QVERIFY(items.length() != 0);       // this shouldn't happen
    QString top_path = QString("/%1").arg(items[0]);
    QDropbox2Folder db_folder(top_path, db2);
    QCOMPARE(db_folder.remove(), true);
}
#endif      // QDROPBOX2_FILE_TESTS

QTEST_MAIN(QtDropbox2Test)
