/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestController.h"

#include <fcntl.h>
#include <io.h>
#include <shlwapi.h>
#include <string>
#include <WebKit2/WKContextPrivateWin.h>
#include <WebKit2/WKStringCF.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

using namespace std;

namespace WTR {

static HANDLE webProcessCrashingEvent;
static const char webProcessCrashingEventName[] = "WebKitTestRunner.WebProcessCrashing";
// This is the longest we'll wait (in seconds) for the web process to finish crashing and a crash
// log to be saved. This interval should be just a tiny bit longer than it will ever reasonably
// take to save a crash log.
static const double maximumWaitForWebProcessToCrash = 60;

#ifdef DEBUG_ALL
const LPWSTR testPluginDirectoryName = L"TestNetscapePlugin_Debug";
const char* injectedBundleDLL = "\\InjectedBundle_debug.dll";
#else
const LPWSTR testPluginDirectoryName = L"TestNetscapePlugin";
const char* injectedBundleDLL = "\\InjectedBundle.dll";
#endif

static void addQTDirToPATH()
{
    static LPCWSTR pathEnvironmentVariable = L"PATH";
    static LPCWSTR quickTimeKeyName = L"Software\\Apple Computer, Inc.\\QuickTime";
    static LPCWSTR quickTimeSysDir = L"QTSysDir";
    static bool initialized;

    if (initialized)
        return;
    initialized = true;

    // Get the QuickTime dll directory from the registry. The key can be in either HKLM or HKCU.
    WCHAR qtPath[MAX_PATH];
    DWORD qtPathBufferLen = sizeof(qtPath);
    DWORD keyType;
    HRESULT result = ::SHGetValueW(HKEY_LOCAL_MACHINE, quickTimeKeyName, quickTimeSysDir, &keyType, (LPVOID)qtPath, &qtPathBufferLen);
    if (result != ERROR_SUCCESS || !qtPathBufferLen || keyType != REG_SZ) {
        qtPathBufferLen = sizeof(qtPath);
        result = ::SHGetValueW(HKEY_CURRENT_USER, quickTimeKeyName, quickTimeSysDir, &keyType, (LPVOID)qtPath, &qtPathBufferLen);
        if (result != ERROR_SUCCESS || !qtPathBufferLen || keyType != REG_SZ)
            return;
    }

    // Read the current PATH.
    DWORD pathSize = ::GetEnvironmentVariableW(pathEnvironmentVariable, 0, 0);
    Vector<WCHAR> oldPath(pathSize);
    if (!::GetEnvironmentVariableW(pathEnvironmentVariable, oldPath.data(), oldPath.size()))
        return;

    // And add the QuickTime dll.
    wstring newPath;
    newPath.append(qtPath);
    newPath.append(L";");
    newPath.append(oldPath.data(), oldPath.size());
    ::SetEnvironmentVariableW(pathEnvironmentVariable, newPath.data());
}

static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS*)
{
    fputs("#CRASHED\n", stderr);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

void TestController::notifyDone()
{
}

void TestController::platformInitialize()
{
    // Cygwin calls ::SetErrorMode(SEM_FAILCRITICALERRORS), which we will inherit. This is bad for
    // testing/debugging, as it causes the post-mortem debugger not to be invoked. We reset the
    // error mode here to work around Cygwin's behavior. See <http://webkit.org/b/55222>.
    ::SetErrorMode(0);

    ::SetUnhandledExceptionFilter(exceptionFilter);

    _setmode(1, _O_BINARY);
    _setmode(2, _O_BINARY);

    // Add the QuickTime dll directory to PATH or QT 7.6 will fail to initialize on systems
    // linked with older versions of qtmlclientlib.dll.
    addQTDirToPATH();

    webProcessCrashingEvent = ::CreateEventA(0, FALSE, FALSE, webProcessCrashingEventName);
}

void TestController::platformDestroy()
{
}

void TestController::initializeInjectedBundlePath()
{
    CFStringRef exeContainerPath = CFURLCopyFileSystemPath(CFURLCreateCopyDeletingLastPathComponent(0, CFBundleCopyExecutableURL(CFBundleGetMainBundle())), kCFURLWindowsPathStyle);
    CFMutableStringRef bundlePath = CFStringCreateMutableCopy(0, 0, exeContainerPath);
    CFStringAppendCString(bundlePath, injectedBundleDLL, kCFStringEncodingWindowsLatin1);
    m_injectedBundlePath.adopt(WKStringCreateWithCFString(bundlePath));
}

void TestController::initializeTestPluginDirectory()
{
    RetainPtr<CFURLRef> bundleURL = adoptCF(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
    RetainPtr<CFURLRef> bundleDirectoryURL = adoptCF(CFURLCreateCopyDeletingLastPathComponent(0, bundleURL.get()));
    RetainPtr<CFStringRef> testPluginDirectoryNameString = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(testPluginDirectoryName), wcslen(testPluginDirectoryName)));
    RetainPtr<CFURLRef> testPluginDirectoryURL = adoptCF(CFURLCreateCopyAppendingPathComponent(0, bundleDirectoryURL.get(), testPluginDirectoryNameString.get(), true));
    RetainPtr<CFStringRef> testPluginDirectoryPath = adoptCF(CFURLCopyFileSystemPath(testPluginDirectoryURL.get(), kCFURLWindowsPathStyle));
    m_testPluginDirectory.adopt(WKStringCreateWithCFString(testPluginDirectoryPath.get()));
}

