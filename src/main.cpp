/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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
#include "consts.h"
#include "utils.h"
#include "env.h"
#include "phantom.h"
#include "crashdump.h"

#include <QApplication>
#include <QSslSocket>
#include <QIcon>
#include <QWebSettings>

#include <exception>
#include <stdio.h>

static int inner_main(int argc, char** argv)
{
    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/phantomjs-icon.png"));
    app.setApplicationName("PhantomJS");
    app.setOrganizationName("Ofi Labs");
    app.setOrganizationDomain("www.ofilabs.com");
    app.setApplicationVersion(PHANTOMJS_VERSION_STRING);

    // Registering an alternative Message Handler
    qInstallMessageHandler(Utils::messageHandler);

#if defined(Q_OS_LINUX)
    if (QSslSocket::supportsSsl()) {
        // Don't perform on-demand loading of root certificates on Linux
        QSslSocket::addDefaultCaCertificates(QSslSocket::systemCaCertificates());
    }
#endif

    // Get the Phantom singleton
    Phantom* phantom = Phantom::instance();

    // Start script execution
    if (phantom->execute()) {
        app.exec();
    }

    // End script execution: delete the phantom singleton and set
    // execution return value
    int retVal = phantom->returnValue();
    delete phantom;

#ifndef QT_NO_DEBUG
    // Clear all cached data before exiting, so it is not detected as
    // leaked.
    QWebSettings::clearMemoryCaches();
#endif

    return retVal;
}

int main(int argc, char** argv)
{
    try {
        init_crash_handler();
        return inner_main(argc, argv);

        // These last-ditch exception handlers write to the C stderr
        // because who knows what kind of state Qt is in.  And they avoid
        // using fprintf because _that_ might be in bad shape too.
        // (I would drop all the way down to write() but then I'd have to
        // write the code again for Windows.)
        //
        // print_crash_message includes a call to fflush(stderr).
    } catch (std::bad_alloc) {
        fputs("Memory exhausted.\n", stderr);
        fflush(stderr);
        return 1;

    } catch (std::exception& e) {
        fputs("Uncaught C++ exception: ", stderr);
        fputs(e.what(), stderr);
        putc('\n', stderr);
        print_crash_message();
        return 1;

    } catch (...) {
        fputs("Uncaught nonstandard exception.\n", stderr);
        print_crash_message();
        return 1;
    }
}
