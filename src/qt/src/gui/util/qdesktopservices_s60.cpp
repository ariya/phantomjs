/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <qcoreapplication.h>
#include <qdir.h>
#include <qurl.h>
#include <private/qcore_symbian_p.h>

#include <f32file.h>                // TDriveUnit etc
#include <pathinfo.h>               // PathInfo

#ifndef USE_SCHEMEHANDLER
#ifdef Q_WS_S60
// This flag changes the implementation to use S60 CDcoumentHandler
// instead of apparc when opening the files
#define USE_DOCUMENTHANDLER
#endif

#include <txtrich.h>                // CRichText
#include <eikenv.h>                 // CEikonEnv
#include <apgcli.h>                 // RApaLsSession
#include <apgtask.h>                // TApaTaskList, TApaTask
#include <rsendas.h>                // RSendAs
#include <rsendasmessage.h>         // RSendAsMessage

#ifdef USE_DOCUMENTHANDLER
#include <DocumentHandler.h>        // CDocumentHandler
#include <AknServerApp.h>
#endif
#else // USE_SCHEMEHANDLER
#include <schemehandler.h>
#endif

#ifdef Q_WS_S60
#include <CDirectoryLocalizer.h>     // CDirectoryLocalizer
#endif

QT_BEGIN_NAMESPACE

_LIT(KCacheSubDir, "Cache\\");
_LIT(KSysBin, "\\Sys\\Bin\\");
_LIT(KBrowserPrefix, "4 " );
_LIT(KFontsDir, "z:\\resource\\Fonts\\");

#ifndef USE_SCHEMEHANDLER
// copied from miutset.h, so we don't get a dependency into the app layer
const TUid KUidMsgTypeSMTP = {0x10001028}; // 268439592
const TUid KUidBrowser = { 0x10008D39 };

template<class R>
class QAutoClose
{
public:
    QAutoClose(R& aObj) : mPtr(&aObj) {}
    ~QAutoClose()
    {
        if (mPtr)
            mPtr->Close();
    }
    void Forget()
    {
        mPtr = 0;
    }
private:
    QAutoClose(const QAutoClose&);
    QAutoClose& operator=(const QAutoClose&);
private:
    R* mPtr;
};

#ifdef USE_DOCUMENTHANDLER
class QS60DocumentHandler : public MAknServerAppExitObserver
{
public:
    QS60DocumentHandler() :docHandler(0) {}

    ~QS60DocumentHandler() {
        delete docHandler;
    }

    CDocumentHandler& documentHandler() {
        // In case user calls openUrl twice subsequently, before the first embedded app is closed
        // we use the same CDocumentHandler instance. Using same instance makes sure the first
        // launched embedded app is closed and latter one gets embedded to our app.
        // Using different instance would help only theoretically since user cannot interact with
        // several embedded apps at the same time.
        if(!docHandler) {
            QT_TRAP_THROWING(docHandler = CDocumentHandler::NewL());
            docHandler->SetExitObserver(this);
        }
        return *docHandler;
    }

private: // From MAknServerAppExitObserver
    void HandleServerAppExit(TInt /*aReason*/) {
        delete docHandler;
        docHandler = 0;
    }

private:
    CDocumentHandler* docHandler;
};
Q_GLOBAL_STATIC(QS60DocumentHandler, qt_s60_documenthandler);
#endif

static void handleMailtoSchemeLX(const QUrl &url)
{
    // this function has many intermingled leaves and throws. Qt and Symbian objects do not have
    // destructor dependencies, and cleanup object is used to prevent cleanup stack dependency on stack.
    QString recipient = url.path();
    QString subject = url.queryItemValue(QLatin1String("subject"));
    QString body = url.queryItemValue(QLatin1String("body"));
    QString to = url.queryItemValue(QLatin1String("to"));
    QString cc = url.queryItemValue(QLatin1String("cc"));
    QString bcc = url.queryItemValue(QLatin1String("bcc"));

    // these fields might have comma separated addresses
    QStringList recipients = recipient.split(QLatin1String(","), QString::SkipEmptyParts);
    QStringList tos = to.split(QLatin1String(","), QString::SkipEmptyParts);
    QStringList ccs = cc.split(QLatin1String(","), QString::SkipEmptyParts);
    QStringList bccs = bcc.split(QLatin1String(","), QString::SkipEmptyParts);

    RSendAs sendAs;
    User::LeaveIfError(sendAs.Connect());
    QAutoClose<RSendAs> sendAsCleanup(sendAs);

    CSendAsAccounts* accounts = CSendAsAccounts::NewL();
    CleanupStack::PushL(accounts);
    sendAs.AvailableAccountsL(KUidMsgTypeSMTP, *accounts);
    TInt count = accounts->Count();
    CleanupStack::PopAndDestroy(accounts);

    if(!count) {
        // TODO: Task 259192: We should try to create account if count == 0
        // CSendUi would provide account creation service for us, but it requires ridicilous
        // capabilities: LocalServices NetworkServices ReadDeviceData ReadUserData WriteDeviceData WriteUserData
        User::Leave(KErrNotSupported);
    } else {
        RSendAsMessage sendAsMessage;
        sendAsMessage.CreateL(sendAs, KUidMsgTypeSMTP);
        QAutoClose<RSendAsMessage> sendAsMessageCleanup(sendAsMessage);


        // Subject
        sendAsMessage.SetSubjectL(qt_QString2TPtrC(subject));

        // Body
        sendAsMessage.SetBodyTextL(qt_QString2TPtrC(body));

        // To
        foreach(QString item, recipients)
         sendAsMessage.AddRecipientL(qt_QString2TPtrC(item), RSendAsMessage::ESendAsRecipientTo);

        foreach(QString item, tos)
         sendAsMessage.AddRecipientL(qt_QString2TPtrC(item), RSendAsMessage::ESendAsRecipientTo);

        // Cc
        foreach(QString item, ccs)
         sendAsMessage.AddRecipientL(qt_QString2TPtrC(item), RSendAsMessage::ESendAsRecipientCc);

        // Bcc
        foreach(QString item, bccs)
         sendAsMessage.AddRecipientL(qt_QString2TPtrC(item), RSendAsMessage::ESendAsRecipientBcc);

        // send the message
        sendAsMessage.LaunchEditorAndCloseL();
        // sendAsMessage is already closed
        sendAsMessageCleanup.Forget();
    }
}

