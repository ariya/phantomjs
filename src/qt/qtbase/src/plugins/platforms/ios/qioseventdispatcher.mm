/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qioseventdispatcher.h"
#include "qiosapplicationdelegate.h"
#include "qiosglobal.h"

#include <QtCore/qprocessordetection.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qthread_p.h>

#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <Foundation/NSProcessInfo.h>
#import <Foundation/NSThread.h>
#import <Foundation/NSNotification.h>

#import <UIKit/UIApplication.h>

#include <setjmp.h> // Here be dragons

#include <sys/mman.h>

#define qAlignDown(val, align) val & ~(align - 1)
#define qAlignUp(val, align) qAlignDown(val + (align - 1), align)

static const size_t kBytesPerKiloByte = 1024;
static const long kPageSize = sysconf(_SC_PAGESIZE);

/*
    The following diagram shows the layout of the reserved
    stack in relation to the regular stack, and the basic
    flow of the initial startup sequence. Note how we end
    up back in applicationDidLaunch after the user's main
    recurses into qApp-exec(), which allows us to return
    from applicationDidLaunch and spin the run-loop at the
    same level (UIApplicationMain) as iOS nativly does.

        +-----------------------------+
        |            qtmn()           |
        |     +--------------------+ <-- base
        | +---->      main()       |  |
        | |   +--------------------+  |
        | |   |        ...         |  |
        | |   +--------------------+  |
        | |   |    qApp->exec()    |  |
        | |   +--------------------+  |
        | |   |  processEvents()   |  |
        | |   |                    |  |
        | | +--+   longjmp(a)      |  |
        | | | |                    |  |
        | | | +--------------------+  |
        | | | |                    |  |
        | | | |                    |  |
        | | | |       unused       |  |
        | | | |                    |  |
        | | | |                    |  |
        | | | +--------------------+ <-- limit
        | | | |    memory guard    |  |
        | | | +--------------------+ <-- reservedStack
        +-|-|-------------------------+
        | | |  UIApplicationMain()    |
        +-|-|-------------------------+
        | | | applicationDidLaunch()  |
        | | |                         |
        | | +-->   setjmp(a)          |
        | +----+  trampoline()        |
        |                             |
        +-----------------------------+

    Note: the diagram does not reflect alignment issues.
*/

namespace
{
    struct Stack
    {
        uintptr_t base;
        uintptr_t limit;

        static size_t computeSize(size_t requestedSize)
        {
            if (!requestedSize)
                return 0;

            // The stack size must be a multiple of 4 KB
            size_t stackSize = qAlignUp(requestedSize, 4 * kBytesPerKiloByte);

            // Be at least 16 KB
            stackSize = qMax(16 * kBytesPerKiloByte, stackSize);

            // Have enough extra space for our (aligned) memory guard
            stackSize += (2 * kPageSize);

            // But not exceed the 1MB maximum (adjusted to account for current stack usage)
            stackSize = qMin(stackSize, ((1024 - 64) * kBytesPerKiloByte));

            // Which we verify, just in case
            struct rlimit stackLimit = {0, 0};
            if (getrlimit(RLIMIT_STACK, &stackLimit) == 0 && stackSize > stackLimit.rlim_cur)
                qFatal("Unexpectedly exceeded stack limit");

            return stackSize;
        }

        void adopt(void* memory, size_t size)
        {
            uintptr_t memoryStart = uintptr_t(memory);

            // Add memory guard at the end of the reserved stack, so that any stack
            // overflow during the user's main will trigger an exception at that point,
            // and not when we return and find that the current stack has been smashed.
            // We allow read though, so that garbage-collection can pass through our
            // stack in its mark phase without triggering access violations.
            uintptr_t memoryGuardStart = qAlignUp(memoryStart, kPageSize);
            if (mprotect((void*)memoryGuardStart, kPageSize, PROT_READ))
                qWarning() << "Failed to add memory guard:" << strerror(errno);

            // We don't consider the memory guard part of the usable stack space
            limit = memoryGuardStart + kPageSize;

            // The stack grows downwards in memory, so the stack base is actually
            // at the end of the reserved stack space. And, as the memory guard
            // was page aligned, we need to align down the base as well, to
            // keep the requirement that the stack size is a multiple of 4K.
            base = qAlignDown(memoryStart + size, kPageSize);
        }

