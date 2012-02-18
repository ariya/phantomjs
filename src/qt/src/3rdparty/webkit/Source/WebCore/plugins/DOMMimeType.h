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

#ifndef DOMMimeType_h
#define DOMMimeType_h

#include "Frame.h"
#include "PluginData.h"

#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DOMPlugin;

class DOMMimeType : public RefCounted<DOMMimeType>, private FrameDestructionObserver {
public:
    static PassRefPtr<DOMMimeType> create(PassRefPtr<PluginData> pluginData, Frame* frame, unsigned index) { return adoptRef(new DOMMimeType(pluginData, frame, index)); }
    ~DOMMimeType();

    const String &type() const;
    String suffixes() const;
    const String &description() const;
    PassRefPtr<DOMPlugin> enabledPlugin() const;

    // FrameDestructionObserver
    virtual void frameDestroyed() { m_frame = 0; }

private:
    const MimeClassInfo& mimeClassInfo() const { return m_pluginData->mimes()[m_index]; }
    
    DOMMimeType(PassRefPtr<PluginData>, Frame*, unsigned index);
    RefPtr<PluginData> m_pluginData;
    Frame* m_frame;
    unsigned m_index;
};

}

#endif