static bool handleMailtoScheme(const QUrl &url)
{
    TRAPD(err, QT_TRYCATCH_LEAVING(handleMailtoSchemeLX(url)));
    return err ? false : true;
}

static void handleOtherSchemesL(const TDesC& aUrl)
{
    // Other schemes are at the moment passed to WEB browser
    HBufC* buf16 = HBufC::NewLC(aUrl.Length() + KBrowserPrefix.iTypeLength);
    buf16->Des().Copy(KBrowserPrefix); // Prefix used to launch correct browser view
    buf16->Des().Append(aUrl);

    TApaTaskList taskList(CEikonEnv::Static()->WsSession());
    TApaTask task = taskList.FindApp(KUidBrowser);
    if (task.Exists()){
        // Switch to existing browser instance
        task.BringToForeground();
        HBufC8* param8 = HBufC8::NewLC(buf16->Length());
        param8->Des().Append(buf16->Des());
        task.SendMessage(TUid::Uid( 0 ), *param8); // Uid is not used
        CleanupStack::PopAndDestroy(param8);
    } else {
        // Start a new browser instance
        RApaLsSession appArcSession;
        User::LeaveIfError(appArcSession.Connect());
        CleanupClosePushL<RApaLsSession>(appArcSession);
        TThreadId id;
        appArcSession.StartDocument(*buf16, KUidBrowser, id);
        CleanupStack::PopAndDestroy(); // appArcSession
    }

    CleanupStack::PopAndDestroy(buf16);
}

static bool handleOtherSchemes(const QUrl &url)
{
    QString encUrl(QString::fromUtf8(url.toEncoded()));
    TPtrC urlPtr(qt_QString2TPtrC(encUrl));
    TRAPD( err, handleOtherSchemesL(urlPtr));
    return err ? false : true;
}


static void openDocumentL(const TDesC& aUrl)
{
#ifndef USE_DOCUMENTHANDLER
    // Start app associated to file MIME type by using RApaLsSession
    // Apparc base method cannot be used to open app in embedded mode,
    // but seems to be most stable way at the moment
    RApaLsSession appArcSession;
    User::LeaveIfError(appArcSession.Connect());
    CleanupClosePushL<RApaLsSession>(appArcSession);
    TThreadId id;
    // ESwitchFiles means do not start another instance
    // Leaves if file does not exist, leave is trapped in openDocument and false returned to user.
    User::LeaveIfError(appArcSession.StartDocument(aUrl, id,
            RApaLsSession::ESwitchFiles)); // ELaunchNewApp
    CleanupStack::PopAndDestroy(); // appArcSession
#else
    // This is an alternative way to launch app associated to MIME type
    // CDocumentHandler also supports opening apps in embedded mode.
    TDataType temp;
    qt_s60_documenthandler()->documentHandler().OpenFileEmbeddedL(aUrl, temp);
#endif
}

static bool launchWebBrowser(const QUrl &url)
{
    if (!url.isValid())
        return false;

    if (url.scheme() == QLatin1String("mailto")) {
        return handleMailtoScheme(url);
    }
    return handleOtherSchemes( url );
}

static bool openDocument(const QUrl &file)
{
    if (!file.isValid())
        return false;

    QString filePath = file.toLocalFile();
    filePath = QDir::toNativeSeparators(filePath);
    TPtrC filePathPtr(qt_QString2TPtrC(filePath));
    TRAPD(err, openDocumentL(filePathPtr));
    return err ? false : true;
}

