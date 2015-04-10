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

#include "ExceptionCode.h"
#include "SVGAnimatedProperty.h"
#include "SVGException.h"
#include "SVGLengthContext.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

JSValue JSSVGLength::value(ExecState* exec) const
{
    SVGLength& podImp = impl()->propertyReference();
    ExceptionCode ec = 0;
    SVGLengthContext lengthContext(impl()->contextElement());
    float value = podImp.value(lengthContext, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return jsNumber(value);
}

void JSSVGLength::setValue(ExecState* exec, JSValue value)
{
    if (impl()->isReadOnly()) {
        setDOMException(exec, NO_MODIFICATION_ALLOWED_ERR);
        return;
    }

    if (!value.isUndefinedOrNull() && !value.isNumber() && !value.isBoolean()) {
        throwVMTypeError(exec);
        return;
    }

    SVGLength& podImp = impl()->propertyReference();

    ExceptionCode ec = 0;
    SVGLengthContext lengthContext(impl()->contextElement());
    podImp.setValue(value.toFloat(exec), lengthContext, ec);
    if (ec) {
        setDOMException(exec, ec);
        return;
    }

    impl()->commitChange();
}

JSValue JSSVGLength::convertToSpecifiedUnits(ExecState* exec)
{
    if (impl()->isReadOnly()) {
        setDOMException(exec, NO_MODIFICATION_ALLOWED_ERR);
        return jsUndefined();
    }

    SVGLength& podImp = impl()->propertyReference();

    if (exec->argumentCount() < 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    unsigned short unitType = exec->argument(0).toUInt32(exec);
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    SVGLengthContext lengthContext(impl()->contextElement());
    podImp.convertToSpecifiedUnits(unitType, lengthContext, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    impl()->commitChange();
    return jsUndefined();
}

}

#endif // ENABLE(SVG)
