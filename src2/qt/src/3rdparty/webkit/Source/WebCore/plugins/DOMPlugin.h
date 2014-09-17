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

#ifndef DOMPlugin_h
#define DOMPlugin_h

#include "DOMMimeType.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class Plugin;
class PluginData;

class DOMPlugin : public RefCounted<DOMPlugin>, private FrameDestructionObserver {
public:
    static PassRefPtr<DOMPlugin> create(PluginData* pluginData, Frame* frame, unsigned index) { return adoptRef(new DOMPlugin(pluginData, frame, index)); }
    ~DOMPlugin();

    String name() const;
    String filename() const;
    String description() const;

    unsigned length() const;

    PassRefPtr<DOMMimeType> item(unsigned index);
    bool canGetItemsForName(const AtomicString& propertyName);
    PassRefPtr<DOMMimeType> namedItem(const AtomicString& propertyName);

    // FrameDestructionObserver
    virtual void frameDestroyed() { m_frame = 0; }

private:
    const PluginInfo& pluginInfo() const { return m_pluginData->plugins()[m_index]; }

    DOMPlugin(PluginData*, Frame*, unsigned index);
    RefPtr<PluginData> m_pluginData;
    Frame* m_frame;
    unsigned m_index;
};

} // namespace WebCore

#endif // Plugin_h