#else //USE_SCHEMEHANDLER
// The schemehandler component only exist in private SDK. This implementation
// exist here just for convenience in case that we need to use it later on
// The schemehandle based implementation is not yet tested.

// The biggest advantage of schemehandler is that it can handle
// wide range of schemes and is extensible by plugins
static void handleUrlL(const TDesC& aUrl)
{
    CSchemeHandler* schemeHandler = CSchemeHandler::NewL(aUrl);
    CleanupStack::PushL(schemeHandler);
    schemeHandler->HandleUrlStandaloneL(); // Process the Url in standalone mode
    CleanupStack::PopAndDestroy();
}

static bool handleUrl(const QUrl &url, bool useEncodedUrl)
{
    if (!url.isValid())
        return false;

    QString urlString(useEncodedUrl ? url.toEncoded() : url.toString());
    TPtrC urlPtr(qt_QString2TPtrC(urlString));
    TRAPD( err, handleUrlL(urlPtr));
    return err ? false : true;
}

static bool launchWebBrowser(const QUrl &url)
{
    return handleUrl(url, true);
}

static bool openDocument(const QUrl &file)
{
    return handleUrl(file, false);
}

#endif //USE_SCHEMEHANDLER

// Common functions to all implementations

static TDriveUnit exeDrive()
{
    RProcess me;
    TFileName processFileName = me.FileName();
    TDriveUnit drive(processFileName);
    return drive;
}

static TDriveUnit writableExeDrive()
{
    TDriveUnit drive = exeDrive();
    if (drive.operator TInt() == EDriveZ)
        return TDriveUnit(EDriveC);
    return drive;
}

static TPtrC writableDataRoot()
{
    TDriveUnit drive = exeDrive();
    switch (drive.operator TInt()){
        case EDriveC:
            return PathInfo::PhoneMemoryRootPath();
            break;
        case EDriveE:
            return PathInfo::MemoryCardRootPath();
            break;
        case EDriveZ:
            // It is not possible to write on ROM drive ->
            // return phone mem root path instead
            return PathInfo::PhoneMemoryRootPath();
            break;
        default:
            return PathInfo::PhoneMemoryRootPath();
            break;
    }
}

QString QDesktopServices::storageLocation(StandardLocation type)
{
    TFileName path;

    switch (type) {
    case DesktopLocation:
        qWarning("No desktop concept in Symbian OS");
        // But lets still use some feasible default
        path.Append(writableDataRoot());
        break;
    case DocumentsLocation:
        path.Append(writableDataRoot());
        break;
    case FontsLocation:
        path.Append(KFontsDir);
        break;
    case ApplicationsLocation:
        path.Append(exeDrive().Name());
        path.Append(KSysBin);
        break;
    case MusicLocation:
        path.Append(writableDataRoot());
        path.Append(PathInfo::SoundsPath());
        break;
    case MoviesLocation:
        path.Append(writableDataRoot());
        path.Append(PathInfo::VideosPath());
        break;
    case PicturesLocation:
        path.Append(writableDataRoot());
        path.Append(PathInfo::ImagesPath());
        break;
    case TempLocation:
        return QDir::tempPath();
        break;
    case HomeLocation:
        path.Append(writableDataRoot());
        //return QDir::homePath(); break;
        break;
    case DataLocation:
        qt_s60GetRFs().PrivatePath(path);
        path.Insert(0, writableExeDrive().Name());
        break;
    case CacheLocation:
        qt_s60GetRFs().PrivatePath(path);
        path.Insert(0, writableExeDrive().Name());
        path.Append(KCacheSubDir);
        break;
    default:
        // Lets use feasible default
        path.Append(writableDataRoot());
        break;
    }

    // Convert to cross-platform format and clean the path
    QString nativePath = QString::fromUtf16(path.Ptr(), path.Length());
    QString qtPath = QDir::fromNativeSeparators(nativePath);
    qtPath = QDir::cleanPath(qtPath);

    // Note: The storage location returned can be a directory that does not exist;
    // i.e., it may need to be created by the system or the user.
    return  qtPath;
}

typedef QString (*LocalizerFunc)(QString&);

static QString defaultLocalizedDirectoryName(QString&)
{
    return QString();
}

QString QDesktopServices::displayName(StandardLocation type)
{
    QString ret;

#ifdef Q_WS_S60
    QString rawPath = storageLocation(type);

    TRAPD(err,
        QT_TRYCATCH_LEAVING(
            CDirectoryLocalizer* localizer = CDirectoryLocalizer::NewL();
            CleanupStack::PushL(localizer);
            localizer->SetFullPath(qt_QString2TPtrC(QDir::toNativeSeparators(rawPath)));
            if (localizer->IsLocalized()) {
                TPtrC locName(localizer->LocalizedName());
                ret = qt_TDesC2QString(locName);
            }
            CleanupStack::PopAndDestroy(localizer);
        )
    )

    if (err != KErrNone)
        ret = QString();
#else
    qWarning("QDesktopServices::displayName() not implemented for this platform version");
#endif

    return ret;
}

QT_END_NAMESPACE
