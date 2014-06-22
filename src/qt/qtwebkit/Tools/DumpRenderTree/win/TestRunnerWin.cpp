/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestRunner.h"

#include "DumpRenderTree.h"
#include "EditingDelegate.h"
#include "PolicyDelegate.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <CoreFoundation/CoreFoundation.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRefBSTR.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include <WebCore/COMPtr.h>
#include <WebKit/WebKit.h>
#include <WebKit/WebKitCOMAPI.h>
#include <comutil.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <string>
#include <wtf/Assertions.h>
#include <wtf/Platform.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

using std::string;
using std::wstring;

static bool resolveCygwinPath(const wstring& cygwinPath, wstring& windowsPath);

TestRunner::~TestRunner()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    // reset webview-related states back to default values in preparation for next test

    COMPtr<IWebViewPrivate> viewPrivate;
    if (SUCCEEDED(webView->QueryInterface(&viewPrivate)))
        viewPrivate->setTabKeyCyclesThroughElements(TRUE);

    COMPtr<IWebViewEditing> viewEditing;
    if (FAILED(webView->QueryInterface(&viewEditing)))
        return;
    COMPtr<IWebEditingDelegate> delegate;
    if (FAILED(viewEditing->editingDelegate(&delegate)))
        return;
    COMPtr<EditingDelegate> editingDelegate(Query, viewEditing.get());
    if (editingDelegate)
        editingDelegate->setAcceptsEditing(TRUE);
}

void TestRunner::addDisallowedURL(JSStringRef url)
{
    // FIXME: Implement!
}

void TestRunner::clearBackForwardList()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebBackForwardList> backForwardList;
    if (FAILED(webView->backForwardList(&backForwardList)))
        return;

    COMPtr<IWebHistoryItem> item;
    if (FAILED(backForwardList->currentItem(&item)))
        return;

    // We clear the history by setting the back/forward list's capacity to 0
    // then restoring it back and adding back the current item.
    int capacity;
    if (FAILED(backForwardList->capacity(&capacity)))
        return;

    backForwardList->setCapacity(0);
    backForwardList->setCapacity(capacity);
    backForwardList->addItem(item.get());
    backForwardList->goToItem(item.get());
}

bool TestRunner::callShouldCloseOnWebView()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return false;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return false;

    BOOL result;
    viewPrivate->shouldClose(&result);
    return result;
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef name)
{
    // FIXME: Implement!
    return 0;
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef name)
{
    // FIXME: Implement!
    return 0;
}

void TestRunner::dispatchPendingLoadRequests()
{
    // FIXME: Implement for testing fix for 6727495
}

void TestRunner::display()
{
    displayWebView();
}

void TestRunner::keepWebHistory()
{
    COMPtr<IWebHistory> history;
    if (FAILED(WebKitCreateInstance(CLSID_WebHistory, 0, __uuidof(history), reinterpret_cast<void**>(&history))))
        return;

    COMPtr<IWebHistory> sharedHistory;
    if (FAILED(WebKitCreateInstance(CLSID_WebHistory, 0, __uuidof(sharedHistory), reinterpret_cast<void**>(&sharedHistory))))
        return;

    history->setOptionalSharedHistory(sharedHistory.get());
}

void TestRunner::waitForPolicyDelegate()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    setWaitToDump(true);
    policyDelegate->setControllerToNotifyDone(this);
    webView->setPolicyDelegate(policyDelegate);
}

size_t TestRunner::webHistoryItemCount()
{
    COMPtr<IWebHistory> history;
    if (FAILED(WebKitCreateInstance(CLSID_WebHistory, 0, __uuidof(history), reinterpret_cast<void**>(&history))))
        return 0;

    COMPtr<IWebHistory> sharedHistory;
    if (FAILED(history->optionalSharedHistory(&sharedHistory)) || !sharedHistory)
        return 0;

    COMPtr<IWebHistoryPrivate> sharedHistoryPrivate;
    if (FAILED(sharedHistory->QueryInterface(&sharedHistoryPrivate)))
        return 0;

    int count;
    if (FAILED(sharedHistoryPrivate->allItems(&count, 0)))
        return 0;

    return count;
}

JSRetainPtr<JSStringRef> TestRunner::platformName() const
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("win"));
    return platformName;
}

