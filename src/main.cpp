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

#if QT_VERSION != QT_VERSION_CHECK(4, 8, 2)
#error Something is wrong with the setup. Please report to the mailing list!
#endif

int main(int argc, char** argv, const char** envp)
{
#ifdef Q_OS_LINUX
    google_breakpad::ExceptionHandler eh("/tmp", NULL, Utils::exceptionHandler, NULL, true);
#endif
#ifdef Q_OS_MAC
    google_breakpad::ExceptionHandler eh("/tmp", NULL, Utils::exceptionHandler, NULL, true, NULL);
#endif

    QApplication app(argc, argv);
    Phantom phantom;

    // Registering an alternative Message Handler
    Utils::printDebugMessages = phantom.printDebugMessages();
    qInstallMsgHandler(Utils::messageHandler);

    app.setWindowIcon(QIcon(":/phantomjs-icon.png"));
    app.setApplicationName("PhantomJS");
    app.setOrganizationName("Ofi Labs");
    app.setOrganizationDomain("www.ofilabs.com");
    app.setApplicationVersion(PHANTOMJS_VERSION_STRING);

    Env::instance()->parse(envp);

    phantom.init();
    if (phantom.execute()) {
        app.exec();
    }
    return phantom.returnValue();
}
