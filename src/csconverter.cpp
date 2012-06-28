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

#include "csconverter.h"

#include <QCoreApplication>
#include <QWebFrame>

#include "utils.h"
#include "terminal.h"

static CSConverter *csconverter_instance = 0;

CSConverter *CSConverter::instance()
{
    if (!csconverter_instance)
        csconverter_instance = new CSConverter();

    return csconverter_instance;
}

CSConverter::CSConverter()
    : QObject(QCoreApplication::instance())
{
    m_webPage.mainFrame()->evaluateJavaScript(
        Utils::readResourceFileUtf8(":/coffee-script/extras/coffee-script.js"),
        QString("phantomjs://coffee-script/extras/coffee-script.js")
    );
    m_webPage.mainFrame()->addToJavaScriptWindowObject("converter", this);
}

QVariant CSConverter::convert(const QString &script)
{
    setProperty("source", script);
    QVariant result = m_webPage.mainFrame()->evaluateJavaScript(
        "try {" \
        "    [true, this.CoffeeScript.compile(converter.source)];" \
        "} catch (error) {" \
        "    [false, error.message];" \
        "}",
        QString()
    );
    return result;
}
