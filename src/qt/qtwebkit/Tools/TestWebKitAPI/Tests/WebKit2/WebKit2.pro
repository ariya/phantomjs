TEMPLATE = app
TARGET = tst_webkit2

SOURCES += \
    AboutBlankLoad.cpp \
    DocumentStartUserScriptAlertCrash.cpp \
    DOMWindowExtensionBasic.cpp \
    DOMWindowExtensionNoCache.cpp \
    DocumentStartUserScriptAlertCrash.cpp \
    EvaluateJavaScript.cpp \
    FailedLoad.cpp \
    Find.cpp \
    FrameMIMETypeHTML.cpp \
    FrameMIMETypePNG.cpp \
    GetInjectedBundleInitializationUserDataCallback.cpp \
    HitTestResultNodeHandle.cpp \
    InjectedBundleBasic.cpp \
    InjectedBundleFrameHitTest.cpp \
    InjectedBundleInitializationUserDataCallbackWins.cpp \
    LoadAlternateHTMLStringWithNonDirectoryURL.cpp \
    LoadCanceledNoServerRedirectCallback.cpp \
    LoadPageOnCrash.cpp \
    MouseMoveAfterCrash.cpp \
    PageLoadBasic.cpp \
    PageLoadDidChangeLocationWithinPageForFrame.cpp \
    PageVisibilityState.cpp \
    ParentFrame.cpp \
    PreventEmptyUserAgent.cpp \
    PrivateBrowsingPushStateNoHistoryCallback.cpp \
    ReloadPageAfterCrash.cpp \
    ResizeWindowAfterCrash.cpp \
    ResponsivenessTimerDoesntFireEarly.cpp \
    TerminateTwice.cpp \
    UserMessage.cpp \
    WillSendSubmitEvent.cpp \
    WKConnection.cpp \
    WKString.cpp \
    WKStringJSString.cpp \
    WKURL.cpp

FAILING_SOURCES = \
    CanHandleRequest.cpp \

FAILING_SOURCES += ShouldGoToBackForwardListItem.cpp # Only stalls in flickable mode.
FAILING_SOURCES += SpacebarScrolling.cpp # Only fails in flickable mode.

SOURCES += $$FAILING_SOURCES

# Tests skipped because they crash, stall or do not compile.
SKIPPED_SOURCES = \
    CookieManager.cpp \
    DownloadDecideDestinationCrash.cpp \
    ForceRepaint.cpp \
    NewFirstVisuallyNonEmptyLayout.cpp \
    NewFirstVisuallyNonEmptyLayoutFails.cpp \
    NewFirstVisuallyNonEmptyLayoutForImages.cpp \
    NewFirstVisuallyNonEmptyLayoutFrames.cpp \
    RestoreSessionStateContainingFormData.cpp \
    WebCoreStatisticsWithNoWebProcess.cpp

include(../../TestWebKitAPI.pri)

DEFINES += APITEST_SOURCE_DIR=\\\"$$PWD\\\"