enum RunLoopResult { TimedOut, ObjectSignaled, ConditionSatisfied };

static RunLoopResult runRunLoopUntil(bool& condition, HANDLE object, double timeout)
{
    DWORD end = ::GetTickCount() + timeout * 1000;
    while (!condition) {
        DWORD now = ::GetTickCount();
        if (now > end)
            return TimedOut;

        DWORD objectCount = object ? 1 : 0;
        const HANDLE* objects = object ? &object : 0;
        DWORD result = ::MsgWaitForMultipleObjectsEx(objectCount, objects, end - now, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (result == WAIT_TIMEOUT)
            return TimedOut;

        if (objectCount && result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + objectCount)
            return ObjectSignaled;

        ASSERT(result == WAIT_OBJECT_0 + objectCount);
        // There are messages in the queue. Process them.
        MSG msg;
        while (::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    return ConditionSatisfied;
}

void TestController::platformRunUntil(bool& done, double timeout)
{
    // FIXME: No timeout should occur if timeout is equal to m_noTimeout (necessary when running performance tests).
    RunLoopResult result = runRunLoopUntil(done, webProcessCrashingEvent, timeout);
    if (result == TimedOut || result == ConditionSatisfied)
        return;
    ASSERT(result == ObjectSignaled);

    // The web process is crashing. A crash log might be being saved, which can take a long
    // time, and we don't want to time out while that happens.

    // First, let the test harness know this happened so it won't think we've hung. But
    // make sure we don't exit just yet!
    m_shouldExitWhenWebProcessCrashes = false;
    processDidCrash();
    m_shouldExitWhenWebProcessCrashes = true;

    // Then spin a run loop until it finishes crashing to give time for a crash log to be saved. If
    // it takes too long for a crash log to be saved, we'll just give up.
    bool neverSetCondition = false;
    result = runRunLoopUntil(neverSetCondition, 0, maximumWaitForWebProcessToCrash);
    ASSERT_UNUSED(result, result == TimedOut);
    exit(1);
}

static WKRetainPtr<WKStringRef> toWK(const char* string)
{
    return WKRetainPtr<WKStringRef>(AdoptWK, WKStringCreateWithUTF8CString(string));
}

void TestController::platformInitializeContext()
{
    // FIXME: Make DRT pass with Windows native controls. <http://webkit.org/b/25592>
    WKContextSetShouldPaintNativeControls(m_context.get(), false);

    WKContextSetInitializationUserDataForInjectedBundle(m_context.get(), toWK(webProcessCrashingEventName).get());
}

void TestController::runModal(PlatformWebView*)
{
    // FIXME: Need to implement this to test showModalDialog.
}

const char* TestController::platformLibraryPathForTesting()
{
    return 0;
}

} // namespace WTR