void TestRunner::notifyDone()
{
    // Same as on mac.  This can be shared.
    if (m_waitToDump && !topLoadingFrame && !WorkQueue::shared()->count())
        dump();
    m_waitToDump = false;
}

JSStringRef TestRunner::pathToLocalResource(JSContextRef context, JSStringRef url)
{
    wstring input(JSStringGetCharactersPtr(url), JSStringGetLength(url));

    wstring localPath;
    if (!resolveCygwinPath(input, localPath)) {
        printf("ERROR: Failed to resolve Cygwin path %S\n", input.c_str());
        return 0;
    }

    return JSStringCreateWithCharacters(localPath.c_str(), localPath.length());
}

static wstring jsStringRefToWString(JSStringRef jsStr)
{
    size_t length = JSStringGetLength(jsStr);
    Vector<WCHAR> buffer(length + 1);
    memcpy(buffer.data(), JSStringGetCharactersPtr(jsStr), length * sizeof(WCHAR));
    buffer[length] = '\0';

    return buffer.data();
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    COMPtr<IWebDataSource> dataSource;
    if (FAILED(frame->dataSource(&dataSource)))
        return;

    COMPtr<IWebURLResponse> response;
    if (FAILED(dataSource->response(&response)) || !response)
        return;

    BSTR responseURLBSTR;
    if (FAILED(response->URL(&responseURLBSTR)))
        return;
    wstring responseURL(responseURLBSTR, SysStringLen(responseURLBSTR));
    SysFreeString(responseURLBSTR);

    // FIXME: We should do real relative URL resolution here.
    int lastSlash = responseURL.rfind('/');
    if (lastSlash != -1)
        responseURL = responseURL.substr(0, lastSlash);

    wstring wURL = jsStringRefToWString(url);
    wstring wAbsoluteURL = responseURL + TEXT("/") + wURL;
    JSRetainPtr<JSStringRef> jsAbsoluteURL(Adopt, JSStringCreateWithCharacters(wAbsoluteURL.data(), wAbsoluteURL.length()));

    WorkQueue::shared()->queue(new LoadItem(jsAbsoluteURL.get(), target));
}

void TestRunner::setAcceptsEditing(bool acceptsEditing)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewEditing> viewEditing;
    if (FAILED(webView->QueryInterface(&viewEditing)))
        return;

    COMPtr<IWebEditingDelegate> delegate;
    if (FAILED(viewEditing->editingDelegate(&delegate)))
        return;

    EditingDelegate* editingDelegate = (EditingDelegate*)(IWebEditingDelegate*)delegate.get();
    editingDelegate->setAcceptsEditing(acceptsEditing);
}

void TestRunner::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    if (alwaysAcceptCookies == m_alwaysAcceptCookies)
        return;

    if (!::setAlwaysAcceptCookies(alwaysAcceptCookies))
        return;
    m_alwaysAcceptCookies = alwaysAcceptCookies;
}

void TestRunner::setAuthorAndUserStylesEnabled(bool flag)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setAuthorAndUserStylesEnabled(flag);
}

void TestRunner::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    if (setDelegate) {
        policyDelegate->setPermissive(permissive);
        webView->setPolicyDelegate(policyDelegate);
    } else
        webView->setPolicyDelegate(0);
}

void TestRunner::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    // FIXME: Implement for DeviceOrientation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=30335.
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
}

void TestRunner::setMockGeolocationPositionUnavailableError(JSStringRef message)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
}

void TestRunner::setGeolocationPermission(bool allow)
{
    // FIXME: Implement for Geolocation layout tests.
    setGeolocationPermissionCommon(allow);
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    // FIXME: Implement for Geolocation layout tests.
    return -1;
}

void TestRunner::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setMockSpeechInputDumpRect(bool flag)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::startSpeechInput(JSContextRef inputElement)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setIconDatabaseEnabled(bool iconDatabaseEnabled)
{
    // See also <rdar://problem/6480108>
    COMPtr<IWebIconDatabase> iconDatabase;
    COMPtr<IWebIconDatabase> tmpIconDatabase;
    if (FAILED(WebKitCreateInstance(CLSID_WebIconDatabase, 0, IID_IWebIconDatabase, (void**)&tmpIconDatabase)))
        return;
    if (FAILED(tmpIconDatabase->sharedIconDatabase(&iconDatabase)))
        return;

    iconDatabase->setEnabled(iconDatabaseEnabled);
}

