/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef MediaQueryListListener_h
#define MediaQueryListListener_h

#include "PlatformString.h"
#include "ScriptState.h"
#include "ScriptValue.h"

#include <wtf/RefCounted.h>

namespace WebCore {

class MediaQueryList;

// See http://dev.w3.org/csswg/cssom-view/#the-mediaquerylist-interface

class MediaQueryListListener : public RefCounted<MediaQueryListListener> {
public:
    static PassRefPtr<MediaQueryListListener> create(ScriptValue value)
    {
        if (!value.isFunction())
            return 0;
        return adoptRef(new MediaQueryListListener(value));
    }
    void queryChanged(ScriptState*, MediaQueryList*);

    bool operator==(const MediaQueryListListener& other) const { return m_value == other.m_value; }

private:
    MediaQueryListListener(ScriptValue value) : m_value(value) { }

    ScriptValue m_value;
};

}

#endif // MediaQueryListListener_h
