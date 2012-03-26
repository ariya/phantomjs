/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All Rights Reserved.
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "JSNavigator.h"

#include "CallbackFunction.h"
#include "JSNavigatorUserMediaErrorCallback.h"
#include "JSNavigatorUserMediaSuccessCallback.h"
#include "Navigator.h"

namespace WebCore {

using namespace JSC;

#if ENABLE(MEDIA_STREAM)
JSValue JSNavigator::webkitGetUserMedia(ExecState* exec)
{
    // Arguments: Options, successCallback, (optional)errorCallback

    String options = ustringToString(exec->argument(0).toString(exec));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<NavigatorUserMediaSuccessCallback> successCallback = createFunctionOnlyCallback<JSNavigatorUserMediaSuccessCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(1));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<NavigatorUserMediaErrorCallback> errorCallback = createFunctionOnlyCallback<JSNavigatorUserMediaErrorCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(2), CallbackAllowUndefined);
    if (exec->hadException())
        return jsUndefined();

    m_impl->webkitGetUserMedia(options, successCallback.release(), errorCallback.release());
    return jsUndefined();
}
#endif // ENABLE(MEDIA_STREAM)

}