void TestRunner::setMainFrameIsFirstResponder(bool flag)
{
    // FIXME: Implement!
}

void TestRunner::setPrivateBrowsingEnabled(bool privateBrowsingEnabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    preferences->setPrivateBrowsingEnabled(privateBrowsingEnabled);
}

void TestRunner::setXSSAuditorEnabled(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setXSSAuditorEnabled(enabled);
}

void TestRunner::setSpatialNavigationEnabled(bool enabled)
{
    // FIXME: Implement for SpatialNavigation layout tests.
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setAllowUniversalAccessFromFileURLs(enabled);
}

void TestRunner::setAllowFileAccessFromFileURLs(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setAllowFileAccessFromFileURLs(enabled);
}

void TestRunner::setPopupBlockingEnabled(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    preferences->setJavaScriptCanOpenWindowsAutomatically(!enabled);
}

void TestRunner::setPluginsEnabled(bool flag)
{
    // FIXME: Implement
}

void TestRunner::setJavaScriptCanAccessClipboard(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setJavaScriptCanAccessClipboard(enabled);
}

void TestRunner::setTabKeyCyclesThroughElements(bool shouldCycle)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;

    viewPrivate->setTabKeyCyclesThroughElements(shouldCycle ? TRUE : FALSE);
}

void TestRunner::setUseDashboardCompatibilityMode(bool flag)
{
    // FIXME: Implement!
}

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    preferences->setUserStyleSheetEnabled(flag);
}

bool appendComponentToPath(wstring& path, const wstring& component)
{
    WCHAR buffer[MAX_PATH];

    if (path.size() + 1 > MAX_PATH)
        return false;

    memcpy(buffer, path.data(), path.size() * sizeof(WCHAR));
    buffer[path.size()] = '\0';

    if (!PathAppendW(buffer, component.c_str()))
        return false;

    path = wstring(buffer);
    return true;
}

static bool followShortcuts(wstring& path)
{
    if (PathFileExists(path.c_str()))
        return true;

    // Do we have a shortcut?
    wstring linkPath = path;
    linkPath.append(TEXT(".lnk"));
    if (!PathFileExists(linkPath.c_str()))
       return true;

    // We have a shortcut, find its target.
    COMPtr<IShellLink> shortcut(Create, CLSID_ShellLink);
    if (!shortcut)
       return false;
    COMPtr<IPersistFile> persistFile(Query, shortcut);
    if (!shortcut)
        return false;
    if (FAILED(persistFile->Load(linkPath.c_str(), STGM_READ)))
        return false;
    if (FAILED(shortcut->Resolve(0, 0)))
        return false;
    WCHAR targetPath[MAX_PATH];
    DWORD targetPathLen = _countof(targetPath);
    if (FAILED(shortcut->GetPath(targetPath, targetPathLen, 0, 0)))
        return false;
    if (!PathFileExists(targetPath))
        return false;
    // Use the target path as the result path instead.
    path = wstring(targetPath);

    return true;
}

static bool resolveCygwinPath(const wstring& cygwinPath, wstring& windowsPath)
{
    wstring fileProtocol = L"file://";
    bool isFileProtocol = cygwinPath.find(fileProtocol) != string::npos;
    if (cygwinPath[isFileProtocol ? 7 : 0] != '/') // ensure path is absolute
        return false;

    // Get the Root path.
    WCHAR rootPath[MAX_PATH];
    DWORD rootPathSize = _countof(rootPath);
    DWORD keyType;
    DWORD result = ::SHGetValueW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2\\/"), TEXT("native"), &keyType, &rootPath, &rootPathSize);

    if (result != ERROR_SUCCESS || keyType != REG_SZ) {
        // Cygwin 1.7 doesn't store Cygwin's root as a mount point anymore, because mount points are now stored in /etc/fstab.
        // However, /etc/fstab doesn't contain any information about where / is located as a Windows path, so we need to use Cygwin's
        // new registry key that has the root.
        result = ::SHGetValueW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Cygwin\\setup"), TEXT("rootdir"), &keyType, &rootPath, &rootPathSize);
        if (result != ERROR_SUCCESS || keyType != REG_SZ)
            return false;
    }

    windowsPath = wstring(rootPath, rootPathSize);

    int oldPos = isFileProtocol ? 8 : 1;
    while (1) {
        int newPos = cygwinPath.find('/', oldPos);

        if (newPos == -1) {
            wstring pathComponent = cygwinPath.substr(oldPos);

            if (!appendComponentToPath(windowsPath, pathComponent))
               return false;

            if (!followShortcuts(windowsPath))
                return false;

            break;
        }

        wstring pathComponent = cygwinPath.substr(oldPos, newPos - oldPos);
        if (!appendComponentToPath(windowsPath, pathComponent))
            return false;

        if (!followShortcuts(windowsPath))
            return false;

        oldPos = newPos + 1;
    }

    if (isFileProtocol)
        windowsPath = fileProtocol + windowsPath;

    return true;
}

