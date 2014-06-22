/*
 *  Copyright (C) 2011 Apple Inc. All rights reserved.
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
 *
 */

#ifndef StringRecursionChecker_h
#define StringRecursionChecker_h

#include "Interpreter.h"
#include "VMStackBounds.h"
#include <wtf/StackStats.h>
#include <wtf/WTFThreadData.h>

namespace JSC {

class StringRecursionChecker {
    WTF_MAKE_NONCOPYABLE(StringRecursionChecker);

public:
    StringRecursionChecker(ExecState*, JSObject* thisObject);
    ~StringRecursionChecker();

    JSValue earlyReturnValue() const; // 0 if everything is OK, value to return for failure cases

private:
    JSValue throwStackOverflowError();
    JSValue emptyString();
    JSValue performCheck();

    ExecState* m_exec;
    JSObject* m_thisObject;
    JSValue m_earlyReturnValue;

    StackStats::CheckPoint stackCheckpoint;
};

inline JSValue StringRecursionChecker::performCheck()
{
    VM& vm = m_exec->vm();
    const VMStackBounds nativeStack(vm, wtfThreadData().stack());
    if (!nativeStack.isSafeToRecurse())
        return throwStackOverflowError();
    bool alreadyVisited = !vm.stringRecursionCheckVisitedObjects.add(m_thisObject).isNewEntry;
    if (alreadyVisited)
        return emptyString(); // Return empty string to avoid infinite recursion.
    return JSValue(); // Indicate success.
}

inline StringRecursionChecker::StringRecursionChecker(ExecState* exec, JSObject* thisObject)
    : m_exec(exec)
    , m_thisObject(thisObject)
    , m_earlyReturnValue(performCheck())
{
}

inline JSValue StringRecursionChecker::earlyReturnValue() const
{
    return m_earlyReturnValue;
}

inline StringRecursionChecker::~StringRecursionChecker()
{
    if (m_earlyReturnValue)
        return;
    ASSERT(m_exec->vm().stringRecursionCheckVisitedObjects.contains(m_thisObject));
    m_exec->vm().stringRecursionCheckVisitedObjects.remove(m_thisObject);
}

}

#endif
