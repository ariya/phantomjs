/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PixelDumpSupportBlackBerry_h
#define PixelDumpSupportBlackBerry_h

#include <stdio.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

class BitmapContext : public RefCounted<BitmapContext> {
public:

    static PassRefPtr<BitmapContext> createByAdoptingData(unsigned char* data, int width, int height)
    {
        return adoptRef(new BitmapContext(data, width, height));
    }

    ~BitmapContext()
    {
        delete m_data;
    }

    unsigned char* m_data;
    int m_width, m_height;

private:

    BitmapContext(unsigned char* data, int width, int height)
        : m_data(data)
        , m_width(width)
        , m_height(height)
    {
    }
};

#endif // PixelDumpSupportBlackBerry_h
