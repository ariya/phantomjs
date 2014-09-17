/*
 *  Copyright (C) 2008, 2009 Apple Inc. All rights reseved.
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

#ifndef JSDOMWindowCustom_h
#define JSDOMWindowCustom_h

#include "JSDOMWindow.h"
#include "JSDOMWindowShell.h"
#include "SecurityOrigin.h"
#include <wtf/AlwaysInline.h>

namespace WebCore {

inline JSDOMWindow* asJSDOMWindow(JSC::JSGlobalObject* globalObject)
{
    return static_cast<JSDOMWindow*>(globalObject);
}
 
inline const JSDOMWindow* asJSDOMWindow(const JSC::JSGlobalObject* globalObject)
{
    return static_cast<const JSDOMWindow*>(globalObject);
}

inline bool JSDOMWindowBase::allowsAccessFrom(const JSGlobalObject* other) const
{
    if (allowsAccessFromPrivate(other))
        return true;
    printErrorMessage(crossDomainAccessErrorMessage(other));
    return false;
}

inline bool JSDOMWindowBase::allowsAccessFrom(JSC::ExecState* exec) const
{
    if (allowsAccessFromPrivate(exec->lexicalGlobalObject()))
        return true;
    printErrorMessage(crossDomainAccessErrorMessage(exec->lexicalGlobalObject()));
    return false;
}
    
inline bool JSDOMWindowBase::allowsAccessFromNoErrorMessage(JSC::ExecState* exec) const
{
    return allowsAccessFromPrivate(exec->lexicalGlobalObject());
}
    
inline bool JSDOMWindowBase::allowsAccessFrom(JSC::ExecState* exec, String& message) const
{
    if (allowsAccessFromPrivate(exec->lexicalGlobalObject()))
        return true;
    message = crossDomainAccessErrorMessage(exec->lexicalGlobalObject());
    return false;
}
    
ALWAYS_INLINE bool JSDOMWindowBase::allowsAccessFromPrivate(const JSGlobalObject* other) const
{
    const JSDOMWindow* originWindow = asJSDOMWindow(other);
    const JSDOMWindow* targetWindow = m_shell->window();

    if (originWindow == targetWindow)
        return true;

    const SecurityOrigin* originSecurityOrigin = originWindow->impl()->securityOrigin();
    const SecurityOrigin* targetSecurityOrigin = targetWindow->impl()->securityOrigin();

    return originSecurityOrigin->canAccess(targetSecurityOrigin);
}

}

#endif // JSDOMWindowCustom_h
