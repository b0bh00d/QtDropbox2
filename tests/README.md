# QtDropbox2: Unit Tests

## Introduction
This subproject builds a test application that verifies, as much as possible, if
QtDropbox2 is working correctly.  In order to compile and execute the tests, you
will need to create a custom header file called config.h.  This file defines
several values that provide the Dropbox unit tests with the information needed
to access a Dropbox account.

## Dropbox OAuth v1 Access
The APIv2 interface continues to support using key and secret values to generate
an OAuth2 access tokens.  You can employ this facility by defining these values
in the configuration header.

Example:

    #define QDROPBOX2_APP_KEY "qievr8hamyg6ndck"
    #define QDROPBOX2_APP_SECRET "qomoftv0472git7"

Please note that this interface does not currently work due to my confusion
concerning the need to first have the bearer access token, which in itself can
be used to interact with the Dropbox account.  Avoid using this method of
construction for now.

## Dropbox Application Access Token
If you have a pre-generated access token for application access, you define it
in the config.h header as follows:

Example:

    #define QDROPBOX2_ACCESS_TOKEN "fKdffKJJKJSSSSjskdjjIsdd-sJJKKKSsssdjk..."

If provided, this value will take precedence over the QDROPBOX2_APP_KEY and
QDROPBOX2_APP_SECRET values.

You can enable your application, and generate the Access Token, from the
Developer App Console, https://www.dropbox.com/developers/apps.

## Enabling Tests
You need to define certain values to enable certain tests.  For example, in
order to enable the QDropbox2 account tests, you will need to define
QDROPBOX2_ACCOUNT_TESTS.

All of the test subsystems require interaction with a Dropbox account, so you
will need to define one or more of QDROPBOX2_ACCOUNT_TESTS, QDROPBOX2_FOLDER_TESTS
or QDROPBOX2_FILE_TESTS.  Enabling any of these will require the presence of an
authorizing access token (see the previous section).

Example:

    #define QDROPBOX2_ACCOUNT_TESTS
    #define QDROPBOX2_FOLDER_TESTS
    #define QDROPBOX2_FILE_TESTS
    #define QDROPBOX2_ACCESS_TOKEN "fKdffKJJKJSSSSjskdjjIsdd-sJJKKKSsssdjk22jkss)9923kjsssfsaaFFSAjJ"

For folder tests, you will need to define a folder name (or path) in the
QDROPBOX2_FOLDER:

    ...
    #define QDROPBOX2_FOLDER_TESTS
    #define QDROPBOX2_FOLDER "/This/Is/My/Test/Folder"
    ...

For file tests, you will need to define a local file to be uploaded/downloaded:

    ...
    #define QDROPBOX2_FILE_TESTS
    #define QDROPBOX2_FILE "X:/MyTestFile.mp4""
    ...

Please note that the framework is currently limited to a single file of no more
than 150MB in size for uploading.  Files larger than that will cause a failure
of the upload interface.  This is because the "upload_session" REST interface
has not yet been implemented that would allow files larger than this limit to
be exchanged.

## Build & Execute
The projects in this repository assume you will be using QtCreator to build
them.  If you build from the command line, you may need some experimentation.

In order to execute the unit tests, you have to build QtDropbox2 first.