        bool isValid()
        {
            return base && limit;
        }

        size_t size()
        {
            return base - limit;
        }

        static const int kScribblePattern;

        void scribble()
        {
            memset_pattern4((void*)limit, &kScribblePattern, size());
        }

        void printUsage()
        {
            uintptr_t highWaterMark = limit;
            for (; highWaterMark < base; highWaterMark += 4) {
                if (memcmp((void*)highWaterMark, &kScribblePattern, 4))
                    break;
            }

            qDebug("main() used roughly %lu bytes of stack space", (base - highWaterMark));
        }
    };

    const int Stack::kScribblePattern = 0xfafafafa;

    Stack userMainStack;

    jmp_buf processEventEnterJumpPoint;
    jmp_buf processEventExitJumpPoint;

    bool applicationAboutToTerminate = false;
    jmp_buf applicationWillTerminateJumpPoint;

    bool debugStackUsage = false;
}

extern "C" int __attribute__((weak)) main(int argc, char *argv[])
{
    @autoreleasepool {
        size_t defaultStackSize = 512 * kBytesPerKiloByte; // Same as secondary threads

        uint requestedStackSize = qMax(0, infoPlistValue(@"QtRunLoopIntegrationStackSize", defaultStackSize));

        if (infoPlistValue(@"QtRunLoopIntegrationDisableSeparateStack", false))
            requestedStackSize = 0;

        char reservedStack[Stack::computeSize(requestedStackSize)];

        if (sizeof(reservedStack) > 0) {
            userMainStack.adopt(reservedStack, sizeof(reservedStack));

            if (infoPlistValue(@"QtRunLoopIntegrationDebugStackUsage", false)) {
                debugStackUsage = true;
                userMainStack.scribble();
                qDebug("Effective stack size is %lu bytes", userMainStack.size());
            }
        }

        qEventDispatcherDebug() << "Running UIApplicationMain"; qIndent();
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([QIOSApplicationDelegate class]));
    }
}

enum SetJumpResult
{
    kJumpPointSetSuccessfully = 0,
    kJumpedFromEventDispatcherProcessEvents,
    kJumpedFromEventLoopExecInterrupt,
    kJumpedFromUserMainTrampoline,
};

// We define qt_main so that user_main_trampoline() will not cause
// missing symbols in the case of hybrid applications that don't
// user our main wrapper. Since the symbol is weak, it will not
// get used or cause a clash in the normal Qt application usecase,
// where we rename main to qt_main.
extern "C" int __attribute__((weak)) qtmn(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    Q_UNREACHABLE();
}

static void __attribute__((noinline, noreturn)) user_main_trampoline()
{
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    int argc = arguments.count;
    char **argv = new char*[argc];
    for (int i = 0; i < argc; ++i) {
        NSString *arg = [arguments objectAtIndex:i];
        argv[i] = reinterpret_cast<char *>(malloc([arg lengthOfBytesUsingEncoding:[NSString defaultCStringEncoding]]));
        strcpy(argv[i], [arg cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    }

    int exitCode = qtmn(argc, argv);
    delete[] argv;

    qEventDispatcherDebug() << "Returned from main with exit code " << exitCode;

    if (Q_UNLIKELY(debugStackUsage))
        userMainStack.printUsage();

    if (applicationAboutToTerminate)
        longjmp(applicationWillTerminateJumpPoint, kJumpedFromUserMainTrampoline);

    // We end up here if the user's main() never calls QApplication::exec(),
    // or if we return from exec() after quitting the application. If so we
    // follow the expected behavior from the point of the user's main(), which
    // is to exit with the given exit code.
    exit(exitCode);
}

// If we don't have a stack set up, we're not running inside
// iOS' native/root level run-loop in UIApplicationMain.
static bool rootLevelRunLoopIntegration()
{
    return userMainStack.isValid();
}

@interface QIOSApplicationStateTracker : NSObject
@end

@implementation QIOSApplicationStateTracker

+ (void) load
{
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(applicationDidFinishLaunching)
        name:UIApplicationDidFinishLaunchingNotification
        object:nil];

    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(applicationWillTerminate)
        name:UIApplicationWillTerminateNotification
        object:nil];
}

