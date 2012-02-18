/*
    Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>

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

#include "config.h"

#if ENABLE(SVG)
#include "JSSVGLength.h"

#include <runtime/Error.h>
#include "SVGAnimatedProperty.h"
#include "SVGException.h"

using namespace JSC;

namespace WebCore {

JSValue JSSVGLength::value(ExecState* exec) const
{
    SVGLength& podImp = impl()->propertyReference();
    ExceptionCode ec = 0;
    float value = podImp.value(impl()->contextElement(), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return jsNumber(value);
}

void JSSVGLength::setValue(ExecState* exec, JSValue value)
{
    if (impl()->role() == AnimValRole) {
        setDOMException(exec, NO_MODIFICATION_ALLOWED_ERR);
        return;
    }

    if (!value.isUndefinedOrNull() && !value.isNumber() && !value.isBoolean()) {
        throwVMTypeError(exec);
        return;
    }

    SVGLength& podImp = impl()->propertyReference();

    ExceptionCode ec = 0;
    podImp.setValue(value.toFloat(exec), impl()->contextElement(), ec);
    if (ec) {
        setDOMException(exec, ec);
        return;
    }

    impl()->commitChange();
}

JSValue JSSVGLength::convertToSpecifiedUnits(ExecState* exec)
{
    if (impl()->role() == AnimValRole) {
        setDOMException(exec, NO_MODIFICATION_ALLOWED_ERR);
        return jsUndefined();
    }

    SVGLength& podImp = impl()->propertyReference();

    // Mimic the behaviour of RequiresAllArguments=Raise.
    if (exec->argumentCount() < 1)
        return throwError(exec, createSyntaxError(exec, "Not enough arguments"));

    unsigned short unitType = exec->argument(0).toUInt32(exec);
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    podImp.convertToSpecifiedUnits(unitType, impl()->contextElement(), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    impl()->commitChange();
    return jsUndefined();
}

}

#endif // ENABLE(SVG)