void TestRunner::setUserStyleSheetLocation(JSStringRef jsURL)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    RetainPtr<CFStringRef> urlString = adoptCF(JSStringCopyCFString(0, jsURL));
    RetainPtr<CFURLRef> url = adoptCF(CFURLCreateWithString(0, urlString.get(), 0));
    if (!url)
        return;

    // Now copy the file system path, POSIX style.
    RetainPtr<CFStringRef> pathCF = adoptCF(CFURLCopyFileSystemPath(url.get(), kCFURLPOSIXPathStyle));
    if (!pathCF)
        return;

    wstring path = cfStringRefToWString(pathCF.get());

    wstring resultPath;
    if (!resolveCygwinPath(path, resultPath))
        return;

    // The path has been resolved, now convert it back to a CFURL.
    int result = WideCharToMultiByte(CP_UTF8, 0, resultPath.c_str(), resultPath.size() + 1, 0, 0, 0, 0);
    Vector<char> utf8Vector(result);
    result = WideCharToMultiByte(CP_UTF8, 0, resultPath.c_str(), resultPath.size() + 1, utf8Vector.data(), result, 0, 0);
    if (!result)
        return;

    url = CFURLCreateFromFileSystemRepresentation(0, (const UInt8*)utf8Vector.data(), utf8Vector.size() - 1, false);
    if (!url)
        return;

    resultPath = cfStringRefToWString(CFURLGetString(url.get()));

    BSTR resultPathBSTR = SysAllocStringLen(resultPath.data(), resultPath.size());
    preferences->setUserStyleSheetLocation(resultPathBSTR);
    SysFreeString(resultPathBSTR);
}

void TestRunner::setValueForUser(JSContextRef context, JSValueRef element, JSStringRef value)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> webViewPrivate(Query, webView);
    if (!webViewPrivate)
        return;

    COMPtr<IDOMElement> domElement;
    if (FAILED(webViewPrivate->elementFromJS(context, element, &domElement)))
        return;

    COMPtr<IDOMHTMLInputElement> domInputElement;
    if (FAILED(domElement->QueryInterface(IID_IDOMHTMLInputElement, reinterpret_cast<void**>(&domInputElement))))
        return;

    _bstr_t valueBSTR(JSStringCopyBSTR(value), false);

    domInputElement->setValueForUser(valueBSTR);
}

void TestRunner::setViewModeMediaFeature(JSStringRef mode)
{
    // FIXME: implement
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef jsURL)
{
    RetainPtr<CFStringRef> urlString = adoptCF(JSStringCopyCFString(0, jsURL));
    ::setPersistentUserStyleSheetLocation(urlString.get());
}

void TestRunner::clearPersistentUserStyleSheet()
{
    ::setPersistentUserStyleSheetLocation(0);
}

void TestRunner::setWindowIsKey(bool flag)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;

    HWND webViewWindow;
    if (FAILED(viewPrivate->viewWindow((OLE_HANDLE*)&webViewWindow)))
        return;

    ::SendMessage(webViewWindow, flag ? WM_SETFOCUS : WM_KILLFOCUS, (WPARAM)::GetDesktopWindow(), 0);
}

static const CFTimeInterval waitToDumpWatchdogInterval = 30.0;

static void CALLBACK waitUntilDoneWatchdogFired(HWND, UINT, UINT_PTR, DWORD)
{
    gTestRunner->waitToDumpWatchdogTimerFired();
}