#if defined(Q_PROCESSOR_X86)
#  define SET_STACK_POINTER "mov %0, %%esp"
#  define FUNCTION_CALL_ALIGNMENT 16
#elif defined(Q_PROCESSOR_ARM)
#  define SET_STACK_POINTER "mov sp, %0"
#  define FUNCTION_CALL_ALIGNMENT 4
#else
#  error "Unknown processor family"
#endif

+ (void) applicationDidFinishLaunching
{
    if (!isQtApplication())
        return;

    if (!rootLevelRunLoopIntegration()) {
        // We schedule the main-redirection for the next run-loop pass, so that we
        // can return from this function and let UIApplicationMain finish its job.
        // This results in running Qt's application eventloop as a nested runloop.
        qEventDispatcherDebug() << "Scheduling main() on next run-loop pass";
        CFRunLoopTimerRef userMainTimer = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault,
             CFAbsoluteTimeGetCurrent(), 0, 0, 0, ^(CFRunLoopTimerRef) { user_main_trampoline(); });
        CFRunLoopAddTimer(CFRunLoopGetMain(), userMainTimer, kCFRunLoopCommonModes);
        CFRelease(userMainTimer);
        return;
    }

    switch (setjmp(processEventEnterJumpPoint)) {
    case kJumpPointSetSuccessfully:
        qEventDispatcherDebug() << "Running main() on separate stack"; qIndent();

        // Redirect the stack pointer to the start of the reserved stack. This ensures
        // that when we longjmp out of the event dispatcher and continue execution, the
        // 'Qt main' call-stack will not be smashed, as it lives in a part of the stack
        // that was allocated back in main().
        __asm__ __volatile__(
            SET_STACK_POINTER
            : /* no outputs */
            : "r" (qAlignDown(userMainStack.base, FUNCTION_CALL_ALIGNMENT))
        );

        user_main_trampoline();

        Q_UNREACHABLE();
        break;
    case kJumpedFromEventDispatcherProcessEvents:
        // We've returned from the longjmp in the event dispatcher,
        // and the stack has been restored to its old self.
        qUnIndent(); qEventDispatcherDebug() << "Returned from processEvents";

        if (Q_UNLIKELY(debugStackUsage))
            userMainStack.printUsage();

        break;
    default:
        qFatal("Unexpected jump result in event loop integration");
    }
}

// We treat applicationWillTerminate as SIGTERM, even if it can't be ignored,
// and follow the bash convention of encoding the signal number in the upper
// four bits of the exit code (exit(3) will only pass on the lower 8 bits).
static const char kApplicationWillTerminateExitCode = SIGTERM | 0x80;

+ (void) applicationWillTerminate
{
    if (!isQtApplication())
        return;

    if (!rootLevelRunLoopIntegration())
        return;

    // Normally iOS just sends SIGKILL to quit applications, in which case there's
    // no chance for us to clean up anything, but in some rare cases iOS will tell
    // us that the application is about to be terminated.

    // We try to play nice with Qt by ending the main event loop, which will result
    // in QCoreApplication::aboutToQuit() being emitted, and main() returning to the
    // trampoline. The trampoline then redirects us back here, so that we can return
    // to UIApplicationMain instead of calling exit().

    applicationAboutToTerminate = true;
    switch (setjmp(applicationWillTerminateJumpPoint)) {
    case kJumpPointSetSuccessfully:
        qEventDispatcherDebug() << "Exiting qApp with SIGTERM exit code"; qIndent();
        qApp->exit(kApplicationWillTerminateExitCode);

        // The runloop will not exit when the application is about to terminate,
        // so we'll never see the exit activity and have a chance to return from
        // QEventLoop::exec(). We initiate the return manually as a workaround.
        qEventDispatcherDebug() << "Manually triggering return from event loop exec";
        static_cast<QIOSEventDispatcher *>(qApp->eventDispatcher())->interruptEventLoopExec();
        break;
    case kJumpedFromUserMainTrampoline:
        // The user's main has returned, so we're ready to let iOS terminate the application
        qUnIndent(); qEventDispatcherDebug() << "kJumpedFromUserMainTrampoline, allowing iOS to terminate";
        break;
    default:
        qFatal("Unexpected jump result in event loop integration");
    }
}

