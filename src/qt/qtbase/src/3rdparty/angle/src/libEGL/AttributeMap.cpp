//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libEGL/AttributeMap.h"

namespace egl
{

AttributeMap::AttributeMap()
{
}

AttributeMap::AttributeMap(const EGLint *attributes)
{
    for (const EGLint *curAttrib = attributes; curAttrib[0] != EGL_NONE; curAttrib += 2)
    {
        insert(curAttrib[0], curAttrib[1]);
    }
}

void AttributeMap::insert(EGLint key, EGLint value)
{
    mAttributes[key] = value;
}

bool AttributeMap::contains(EGLint key) const
{
    return (mAttributes.find(key) != mAttributes.end());
}

EGLint AttributeMap::get(EGLint key, EGLint defaultValue) const
{
    std::map<EGLint, EGLint>::const_iterator iter = mAttributes.find(key);
    return (mAttributes.find(key) != mAttributes.end()) ? iter->second : defaultValue;
}

}
