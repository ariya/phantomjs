/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebCoreJSClientData_h
#define WebCoreJSClientData_h

#include "DOMWrapperWorld.h"
#include "DOMObjectHashTableMap.h"
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class WebCoreJSClientData : public JSC::JSGlobalData::ClientData {
    WTF_MAKE_NONCOPYABLE(WebCoreJSClientData); WTF_MAKE_FAST_ALLOCATED;
    friend class JSGlobalDataWorldIterator;
    friend void initNormalWorldClientData(JSC::JSGlobalData*);

public:
    WebCoreJSClientData() { }
    virtual ~WebCoreJSClientData()
    {
        ASSERT(m_worldSet.contains(m_normalWorld.get()));
        ASSERT(m_worldSet.size() == 1);
        ASSERT(m_normalWorld->hasOneRef());
        m_normalWorld.clear();
        ASSERT(m_worldSet.isEmpty());
    }

    DOMWrapperWorld* normalWorld() { return m_normalWorld.get(); }

    void getAllWorlds(Vector<DOMWrapperWorld*>& worlds)
    {
        copyToVector(m_worldSet, worlds);
    }

    void rememberWorld(DOMWrapperWorld* world)
    {
        ASSERT(!m_worldSet.contains(world));
        m_worldSet.add(world);
    }
    void forgetWorld(DOMWrapperWorld* world)
    {
        ASSERT(m_worldSet.contains(world));
        m_worldSet.remove(world);
    }

    DOMObjectHashTableMap hashTableMap;

private:
    HashSet<DOMWrapperWorld*> m_worldSet;
    RefPtr<DOMWrapperWorld> m_normalWorld;
};

inline void initNormalWorldClientData(JSC::JSGlobalData* globalData)
{
    WebCoreJSClientData* webCoreJSClientData = new WebCoreJSClientData;
    globalData->clientData = webCoreJSClientData; // ~JSGlobalData deletes this pointer.
    webCoreJSClientData->m_normalWorld = DOMWrapperWorld::create(globalData, true);
}

} // namespace WebCore

#endif // WebCoreJSClientData_h
