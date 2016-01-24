/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "crashdump.h"

#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <sys/resource.h>
#include <string.h>
#endif

void
print_crash_message()
{
    fputs("PhantomJS has crashed. Please read the bug reporting guide at\n"
          "<http://phantomjs.org/bug-reporting.html> and file a bug report.\n",
          stderr);
    fflush(stderr);
}

#if defined(Q_OS_WIN)

static LONG WINAPI unhandled_exception_filter(LPEXCEPTION_POINTERS ptrs)
{
    fprintf(stderr, "Fatal Windows exception, code 0x%08x.\n",
            ptrs->ExceptionRecord->ExceptionCode);
    print_crash_message();
    return EXCEPTION_EXECUTE_HANDLER;
}

#if _MSC_VER >= 1400
static void
invalid_parameter_handler(const wchar_t* expression,
                          const wchar_t* function,
                          const wchar_t* file,
                          unsigned int line,
                          uintptr_t /*reserved*/)
{
    // The parameters all have the value NULL unless a debug version of the CRT library is used
    // https://msdn.microsoft.com/en-us/library/a9yf33zb(v=VS.80).aspx
#ifndef _DEBUG
    Q_UNUSED(expression);
    Q_UNUSED(function);
    Q_UNUSED(file);
    Q_UNUSED(line);
    fprintf(stderr, "Invalid parameter detected.\n");
#else
    fprintf(stderr, "Invalid parameter detected at %ls:%u: %ls: %ls\n",
            file, line, function, expression);
#endif // _DEBUG
    print_crash_message();
    ExitProcess(STATUS_FATAL_APP_EXIT);
}
#endif

static void
pure_virtual_call_handler()
{
    fputs("Pure virtual method called.\n", stderr);
    print_crash_message();
    ExitProcess(STATUS_FATAL_APP_EXIT);
}

static void
handle_fatal_signal(int signo)
{
    Q_UNUSED(signo);

    print_crash_message();
    // Because signals on Windows are fake, and because it doesn't provide
    // sigaction(), we cannot rely on reraising the exception.
    ExitProcess(STATUS_FATAL_APP_EXIT);
}

static void
init_crash_handler_os()
{
    SetErrorMode(SEM_FAILCRITICALERRORS |
                 SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX |
                 SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(unhandled_exception_filter);

    // When the app crashes, don't print the abort message
    // and don't call Dr. Watson to make a crash dump.
    // http://msdn.microsoft.com/en-us/library/e631wekh(v=VS.100).aspx
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);


#if _MSC_VER >= 1400
    _set_invalid_parameter_handler(invalid_parameter_handler);
#endif
    _set_purecall_handler(pure_virtual_call_handler);

    // Signals on Windows are not operating system primitives and mostly
    // shouldn't be used, but installing a handler for SIGABRT is the only
    // way to intercept calls to abort().
    signal(SIGABRT, handle_fatal_signal);
}

#else // not Windows; Unix assumed

static void
handle_fatal_signal(int signo)
{
    // It would be nice to print the offending signal name here, but
    // strsignal() isn't reliably available.  Instead we let the shell do it.
    print_crash_message();
    raise(signo);
}

static void
init_crash_handler_os()
{
    const char* offender;

    // Disable core dumps; they are gigantic and useless.
    offender = "setrlimit";
    struct rlimit rl;
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rl)) { goto fail; }

    // Ensure that none of the signals that indicate a fatal CPU exception
    // are blocked.  (If they are delivered while blocked, the behavior is
    // undefined per POSIX -- usually the kernel just zaps the process
    // without giving it a chance to print a helpful message or anything.)
    offender = "sigprocmask";
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGABRT);
    sigaddset(&mask, SIGBUS);
    sigaddset(&mask, SIGFPE);
    sigaddset(&mask, SIGILL);
    sigaddset(&mask, SIGSEGV);
    sigaddset(&mask, SIGQUIT);
    if (sigprocmask(SIG_UNBLOCK, &mask, 0)) { goto fail; }

    // Install a signal handler for all the above signals.  This will call
    // print_crash_message and then reraise the signal (so the exit code will
    // be accurate).
    offender = "sigaction";
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_fatal_signal;
    sa.sa_flags   = SA_NODEFER | SA_RESETHAND;

    if (sigaction(SIGABRT, &sa, 0)) { goto fail; }
    if (sigaction(SIGBUS,  &sa, 0)) { goto fail; }
    if (sigaction(SIGFPE,  &sa, 0)) { goto fail; }
    if (sigaction(SIGILL,  &sa, 0)) { goto fail; }
    if (sigaction(SIGSEGV, &sa, 0)) { goto fail; }
    if (sigaction(SIGQUIT, &sa, 0)) { goto fail; }

    return;

fail:
    perror(offender);
    exit(1);
}

#endif // not Windows

void
init_crash_handler()
{
    // Qt, QtWebkit, and PhantomJS mostly don't make use of C++ exceptions,
    // so in the rare cases where an exception does get thrown, it will
    // pass all the way up the stack and cause the C++ runtime to call
    // std::terminate().  The default std::terminate() handler in some C++
    // runtimes tries to print details of the exception or maybe even a stack
    // trace.  That's great, but... the most frequent case is bad_alloc,
    // thrown because we've run into a system-imposed hard upper limit on
    // memory allocation.  A clever terminate handler may itself perform more
    // memory allocation, which will throw another bad_alloc, and cause a
    // recursive call to terminate.  In some cases this may happen several
    // times before the process finally dies.
    //
    // So we have last-ditch exception handlers in main.cpp that should catch
    // everything, and in case _that_ fails, we replace the terminate handler
    // with something that is guaranteed not to allocate memory.
    std::set_terminate(abort);

    // Initialize system-specific crash detection.
    init_crash_handler_os();
}
