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

#ifdef Q_OS_LINUX
#include "client/linux/handler/exception_handler.h"
#endif
#ifdef Q_OS_MAC
#include "client/mac/handler/exception_handler.h"
#endif

#include <QApplication>

#if !defined(QT_SHARED) && !defined(QT_DLL)
#include <QtPlugin>

// HACK: When linking a static PhantomJS + MSVC agains the
// static Qt included in PhantomJS, we get linker errors.
// This noop function can cure them.
#include <QUuid>
#include <QPainter>
#include <QXmlStreamAttributes>
void makeLinkerHappy()
{
    QWidget().colorCount();
    QUuid().createUuid();
    QPainter().drawImage(QPointF(), QImage(), QRect());
    const QXmlStreamAttributes foo;
    foo[0];
}
// End of linker hack.

Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
#endif

#if QT_VERSION != QT_VERSION_CHECK(4, 8, 2)
#error Something is wrong with the setup. Please report to the mailing list!
#endif

int main(int argc, char** argv, const char** envp)
{
    // Setup Google Breakpad exception handler
#ifdef Q_OS_LINUX
    google_breakpad::ExceptionHandler eh("/tmp", NULL, Utils::exceptionHandler, NULL, true);
#endif
#ifdef Q_OS_MAC
    google_breakpad::ExceptionHandler eh("/tmp", NULL, Utils::exceptionHandler, NULL, true, NULL);
#endif

    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/phantomjs-icon.png"));
    app.setApplicationName("PhantomJS");
    app.setOrganizationName("Ofi Labs");
    app.setOrganizationDomain("www.ofilabs.com");
    app.setApplicationVersion(PHANTOMJS_VERSION_STRING);

    // Prepare the "env" singleton using the environment variables
    Env::instance()->parse(envp);

    // Get the Phantom singleton
    Phantom *phantom = Phantom::instance();

    // Registering an alternative Message Handler
    Utils::printDebugMessages = phantom->printDebugMessages();
    qInstallMsgHandler(Utils::messageHandler);

    // Start script execution
    if (phantom->execute()) {
        app.exec();
    }

    // End script execution: delete the phantom singleton and set execution return value
    int retVal = phantom->returnValue();
    delete phantom;
    return retVal;
}
