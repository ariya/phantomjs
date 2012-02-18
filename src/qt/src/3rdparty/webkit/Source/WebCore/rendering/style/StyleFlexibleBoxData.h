/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef StyleFlexibleBoxData_h
#define StyleFlexibleBoxData_h

#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class StyleFlexibleBoxData : public RefCounted<StyleFlexibleBoxData> {
public:
    static PassRefPtr<StyleFlexibleBoxData> create() { return adoptRef(new StyleFlexibleBoxData); }
    PassRefPtr<StyleFlexibleBoxData> copy() const { return adoptRef(new StyleFlexibleBoxData(*this)); }
    
    bool operator==(const StyleFlexibleBoxData& o) const;
    bool operator!=(const StyleFlexibleBoxData& o) const
    {
        return !(*this == o);
    }

    float flex;
    unsigned int flex_group;
    unsigned int ordinal_group;

    unsigned align : 3; // EBoxAlignment
    unsigned pack: 3; // EBoxAlignment
    unsigned orient: 1; // EBoxOrient
    unsigned lines : 1; // EBoxLines
    
private:
    StyleFlexibleBoxData();
    StyleFlexibleBoxData(const StyleFlexibleBoxData&);
};

} // namespace WebCore

#endif // StyleFlexibleBoxData_h
