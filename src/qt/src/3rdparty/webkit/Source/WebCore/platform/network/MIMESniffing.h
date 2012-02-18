/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MIMESniffing_h
#define MIMESniffing_h

#include <stddef.h>

// MIME type sniffing implementation based on http://tools.ietf.org/html/draft-abarth-mime-sniff-06

class MIMESniffer {
public:
    MIMESniffer(const char* advertisedMIMEType, bool isSupportedImageType);

    size_t dataSize() const { return m_dataSize; }
    const char* sniff(const char* data, size_t size) const { return m_function ?  m_function(data, size) : 0; }
    bool isValid() const { return m_dataSize > 0; }

private:
    typedef const char* (*SniffFunction)(const char*, size_t);
    size_t m_dataSize;
    SniffFunction m_function;
};

#endif // MIMESniffing_h