@end

QT_BEGIN_NAMESPACE
QT_USE_NAMESPACE

QIOSEventDispatcher::QIOSEventDispatcher(QObject *parent)
    : QEventDispatcherCoreFoundation(parent)
    , m_processEventCallsAfterExec(0)
    , m_runLoopExitObserver(this, &QIOSEventDispatcher::handleRunLoopExit, kCFRunLoopExit)
{
}

bool __attribute__((returns_twice)) QIOSEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (!rootLevelRunLoopIntegration())
        return QEventDispatcherCoreFoundation::processEvents(flags);

    if (applicationAboutToTerminate) {
        qEventDispatcherDebug() << "Detected QEventLoop exec after application termination";
        // Re-issue exit, and return immediately
        qApp->exit(kApplicationWillTerminateExitCode);
        return false;
    }

    if (!m_processEventCallsAfterExec && (flags & QEventLoop::EventLoopExec)) {
        ++m_processEventCallsAfterExec;

        m_runLoopExitObserver.addToMode(kCFRunLoopCommonModes);

        // We set a new jump point here that we can return to when the event loop
        // is asked to exit, so that we can return from QEventLoop::exec().
        switch (setjmp(processEventExitJumpPoint)) {
        case kJumpPointSetSuccessfully:
            qEventDispatcherDebug() << "QEventLoop exec detected, jumping back to native runloop";
            longjmp(processEventEnterJumpPoint, kJumpedFromEventDispatcherProcessEvents);
            break;
        case kJumpedFromEventLoopExecInterrupt:
            // The event loop has exited (either by the hand of the user, or the iOS termination
            // signal), and we jumped back though processEventExitJumpPoint. We return from processEvents,
            // which will emit aboutToQuit if it's QApplication's event loop, and then return to the user's
            // main, which can do whatever it wants, including calling exec() on the application again.
            qEventDispatcherDebug() << "kJumpedFromEventLoopExecInterrupt, returning with eventsProcessed = true";
            return true;
        default:
            qFatal("Unexpected jump result in event loop integration");
        }

        Q_UNREACHABLE();
    }

    if (m_processEventCallsAfterExec)
        ++m_processEventCallsAfterExec;

    bool processedEvents = QEventDispatcherCoreFoundation::processEvents(flags);

    if (m_processEventCallsAfterExec)
        --m_processEventCallsAfterExec;

    return processedEvents;
}

void QIOSEventDispatcher::handleRunLoopExit(CFRunLoopActivity activity)
{
    Q_UNUSED(activity);
    Q_ASSERT(activity == kCFRunLoopExit);

    if (m_processEventCallsAfterExec == 1 && !QThreadData::current()->eventLoops.top()->isRunning()) {
        qEventDispatcherDebug() << "Root runloop level exited";
        interruptEventLoopExec();
    }
}

void QIOSEventDispatcher::interruptEventLoopExec()
{
    Q_ASSERT(m_processEventCallsAfterExec == 1);

    --m_processEventCallsAfterExec;

    m_runLoopExitObserver.removeFromMode(kCFRunLoopCommonModes);

    // We re-set applicationProcessEventsReturnPoint here so that future
    // calls to QEventLoop::exec() will end up back here after entering
    // processEvents, instead of back in didFinishLaunchingWithOptions.
    switch (setjmp(processEventEnterJumpPoint)) {
    case kJumpPointSetSuccessfully:
        qEventDispatcherDebug() << "Jumping back to processEvents";
        longjmp(processEventExitJumpPoint, kJumpedFromEventLoopExecInterrupt);
        break;
    case kJumpedFromEventDispatcherProcessEvents:
        // QEventLoop was re-executed
        qEventDispatcherDebug() << "kJumpedFromEventDispatcherProcessEvents";
        break;
    default:
        qFatal("Unexpected jump result in event loop integration");
    }
}

QT_END_NAMESPACE
