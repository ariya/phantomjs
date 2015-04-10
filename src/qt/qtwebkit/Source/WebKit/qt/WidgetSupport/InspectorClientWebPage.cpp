/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
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
#include "InspectorClientWebPage.h"

#include <qwebframe.h>

using namespace WebKit;

InspectorClientWebPage::InspectorClientWebPage()
{
    QWebView* view = new QWebView;
    view->setPage(this);
    setParent(view);
    connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(javaScriptWindowObjectCleared()));
}

QWebPage* InspectorClientWebPage::createWindow(QWebPage::WebWindowType)
{
    QWebView* view = new QWebView;
    QWebPage* page = new QWebPage;
    view->setPage(page);
    view->setAttribute(Qt::WA_DeleteOnClose);
    return page;
}

void InspectorClientWebPage::javaScriptWindowObjectCleared()
{
    QVariant inspectorJavaScriptWindowObjects = property("_q_inspectorJavaScriptWindowObjects");
    if (!inspectorJavaScriptWindowObjects.isValid())
        return;
    QMap<QString, QVariant> javaScriptNameObjectMap = inspectorJavaScriptWindowObjects.toMap();
    QWebFrame* frame = mainFrame();
    QMap<QString, QVariant>::const_iterator it = javaScriptNameObjectMap.constBegin();
    for ( ; it != javaScriptNameObjectMap.constEnd(); ++it) {
        QString name = it.key();
        QVariant value = it.value();
        QObject* obj = value.value<QObject*>();
        frame->addToJavaScriptWindowObject(name, obj);
    }
}

