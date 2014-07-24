/*
 * Copyright (C) 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
 * All rights reserved.
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

#include "config.h"
#include "FormDataStreamCurl.h"

#include "FormData.h"
#include "ResourceRequest.h"
#include <wtf/text/CString.h>

namespace WebCore {

FormDataStream::~FormDataStream()
{
    if (m_file)
        fclose(m_file);
}

size_t FormDataStream::read(void* ptr, size_t blockSize, size_t numberOfBlocks)
{
    // Check for overflow.
    if (!numberOfBlocks || blockSize > std::numeric_limits<size_t>::max() / numberOfBlocks)
        return 0;

    Vector<FormDataElement> elements;
    if (m_resourceHandle->firstRequest().httpBody())
        elements = m_resourceHandle->firstRequest().httpBody()->elements();

    if (m_formDataElementIndex >= elements.size())
        return 0;

    FormDataElement element = elements[m_formDataElementIndex];

    size_t toSend = blockSize * numberOfBlocks;
    size_t sent;

    if (element.m_type == FormDataElement::encodedFile) {
        if (!m_file)
            m_file = fopen(element.m_filename.utf8().data(), "rb");

        if (!m_file) {
            // FIXME: show a user error?
#ifndef NDEBUG
            printf("Failed while trying to open %s for upload\n", element.m_filename.utf8().data());
#endif
            return 0;
        }

        sent = fread(ptr, blockSize, numberOfBlocks, m_file);
        if (!blockSize && ferror(m_file)) {
            // FIXME: show a user error?
#ifndef NDEBUG
            printf("Failed while trying to read %s for upload\n", element.m_filename.utf8().data());
#endif
            return 0;
        }
        if (feof(m_file)) {
            fclose(m_file);
            m_file = 0;
            m_formDataElementIndex++;
        }
    } else {
        size_t elementSize = element.m_data.size() - m_formDataElementDataOffset;
        sent = elementSize > toSend ? toSend : elementSize;
        memcpy(ptr, element.m_data.data() + m_formDataElementDataOffset, sent);
        if (elementSize > sent)
            m_formDataElementDataOffset += sent;
        else {
            m_formDataElementDataOffset = 0;
            m_formDataElementIndex++;
        }
    }

    return sent;
}

bool FormDataStream::hasMoreElements() const
{
    Vector<FormDataElement> elements;
    if (m_resourceHandle->firstRequest().httpBody())
        elements = m_resourceHandle->firstRequest().httpBody()->elements();

    return m_formDataElementIndex < elements.size();
}

} // namespace WebCore
