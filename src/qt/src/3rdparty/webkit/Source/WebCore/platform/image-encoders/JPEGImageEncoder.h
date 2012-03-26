/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef JPEGImageEncoder_h
#define JPEGImageEncoder_h

#include <wtf/Vector.h>

namespace WebCore {

class IntSize;
bool compressRGBABigEndianToJPEG(unsigned char* rgbaBigEndianData, const IntSize& size, Vector<char>& jpegData);

}

#endif // JPEGImageEncoder_h
