/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef PDFDocumentImage_h
#define PDFDocumentImage_h

#include "FloatRect.h"
#include "GraphicsTypes.h"
#include "Image.h"

#if USE(CG)

typedef struct CGPDFDocument *CGPDFDocumentRef;

namespace WebCore {

    class GraphicsContext;

    class PDFDocumentImage : public Image {
    public:
        static PassRefPtr<PDFDocumentImage> create()
        {
            return adoptRef(new PDFDocumentImage);
        }

    private:
        virtual ~PDFDocumentImage();

        virtual String filenameExtension() const;

        virtual bool hasSingleSecurityOrigin() const { return true; }

        virtual bool dataChanged(bool allDataReceived);

        // FIXME: PDF Images are underreporting decoded sizes and will be unable
        // to prune because these functions are not implemented yet.
        virtual void destroyDecodedData(bool /*destroyAll*/ = true) { }
        virtual unsigned decodedSize() const { return 0; }

        virtual IntSize size() const;

        PDFDocumentImage();
        virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator);
        
        void setCurrentPage(int);
        int pageCount() const;
        void adjustCTM(GraphicsContext*) const;

        CGPDFDocumentRef m_document;
        FloatRect m_mediaBox;
        FloatRect m_cropBox;
        float m_rotation;
        int m_currentPage;
    };

}

#endif // USE(CG)

#endif // PDFDocumentImage_h