void TestRunner::setWaitToDump(bool waitUntilDone)
{
    m_waitToDump = waitUntilDone;
    if (m_waitToDump && !waitToDumpWatchdog)
        waitToDumpWatchdog = SetTimer(0, 0, waitToDumpWatchdogInterval * 1000, waitUntilDoneWatchdogFired);
}

int TestRunner::windowCount()
{
    return openWindows().size();
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
    wstring wName = jsStringRefToWString(name);
    wstring wValue = jsStringRefToWString(value);

    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;

    BSTR nameBSTR = SysAllocStringLen((OLECHAR*)wName.c_str(), wName.length());
    BSTR valueBSTR = SysAllocStringLen((OLECHAR*)wValue.c_str(), wValue.length());
    viewPrivate->executeCoreCommandByName(nameBSTR, valueBSTR);

    SysFreeString(nameBSTR);
    SysFreeString(valueBSTR);
}

bool TestRunner::findString(JSContextRef /* context */, JSStringRef /* target */, JSObjectRef /* optionsArray */)
{
    // FIXME: Implement
    return false;
}

void TestRunner::setCacheModel(int)
{
    // FIXME: Implement
}

bool TestRunner::isCommandEnabled(JSStringRef /*name*/)
{
    printf("ERROR: TestRunner::isCommandEnabled() not implemented\n");
    return false;
}

void TestRunner::clearAllApplicationCaches()
{
    // FIXME: Implement to support application cache quotas.
}

void TestRunner::clearApplicationCacheForOrigin(JSStringRef origin)
{
    // FIXME: Implement to support deleting all application cache for an origin.
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long quota)
{
    // FIXME: Implement to support application cache quotas.
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    // FIXME: Implement to get origins that have application caches.
    return JSValueMakeUndefined(context);
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef name)
{
    // FIXME: Implement to get disk usage by all application caches for an origin.
    return 0;
}

void TestRunner::clearAllDatabases()
{
    COMPtr<IWebDatabaseManager> databaseManager;
    COMPtr<IWebDatabaseManager> tmpDatabaseManager;
    if (FAILED(WebKitCreateInstance(CLSID_WebDatabaseManager, 0, IID_IWebDatabaseManager, (void**)&tmpDatabaseManager)))
        return;
    if (FAILED(tmpDatabaseManager->sharedWebDatabaseManager(&databaseManager)))
        return;

    databaseManager->deleteAllDatabases();
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    BSTR keyBSTR = JSStringCopyBSTR(key);
    BSTR valueBSTR = JSStringCopyBSTR(value);
    prefsPrivate->setPreferenceForTest(keyBSTR, valueBSTR);
    SysFreeString(keyBSTR);
    SysFreeString(valueBSTR);
}

void TestRunner::setDatabaseQuota(unsigned long long quota)
{
    COMPtr<IWebDatabaseManager> databaseManager;
    COMPtr<IWebDatabaseManager> tmpDatabaseManager;

    if (FAILED(WebKitCreateInstance(CLSID_WebDatabaseManager, 0, IID_IWebDatabaseManager, (void**)&tmpDatabaseManager)))
        return;
    if (FAILED(tmpDatabaseManager->sharedWebDatabaseManager(&databaseManager)))
        return;

    databaseManager->setQuota(TEXT("file:///"), quota);
}

void TestRunner::goBack()
{
    // FIXME: implement to enable loader/navigation-while-deferring-loads.html
}

void TestRunner::setDefersLoading(bool)
{
    // FIXME: implement to enable loader/navigation-while-deferring-loads.html
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    COMPtr<IWebViewPrivate> webView;
    if (FAILED(WebKitCreateInstance(__uuidof(WebView), 0, __uuidof(webView), reinterpret_cast<void**>(&webView))))
        return;

    BSTR schemeBSTR = JSStringCopyBSTR(scheme);
    webView->setDomainRelaxationForbiddenForURLScheme(forbidden, schemeBSTR);
    SysFreeString(schemeBSTR);
}

void TestRunner::setAppCacheMaximumSize(unsigned long long size)
{
    printf("ERROR: TestRunner::setAppCacheMaximumSize() not implemented\n");
}

