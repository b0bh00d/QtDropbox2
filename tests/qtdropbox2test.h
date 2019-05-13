#pragma once

#include <QtTest/QtTest>
#include <QDesktopServices>
#include <QThread>
#include <QCryptographicHash>
#include <QByteArray>

#include "qdropbox2.h"
#include "qdropbox2file.h"
#include "qdropbox2folder.h"
#include "config.h"

class QtDropbox2Test : public QObject
{
    Q_OBJECT

public:
    QtDropbox2Test();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

#if defined(QDROPBOX2_ACCOUNT_TESTS)
    void accountUser_sync();
    void accountUser_async();
    void accountUsage_sync();
    void accountUsage_async();
#endif

#if defined(QDROPBOX2_FOLDER_TESTS)
    void createFolder();
    void copyFolder();
    void moveFolder();
    void getContents();
    void removeFolder1();
    void checkForChanges();
    void waitForChanges();
#if !defined(QDROPBOX2_FILE_TESTS)
    // leave the folder in place for the QDROPBOX2_FILE_TESTS to use and clean up
    void removeFolder2();
#endif
#endif

#if defined(QDROPBOX2_FILE_TESTS)
    void uploadFile();
    void copyFile();
    void moveFile();
    void getRevisions();
    void getLink();
    void search();
    void downloadFile();
    void removeFile();
#endif
//#if defined (QDROPBOX2_TEST_DOWNLOAD)
//    void downloadFile();
//#endif
private:        // data members
#if defined(QDROPBOX2_ACCOUNT_TESTS) || defined(QDROPBOX2_FOLDER_TESTS) || defined(QDROPBOX2_FILE_TESTS)
    QDropbox2*  db2;
#endif
#if defined(QDROPBOX2_FILE_TESTS)
    QByteArray  md5;
    QString     filename;
    QString     suffix;
    QString     db_path;
#endif
};

#if defined(QDROPBOX2_ACCOUNT_TESTS)
Q_DECLARE_METATYPE(QDropbox2User)
Q_DECLARE_METATYPE(QDropbox2Usage)
#endif
