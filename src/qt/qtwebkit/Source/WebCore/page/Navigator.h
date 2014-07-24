/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef Navigator_h
#define Navigator_h

#include "DOMWindowProperty.h"
#include "NavigatorBase.h"
#include "ScriptWrappable.h"
#include "Supplementable.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DOMMimeTypeArray;
class DOMPluginArray;
class Frame;
class PluginData;

typedef int ExceptionCode;

class Navigator : public NavigatorBase, public ScriptWrappable, public RefCounted<Navigator>, public DOMWindowProperty, public Supplementable<Navigator> {
public:
    static PassRefPtr<Navigator> create(Frame* frame) { return adoptRef(new Navigator(frame)); }
    virtual ~Navigator();

    String appVersion() const;
    String language() const;
    DOMPluginArray* plugins() const;
    DOMMimeTypeArray* mimeTypes() const;
    bool cookieEnabled() const;
    bool javaEnabled() const;

    virtual String userAgent() const;

    // Relinquishes the storage lock, if one exists.
    void getStorageUpdates();

private:
    explicit Navigator(Frame*);

    mutable RefPtr<DOMPluginArray> m_plugins;
    mutable RefPtr<DOMMimeTypeArray> m_mimeTypes;
};

}

#endif
