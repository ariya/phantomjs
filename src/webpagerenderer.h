/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2017 Vitaly Slobodin <vitaliy.slobodin@gmail.com>

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

#pragma once

#include <QBuffer>

#include "custompage.h"

class WebPageRenderer : public QObject
{
    Q_OBJECT
public:

    /*
      reply - reply to be proxied(nested reply)
      shouldCaptureResponseBody - if true, response body will be collected and
                                  available through body() method
     */
    NetworkReplyProxy(QObject* parent, QNetworkReply* reply,
                      bool shouldCaptureResponseBody);


    /*
      Returns collected body
     */
    QString body();

    /*
      Returns the body size. Even if body isn't being captured.
     */
    int bodySize();

    /*
      Returns nested reply
     */
    QNetworkReply* nestedReply() const
    {
        //TODO: in cpp
        return m_reply;
    };


public:
    enum RenderMode { Content, Viewport };

    explicit WebPageRenderer(CustomPage* customPage, qreal dpi, QObject* parent = Q_NULLPTR);
    virtual ~WebPageRenderer() = default;

    bool renderToFile(
        const QString& filename,
        const QString& format,
        const QRect clipRect,
        QVariantMap paperSize,
        RenderMode mode = Content,
        int quality = -1);
    
    QString renderToBase64(
        const QString& format,
        const QRect clipRect,
        QVariantMap paperSize,
        RenderMode mode = Content,
        int quality = -1) const;

    void setDpi(qreal dpi);

    // TODO: make private
    qreal stringToPointSize(const QString& string) const;

private:
    qreal printMargin(const QVariantMap& map, const QString& key) const;
    QBuffer* renderPdf(QVariantMap paperSize, QRect clipRect) const;

    QImage* renderImage(const RenderMode mode, QRect clipRect) const;

#ifdef Q_OS_WIN
    // TODO: Change output of stdout or stderr to binary
    void setStdOutputMode(const QString& filename, int mode);
#endif
    CustomPage* m_customPage;
    qreal m_dpi;
};
