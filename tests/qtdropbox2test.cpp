#include <iostream>

#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QSharedPointer>

#include "qdropbox2json.h"
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

#if defined(QDROPBOX2_JSON_TESTS)
/*!
 * \brief QDropbox2Json: Simple string read
 * JSON represents an object with single string value. The test
 * tries to read the string value.
 */
void QtDropbox2Test::jsonCase1()
{
    QDropbox2Json json("{\"string\":\"asdf\"}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getString("string").compare("asdf") == 0, "string value does not match");
}

/*!
 * \brief QDropbox2Json: Simple int read
 * JSON represents an object with a single integer value. The test
 * tries to read that value.
 */
void QtDropbox2Test::jsonCase2()
{
    QDropbox2Json json("{\"int\":1234}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getInt("int") == 1234, "integer value does not match");
}

/*!
 * \brief QDropbox2Json: Injson validity check
 * JSON is invalid. Test confirms invalidity of the JSON.
 */
void QtDropbox2Test::jsonCase3()
{
    QDropbox2Json json("{\"test\":\"foo\"");
    QVERIFY2(!json.isValid(), "injson validity not confirmed");
}

/*!
 * \brief QDropbox2Json: Simple boolean read
 * JSON contains a single boolean value. Test accesses this value.
 */
void QtDropbox2Test::jsonCase4()
{
    QDropbox2Json json("{\"bool\":true}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getBool("bool"), "boolean value does not match");
}

/*!
 * \brief QDropbox2Json: Simple floating point read
 * JSON contains a single double value. Test reads it.
 */
void QtDropbox2Test::jsonCase5()
{
    QDropbox2Json json("{\"double\":14.323667}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getDouble("double"), "double value does not match");
}

/*!
 * \brief QDropbox2Json: Subjson read
 * JSON contains a subjson that is read, but not evaluated.
 */
void QtDropbox2Test::jsonCase6()
{
    QDropbox2Json json("{\"json\": {\"string\":\"abcd\"}}");
    QVERIFY2(json.isValid(), "json validity");

    QDropbox2Json* subjson = json.getJson("json");

    QVERIFY2(subjson!=NULL, "subjson is null");
    QVERIFY2(subjson->isValid(), "subjson invalid");
}

/*!
 * \brief QDropbox2Json: Simple unsigned integer read.
 * JSON contains single unsigned integer that is read.
 */
void QtDropbox2Test::jsonCase7()
{
    QDropbox2Json json("{\"uint\":4294967295}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getUInt("uint") == 4294967295, "unsigned int value does not match");
}

/**
 * @brief QDropbox2Json: Test if clear works correctly
 */
void QtDropbox2Test::jsonCase8()
{
    QDropbox2Json json("{\"uint\":4294967295}");
    QVERIFY2(json.isValid(), "json validity");
    json.clear();
    QVERIFY2(json.getUInt("uint") == 0, "internal list not cleared");
    QVERIFY2(json.strContent().isEmpty(), "json string is not cleared");
}

/**
 * @brief QDropbox2Json: Test if array interpretation and access are working.
 */
void QtDropbox2Test::jsonCase9()
{
    QDropbox2Json json("{\"array\": [1, \"test\", true, 7.3]}");
    QVERIFY2(json.isValid(), "json validity");

    QStringList l = json.getArray("array");
    QVERIFY2(l.size() == 4, "array list has wrong size");
    QVERIFY2(l.at(0).compare("1") == 0, "int element not correctly formatted");
    QVERIFY2(l.at(1).compare("test") == 0, "string element not correctly formatted");
    QVERIFY2(l.at(2).compare("true") == 0, "boolean element not correctly formatted");
    QVERIFY2(l.at(3).compare("7.3") == 0, "double element not correctly formatted");
}

/**
 * @brief QDropbox2Json: Test if json in array is accessible.
 */
void QtDropbox2Test::jsonCase10()
{
    QDropbox2Json json("{\"jsonarray\":[{\"key\":\"value\"}]}");
    QVERIFY2(json.isValid(), "json validity");

    QStringList l = json.getArray("jsonarray");
    QVERIFY2(l.size() == 1, "array list has wrong size");

    QDropbox2Json arrayJson(l.at(0));
    QVERIFY2(arrayJson.isValid(), "json from array is invalid");
    QVERIFY2(arrayJson.getString("key").compare("value") == 0, "json from array contains wrong value");
}

/**
 * @brief QDropbox2Json: Checks if compare() is working by doing a self-comparison.
 */
void QtDropbox2Test::jsonCase11()
{
    QString jsonStr = "{\"int\": 1, \"string\": \"test\", \"bool\": true, \"json\": {\"key\": \"value\"}, "
                      "\"array\": [1, 3.5, {\"arraykey\": \"arrayvalue\"}]}";
    QDropbox2Json json(jsonStr);
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.compare(json) == 0, "comparing the same json resulted in negative comparison");
}

/**
 * @brief QDropbox2Json: Test whether strContent() returns the correct JSON
 * The test case creates a JSON and another JSON that is based on the return value of strContent() of
 * the first JSON. Both JSONs are compared afterwards and expected to be equal.
 */
void QtDropbox2Test::jsonCase12()
{
    QString jsonStr = "{\"int\": 1, \"string\": \"test\", \"bool\": true, \"json\": {\"key\": \"value\"}, "
                      "\"array\": [1, 3.5, {\"arraykey\": \"arrayvalue\"}], \"timestamp\": \"Sat, 21 Aug 2010 22:31:20 +0000\"}";
    QDropbox2Json json(jsonStr);
    QVERIFY2(json.isValid(), "json validity");

    QString jsonContent = json.strContent();
    QDropbox2Json json2(jsonContent);
    QString j2c = json2.strContent();

    int compare = json.compare(json2);

    QVERIFY2(compare == 0, "string content of json is incorrect or compare is broken");
}

/**
 * @brief QDropbox2Json: Setter functions
 * The test verifies if the setter functions are working correctly by setting a value and
 * reading it afterwards.
 */
void QtDropbox2Test::jsonCase13()
{
    QDropbox2Json json;
    json.setInt("testInt", 10);
    QVERIFY2(json.getInt("testInt") == 10, "setInt of json is incorrect");

    json.setUInt("testUInt", 10);
    QVERIFY2(json.getUInt("testUInt") == 10, "setUInt of json is incorrect");

    json.setDouble("testDouble", 10.0);
    QVERIFY2(json.getDouble("testDouble") == 10.0, "setDouble of json is incorrect");

    json.setBool("testBool", true);
    QVERIFY2(json.getBool("testBool"), "setBool of json is incorrect");

    json.setString("testString", "10");
    QVERIFY2(json.getString("testString").compare("10"), "setString of json is incorrect");

    QDateTime time = QDateTime::currentDateTime();
    json.setTimestamp("testTimestamp", time);
    QVERIFY2(json.getTimestamp("testTimestamp").daysTo(time) == 0, "setTimestamp of json is incorrect");
}

/**
 * @brief QDropbox2Json: [] in strings
 * Verify that square brackets in strings are working correctly.
 */
void QtDropbox2Test::jsonCase14()
{
    QDropbox2Json json("{\"string\": \"[asdf]abcd\"}");  
    QVERIFY2(json.isValid(), "json could not be parsed");
    QVERIFY2(json.getString("string").compare("[asdf]abcd") == 0, "square brackets in string not parsed correctly");
}

/**
 * @brief QDropbox2Json: {} in strings
 * Verify that curly brackets within a string are parsed correctly
 */
void QtDropbox2Test::jsonCase15()
{
    QDropbox2Json json("{\"string\": \"{asdf}abcd\"}");  
    QVERIFY2(json.isValid(), "json could not be parsed");
    QVERIFY2(json.getString("string").compare("{asdf}abcd") == 0, 
	     QString("curly brackets in string not parsed correctly [%1]").arg(json.getString("string")).toStdString().c_str());
}

/*!
 * \brief QDropbox2Json: 64-bit integer read.
 */
void QtDropbox2Test::jsonCase16()
{
    QDropbox2Json json("{\"int64\":9223372036854775807}");
    QVERIFY2(json.isValid(), "json validity");
    QVERIFY2(json.getInt64("int64") == 9223372036854775807, "64-bit int value does not match");
}

/*!
 * \brief QDropbox2Json: 64-bit unsigned integer read.
 */
void QtDropbox2Test::jsonCase17()
{
    // overflow max signed 64-bit by one to force an unsigned value
    QDropbox2Json json("{\"uint64\":9223372036854775808}");
    QVERIFY2(json.isValid(), "json validity");
	QVERIFY2(json.getUInt64("uint64") == 9223372036854775808U, "64-bit unsigned int value does not match");
}
#endif      // QDROPBOX2_JSON_TESTS

#if defined(QDROPBOX2_ACCOUNT_TESTS)
void QtDropbox2Test::accountUser()
{
    QVERIFY(db2 != nullptr);

    // Retrieve APIv2 account user information (synchronous)
    QDropbox2User info;
    QCOMPARE(db2->userInfo(info), true);

    //QTextStream out(stdout);
    //out << info.displayName() << ":\n";
    //out << "\t            id: " << info.id() << "\n";
    //out << "\t          type: " << info.type() << "\n";
    //out << "\t          name: " << info.displayName() << "\n";
    //out << "\t         email: " << info.email() << "\n";
    //out << "\t emailVerified: " << (info.emailVerified() ? "true" : "false") << "\n";
    //out << "\t    isDisabled: " << (info.isDisabled() ? "true" : "false") << "\n";
    //out << "\t        locale: " << info.locale() << "\n";
    //out << "\t  referralLink: " << info.referralLink().toString() << "\n";
    //out << "\t      isPaired: " << (info.isPaired() ? "true" : "false") << "\n";
    //out << "\t       country: " << info.country() << "\n";
}

void QtDropbox2Test::accountUsage()
{
    QVERIFY(db2 != nullptr);

    // Retrieve APIv2 account usage information (synchronous)
    QDropbox2Usage info;
    QCOMPARE(db2->usageInfo(info), true);

    //QTextStream out(stdout);
    //out << "\t          used: " << info.used() << "\n";
    //out << "\t     allocated: " << info.allocated() << "\n";
    //out << "\tallocationType: " << info.allocationType() << "\n";
}
#endif      // QDROPBOX2_ACCOUNT_TESTS

#if defined(QDROPBOX2_FOLDER_TESTS)
void QtDropbox2Test::createFolder()
{
    QVERIFY(db2 != nullptr);

    // Create a new folder
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
    QCOMPARE(db_folder.create(), true);
}

void QtDropbox2Test::copyFolder()
{
    QVERIFY(db2 != nullptr);

    // Copys the folder created in createFolder() to another folder
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
    QCOMPARE(db_folder.copy("/QtDropbox2Folder"), true);
}

void QtDropbox2Test::removeFolder1()
{
    QVERIFY(db2 != nullptr);

    // Removes the folder created in createFolder()
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);
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
    QDropbox2Folder db_folder("/", db2);
    QDropbox2Folder::ContentsList contents;
    QCOMPARE(db_folder.contents(contents), true);

    //QTextStream out(stdout);
    //foreach(const QDropbox2EntityInfo& entry, contents)
    //{
    //    out << entry.path() << ":\n";
    //    if(entry.isDeleted())
    //        out << "\t     isDeleted: true\n";
    //    else
    //    {
    //        out << "\t            id: " << entry.id() << "\n";
    //        out << "\tclientModified: " << entry.clientModified().toString() << "\n";
    //        out << "\tserverModified: " << entry.serverModified().toString() << "\n";
    //        out << "\t  revisionHash: " << entry.revisionHash() << "\n";
    //        out << "\t         bytes: " << entry.bytes() << "\n";
    //        out << "\t          size: " << entry.size() << "\n";
    //        out << "\t      isShared: " << (entry.isShared() ? "true" : "false") << "\n";
    //        out << "\t   isDirectory: " << (entry.isDirectory() ? "true" : "false") << "\n";
    //    }
    //    out.flush();
    //}
}

void QtDropbox2Test::checkForChanges()
{
    QVERIFY(db2 != nullptr);

    // Check for changes in a folder (synchronous)
    QDropbox2Folder db_folder(QDROPBOX2_FOLDER, db2);   // creation retrieves the latest cursor
    QThread::msleep(1000);
    QDropbox2Folder::ContentsList changes;
    QCOMPARE(db_folder.hasChanged(changes), false);

    //QTextStream out(stdout);
    //out << "Changes were ";
    //QDropbox2Folder::ContentsList changes;
    //if(!db_folder.hasChanged(changes))
    //    out << "not ";
    //out << "detected in the folder.\n";
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
    QVERIFY(db2 != nullptr);

    // Copy a file
    QDropbox2File db_file(db_path, db2);
    Q_ASSERT(db_file.copy(QString("/QtDropbox2.%1").arg(suffix)));
}

void QtDropbox2Test::moveFile()
{
    QVERIFY(db2 != nullptr);

    // Move a file
    QString from_name = QString("/QtDropbox2.%1").arg(suffix);
    QString to_name = QString("/QtDropbox2_2.%1").arg(suffix);
    QDropbox2File db_file(from_name, db2);
    QVERIFY(db_file.error() == 0);
    QVERIFY(db_file.move(to_name));
}

void QtDropbox2Test::getRevisions()
{
    QVERIFY(db2 != nullptr);

    // Retrieve file revisions
    QDropbox2File db_file(db_path, db2);
    QDropbox2File::RevisionsList revisions;
    QCOMPARE(db_file.revisions(revisions, 5), true);

    //QTextStream out(stdout);
    //foreach(const QDropbox2EntityInfo& entry, revisions)
    //{
    //    out << db_file.filename() << ":\n";
    //    if(entry.isDeleted())
    //        out << "\t     isDeleted: true\n";
    //    else
    //    {
    //        out << "\t            id: " << entry.id() << "\n";
    //        out << "\tclientModified: " << entry.clientModified().toString() << "\n";
    //        out << "\tserverModified: " << entry.serverModified().toString() << "\n";
    //        out << "\t  revisionHash: " << entry.revisionHash() << "\n";
    //        out << "\t         bytes: " << entry.bytes() << "\n";
    //        out << "\t          size: " << entry.size() << "\n";
    //        out << "\t          path: " << entry.path() << "\n";
    //        out << "\t      isShared: " << (entry.isShared() ? "true" : "false") << "\n";
    //        out << "\t   isDirectory: " << (entry.isDirectory() ? "true" : "false") << "\n";
    //    }
    //}
}

void QtDropbox2Test::getLink()
{
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
    QDropbox2Folder::ContentsList contents;
    QCOMPARE(db_folder.search(contents, QString(".%1").arg(suffix)), true);

    //QTextStream out(stdout);
    //foreach(const QDropbox2EntityInfo& entry, contents)
    //{
    //    if(entry.isDeleted())
    //        out << "\t     isDeleted: true\n";
    //    else
    //    {
    //        out << "\t            id: " << entry.id() << "\n";
    //        out << "\tclientModified: " << entry.clientModified().toString() << "\n";
    //        out << "\tserverModified: " << entry.serverModified().toString() << "\n";
    //        out << "\t  revisionHash: " << entry.revisionHash() << "\n";
    //        out << "\t         bytes: " << entry.bytes() << "\n";
    //        out << "\t          size: " << entry.size() << "\n";
    //        out << "\t          path: " << entry.path() << "\n";
    //        out << "\t      isShared: " << (entry.isShared() ? "true" : "false") << "\n";
    //        out << "\t   isDirectory: " << (entry.isDirectory() ? "true" : "false") << "\n";
    //    }
    //    out.flush();
    //}
}

void QtDropbox2Test::downloadFile()
{
    QVERIFY(db2 != nullptr);

    // Download a file without signals
    QDropbox2File db_file(db_path, db2);
    //connect(&db_file, &QDropbox2File::signal_downloadProgress, [](qint64 bytesReceived, qint64 bytesTotal)
    //            {
    //                QString percent = QString::number((qint32)((bytesReceived / (bytesTotal * 1.0)) * 100.0));
    //                std::cout << "Download: " << percent.toUtf8().constData() << "%" << "\r";
    //                std::cout.flush();
    //            });

    // opening the file causes it to be downloaded and cached locally
    QCOMPARE(db_file.open(QIODevice::ReadOnly), true);
    QDropbox2EntityInfo info = db_file.metadata();
    QCOMPARE(info.isValid(), true);
    QCOMPARE(info.path(), db_file.filename());

    QByteArray data = db_file.readAll();
    QByteArray local_md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    QCOMPARE(md5, local_md5);

    //QTextStream out(stdout);
    //out << db_file.filename() << ":\n";
    //if(entry.isDeleted())
    //    out << "\t     isDeleted: true\n";
    //else
    //{
    //    out << "\t            id: " << info.id() << "\n";
    //    out << "\tclientModified: " << info.clientModified().toString() << "\n";
    //    out << "\tserverModified: " << info.serverModified().toString() << "\n";
    //    out << "\t  revisionHash: " << info.revisionHash() << "\n";
    //    out << "\t         bytes: " << info.bytes() << "\n";
    //    out << "\t          size: " << info.size() << "\n";
    //    out << "\t          path: " << info.path() << "\n";
    //    out << "\t      isShared: " << (info.isShared() ? "true" : "false") << "\n";
    //    out << "\t   isDirectory: " << (info.isDirectory() ? "true" : "false") << "\n";
    //}
    //out.flush();
}

void QtDropbox2Test::removeFile()
{
    QVERIFY(db2 != nullptr);

    // Remove all the files and folders we created (clean up)
    QString moved_name = QString("/QtDropbox2_2.%1").arg(suffix);

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

///**
// * @brief Prompt the user for authorization.
// */
//void QtDropbox2Test::authorizeApplication(QDropbox2* d)
//{
//    QTextStream out(stdout);
//
//    out << "##########################################" << endl;
//    out << "# You need to grant this test access to  #" << endl;
//    out << "# your Dropbox!                          #" << endl;
//    out << "#                                        #" << endl;
//    out << "# Go to the following URL to do so.      #" << endl;
//    out << "##########################################" << endl << endl;
//
//    out << "URL: " << d->authorizeLink().toString() << endl;
//    QDesktopServices::openUrl(d->authorizeLink());
//    out << "Wait for authorized the application!";
//    out << endl;
//
//    while(d->requestAccessTokenAndWait() == false)
//    {
//        QThread::msleep(1000);
//    }
//}
//
///**
// * @brief Connect a QDropbox to the Dropbox service
// * @param d QDropbox object to be connected
// * @param m Authentication Method
// * @return <code>true</code> on success
// */
//bool QtDropbox2Test::connectDropbox(QDropbox2 *d, QDropbox2::OAuthMethod m)
//{
//    QFile tokenFile("tokens");
//
//    if(tokenFile.exists()) // reuse old tokens
//    {
//        if(tokenFile.open(QIODevice::ReadOnly|QIODevice::Text))
//        {
//            QTextStream instream(&tokenFile);
//            QString token = instream.readLine().trimmed();
//            QString secret = instream.readLine().trimmed();
//            if(!token.isEmpty() && !secret.isEmpty())
//            {
//                d->setToken(token);
//                d->setTokenSecret(secret);
//                tokenFile.close();
//                return true;
//            }
//        }
//        tokenFile.close();
//    }
//
//    // acquire new token
//    if(!d->requestTokenAndWait())
//    {
//        qCritical() << "error on token request";
//        return false;
//    }
//
//    d->setAuthMethod(m);
//    if(!d->requestAccessTokenAndWait())
//    {
//        int i = 0;
//        for(;i<3; ++i) // we try three times
//        {
//            if(d->error() != QDropbox::TokenExpired)
//                break;
//            authorizeApplication(d);
//        }
//
//       if(i>3)
//       {
//           qCritical() <<  "too many tries for authentication";
//           return false;
//       }
//
//        if(d->error() != QDropbox::NoError)
//        {
//           qCritical() << "Error: " << d->error() << " - " << d->errorString() << endl;
//           return false;
//        }
//    }
//
//    if(!tokenFile.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
//        return true;
//
//    QTextStream outstream(&tokenFile);
//    outstream << d->token() << endl;
//    outstream << d->tokenSecret() << endl;
//    tokenFile.close();
//    return true;
//}

QTEST_MAIN(QtDropbox2Test)
