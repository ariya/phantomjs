/*
  This file is part of the PhantomJS project from Ofi Labs.

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

#ifndef REPL_H
#define REPL_H

#include <QtWebKitWidgets/QWebFrame>

#include "phantom.h"

// Linenoise is a C Library: we need to externalise it's symbols for linkage
extern "C" {
#include "linenoise.h"
}

/**
 * REPL. Read–Eval–Print Loop.
 *
 * This class realises the REPL functionality within PhantomJS.
 * It's a Singleton: invoke "REPL::getInstance(QWebFrame *, Phantom *) to
 * create the first-and-only instance, or no parameter to get the singleton
 * if previously created.
 *
 * It's based the Linenoise library (https://github.com/tadmarshall/linenoise).
 * More info about REPL: http://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop
 */
class REPL: public QObject
{
    Q_OBJECT

public:
    static bool instanceExists();
    static REPL* getInstance(QWebFrame* webframe = NULL, Phantom* parent = NULL);

    Q_INVOKABLE QString _getClassName(QObject* obj) const;
    Q_INVOKABLE QStringList _enumerateCompletions(QObject* obj) const;

private:
    REPL(QWebFrame* webframe, Phantom* parent);
    static void offerCompletion(const char* buf, linenoiseCompletions* lc);

private slots:
    void startLoop();
    void stopLoop(const int code);

private:
    QWebFrame* m_webframe;
    Phantom* m_parentPhantom;
    bool m_looping;
    QByteArray m_historyFilepath;
};

#endif // REPL_H