static _bstr_t bstrT(JSStringRef jsString)
{
    // The false parameter tells the _bstr_t constructor to adopt the BSTR we pass it.
    return _bstr_t(JSStringCopyBSTR(jsString), false);
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    COMPtr<IWebViewPrivate> webView;
    if (FAILED(WebKitCreateInstance(__uuidof(WebView), 0, __uuidof(webView), reinterpret_cast<void**>(&webView))))
        return;

    webView->addOriginAccessWhitelistEntry(bstrT(sourceOrigin).GetBSTR(), bstrT(destinationProtocol).GetBSTR(), bstrT(destinationHost).GetBSTR(), allowDestinationSubdomains);
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    COMPtr<IWebViewPrivate> webView;
    if (FAILED(WebKitCreateInstance(__uuidof(WebView), 0, __uuidof(webView), reinterpret_cast<void**>(&webView))))
        return;

    webView->removeOriginAccessWhitelistEntry(bstrT(sourceOrigin).GetBSTR(), bstrT(destinationProtocol).GetBSTR(), bstrT(destinationHost).GetBSTR(), allowDestinationSubdomains);
}

void TestRunner::setScrollbarPolicy(JSStringRef orientation, JSStringRef policy)
{
    // FIXME: implement
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
    COMPtr<IWebViewPrivate> webView;
    if (FAILED(WebKitCreateInstance(__uuidof(WebView), 0, __uuidof(webView), reinterpret_cast<void**>(&webView))))
        return;

    COMPtr<IWebScriptWorld> world;
    if (FAILED(WebKitCreateInstance(__uuidof(WebScriptWorld), 0, __uuidof(world), reinterpret_cast<void**>(&world))))
        return;

    webView->addUserScriptToGroup(_bstr_t(L"org.webkit.DumpRenderTree").GetBSTR(), world.get(), bstrT(source).GetBSTR(), 0, 0, 0, 0, 0, runAtStart ? WebInjectAtDocumentStart : WebInjectAtDocumentEnd);
}


void TestRunner::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    COMPtr<IWebViewPrivate> webView;
    if (FAILED(WebKitCreateInstance(__uuidof(WebView), 0, __uuidof(webView), reinterpret_cast<void**>(&webView))))
        return;

    COMPtr<IWebScriptWorld> world;
    if (FAILED(WebKitCreateInstance(__uuidof(WebScriptWorld), 0, __uuidof(world), reinterpret_cast<void**>(&world))))
        return;

    webView->addUserStyleSheetToGroup(_bstr_t(L"org.webkit.DumpRenderTree").GetBSTR(), world.get(), bstrT(source).GetBSTR(), 0, 0, 0, 0, 0);
}

void TestRunner::setDeveloperExtrasEnabled(bool enabled)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebPreferences> preferences;
    if (FAILED(webView->preferences(&preferences)))
        return;

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (!prefsPrivate)
        return;

    prefsPrivate->setDeveloperExtrasEnabled(enabled);
}

void TestRunner::showWebInspector()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate(Query, webView);
    if (!viewPrivate)
        return;

    COMPtr<IWebInspector> inspector;
    if (SUCCEEDED(viewPrivate->inspector(&inspector)))
        inspector->show();
}

void TestRunner::closeWebInspector()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate(Query, webView);
    if (!viewPrivate)
        return;

    COMPtr<IWebInspector> inspector;
    if (FAILED(viewPrivate->inspector(&inspector)))
        return;

    inspector->close();
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate(Query, webView);
    if (!viewPrivate)
        return;

    COMPtr<IWebInspector> inspector;
    if (FAILED(viewPrivate->inspector(&inspector)))
        return;

    COMPtr<IWebInspectorPrivate> inspectorPrivate(Query, inspector);
    if (!inspectorPrivate)
        return;

    inspectorPrivate->evaluateInFrontend(callId, bstrT(script).GetBSTR());
}

typedef HashMap<unsigned, COMPtr<IWebScriptWorld> > WorldMap;
static WorldMap& worldMap()
{
    static WorldMap& map = *new WorldMap;
    return map;
}

