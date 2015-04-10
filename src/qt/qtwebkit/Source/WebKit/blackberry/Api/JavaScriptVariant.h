/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef JavaScriptVariant_h
#define JavaScriptVariant_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformString.h>

namespace BlackBerry {
namespace WebKit {

class BLACKBERRY_EXPORT JavaScriptVariant {
public:
    enum DataType {
        Undefined = 0,
        Null,
        Boolean,
        Number,
        String,
        Object,
        Exception
    };

    JavaScriptVariant();
    JavaScriptVariant(const JavaScriptVariant&);
    JavaScriptVariant(double);
    JavaScriptVariant(int);
    explicit JavaScriptVariant(bool);
    JavaScriptVariant(const BlackBerry::Platform::String&);
    ~JavaScriptVariant();

    JavaScriptVariant& operator=(const JavaScriptVariant&);

    void setType(const DataType&);
    DataType type() const;

    void setDouble(double);
    double doubleValue() const;

    void setString(const BlackBerry::Platform::String&);
    const BlackBerry::Platform::String& stringValue() const;

    void setBoolean(bool);
    bool booleanValue() const;

private:
    JavaScriptVariant(const char*);
    DataType m_type;

    union {
        bool m_booleanValue;
        double m_doubleValue;
    };

    BlackBerry::Platform::String m_stringValue;
};

}
}

#endif // JavaScriptVariant_h
