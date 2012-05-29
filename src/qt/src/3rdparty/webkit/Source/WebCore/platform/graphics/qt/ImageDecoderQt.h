/*
 * Copyright (C) 2006 Friedemann Kleint <fkleint@trolltech.com>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ImageDecoderQt_h
#define ImageDecoderQt_h

#include "ImageDecoder.h"
#include <QtGui/QImageReader>
#include <QtGui/QPixmap>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QBuffer>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {


class ImageDecoderQt : public ImageDecoder
{
public:
    ImageDecoderQt(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
    ~ImageDecoderQt();

    virtual void setData(SharedBuffer* data, bool allDataReceived);
    virtual bool isSizeAvailable();
    virtual size_t frameCount();
    virtual int repetitionCount() const;
    virtual ImageFrame* frameBufferAtIndex(size_t index);

    virtual String filenameExtension() const;

    virtual void clearFrameBufferCache(size_t clearBeforeFrame);

private:
    ImageDecoderQt(const ImageDecoderQt&);
    ImageDecoderQt &operator=(const ImageDecoderQt&);

private:
    void internalDecodeSize();
    void internalReadImage(size_t);
    bool internalHandleCurrentImage(size_t);
    void forceLoadEverything();
    void clearPointers();

private:
    QByteArray m_format;
    OwnPtr<QBuffer> m_buffer;
    OwnPtr<QImageReader> m_reader;
    mutable int m_repetitionCount;
};



}

#endif