unsigned worldIDForWorld(IWebScriptWorld* world)
{
    WorldMap::const_iterator end = worldMap().end();
    for (WorldMap::const_iterator it = worldMap().begin(); it != end; ++it) {
        if (it->value == world)
            return it->key;
    }

    return 0;
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    COMPtr<IWebFramePrivate> framePrivate(Query, frame);
    if (!framePrivate)
        return;

    // A worldID of 0 always corresponds to a new world. Any other worldID corresponds to a world
    // that is created once and cached forever.
    COMPtr<IWebScriptWorld> world;
    if (!worldID) {
        if (FAILED(WebKitCreateInstance(__uuidof(WebScriptWorld), 0, __uuidof(world), reinterpret_cast<void**>(&world))))
            return;
    } else {
        COMPtr<IWebScriptWorld>& worldSlot = worldMap().add(worldID, 0).iterator->value;
        if (!worldSlot && FAILED(WebKitCreateInstance(__uuidof(WebScriptWorld), 0, __uuidof(worldSlot), reinterpret_cast<void**>(&worldSlot))))
            return;
        world = worldSlot;
    }

    BSTR result;
    if (FAILED(framePrivate->stringByEvaluatingJavaScriptInScriptWorld(world.get(), globalObject, bstrT(script).GetBSTR(), &result)))
        return;
    SysFreeString(result);
}

void TestRunner::removeAllVisitedLinks()
{
    COMPtr<IWebHistory> history;
    if (FAILED(WebKitCreateInstance(CLSID_WebHistory, 0, __uuidof(history), reinterpret_cast<void**>(&history))))
        return;

    COMPtr<IWebHistory> sharedHistory;
    if (FAILED(history->optionalSharedHistory(&sharedHistory)) || !sharedHistory)
        return;

    COMPtr<IWebHistoryPrivate> sharedHistoryPrivate;
    if (FAILED(sharedHistory->QueryInterface(&sharedHistoryPrivate)))
        return;

    sharedHistoryPrivate->removeAllVisitedLinks();
}

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{

}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebBackForwardList> backForwardList;
    if (FAILED(webView->backForwardList(&backForwardList)))
        return;

    COMPtr<IWebHistoryItem> item;
    if (FAILED(backForwardList->currentItem(&item)))
        return;

    BOOL success;
    webView->goToBackForwardItem(item.get(), &success);
}

void TestRunner::setWebViewEditable(bool)
{
}

void TestRunner::authenticateSession(JSStringRef, JSStringRef, JSStringRef)
{
}

void TestRunner::abortModal()
{
}

void TestRunner::setSerializeHTTPLoads(bool)
{
    // FIXME: Implement.
}

void TestRunner::syncLocalStorage()
{
    // FIXME: Implement.
}

void TestRunner::observeStorageTrackerNotifications(unsigned number)
{
    // FIXME: Implement.
}

void TestRunner::deleteAllLocalStorage()
{
    // FIXME: Implement.
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    // FIXME: Implement.
    return JSValueMakeUndefined(context);
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef originIdentifier)
{
    // FIXME: Implement to support getting local storage disk usage for an origin.
    return 0;
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef URL)
{
    // FIXME: Implement.
}

void TestRunner::setTextDirection(JSStringRef direction)
{
    COMPtr<IWebFramePrivate> framePrivate(Query, frame);
    if (!framePrivate)
        return;

    framePrivate->setTextDirection(bstrT(direction).GetBSTR());
}

void TestRunner::addChromeInputField()
{
}

void TestRunner::removeChromeInputField()
{
}

void TestRunner::focusWebView()
{
}

void TestRunner::setBackingScaleFactor(double)
{
}

void TestRunner::grantWebNotificationPermission(JSStringRef origin)
{
}

void TestRunner::denyWebNotificationPermission(JSStringRef jsOrigin)
{
}

void TestRunner::removeAllWebNotificationPermissions()
{
}

void TestRunner::simulateWebNotificationClick(JSValueRef jsNotification)
{
}

void TestRunner::simulateLegacyWebNotificationClick(JSStringRef title)
{
    // FIXME: Implement.
}

void TestRunner::resetPageVisibility()
{
    // FIXME: Implement this.
}

void TestRunner::setPageVisibility(const char*)
{
    // FIXME: Implement this.
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool)
{
    // FIXME: Implement this.
}

void TestRunner::setStorageDatabaseIdleInterval(double)
{
    // FIXME: Implement this.
}

void TestRunner::closeIdleLocalStorageDatabases()
{
    // FIXME: Implement this.
}
