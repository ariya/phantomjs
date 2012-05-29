/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
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

#include "config.h"
#include "DOMWrapperWorld.h"

#include "JSDOMWindow.h"
#include "ScriptController.h"
#include "WebCoreJSClientData.h"

using namespace JSC;

namespace WebCore {

void JSDOMWrapperOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSDOMWrapper* wrapper = static_cast<JSDOMWrapper*>(handle.get().asCell());
    void* domObject = context;
    uncacheWrapper(m_world, domObject, wrapper);
}

void JSStringOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSString* jsString = static_cast<JSString*>(handle.get().asCell());
    StringImpl* stringImpl = static_cast<StringImpl*>(context);
    ASSERT_UNUSED(jsString, m_world->m_stringCache.find(stringImpl)->second.get() == jsString);
    m_world->m_stringCache.remove(stringImpl);
}

DOMWrapperWorld::DOMWrapperWorld(JSC::JSGlobalData* globalData, bool isNormal)
    : m_globalData(globalData)
    , m_isNormal(isNormal)
    , m_defaultWrapperOwner(this)
    , m_stringWrapperOwner(this)
{
    JSGlobalData::ClientData* clientData = m_globalData->clientData;
    ASSERT(clientData);
    static_cast<WebCoreJSClientData*>(clientData)->rememberWorld(this);
}

DOMWrapperWorld::~DOMWrapperWorld()
{
    JSGlobalData::ClientData* clientData = m_globalData->clientData;
    ASSERT(clientData);
    static_cast<WebCoreJSClientData*>(clientData)->forgetWorld(this);

    // These items are created lazily.
    while (!m_scriptControllersWithWindowShells.isEmpty())
        (*m_scriptControllersWithWindowShells.begin())->destroyWindowShell(this);
}

void DOMWrapperWorld::clearWrappers()
{
    m_wrappers.clear();
    m_stringCache.clear();

    // These items are created lazily.
    while (!m_scriptControllersWithWindowShells.isEmpty())
        (*m_scriptControllersWithWindowShells.begin())->destroyWindowShell(this);
}

DOMWrapperWorld* normalWorld(JSC::JSGlobalData& globalData)
{
    JSGlobalData::ClientData* clientData = globalData.clientData;
    ASSERT(clientData);
    return static_cast<WebCoreJSClientData*>(clientData)->normalWorld();
}

DOMWrapperWorld* mainThreadNormalWorld()
{
    ASSERT(isMainThread());
    static DOMWrapperWorld* cachedNormalWorld = normalWorld(*JSDOMWindow::commonJSGlobalData());
    return cachedNormalWorld;
}

} // namespace WebCore
