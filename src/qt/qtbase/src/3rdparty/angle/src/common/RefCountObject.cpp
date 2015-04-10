//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RefCountObject.cpp: Defines the gl::RefCountObject base class that provides
// lifecycle support for GL objects using the traditional BindObject scheme, but
// that need to be reference counted for correct cross-context deletion.
// (Concretely, textures, buffers and renderbuffers.)

#include "RefCountObject.h"

RefCountObject::RefCountObject(GLuint id)
{
    mId = id;
    mRefCount = 0;
}

RefCountObject::~RefCountObject()
{
    ASSERT(mRefCount == 0);
}

void RefCountObject::addRef() const
{
    mRefCount++;
}

void RefCountObject::release() const
{
    ASSERT(mRefCount > 0);

    if (--mRefCount == 0)
    {
        delete this;
    }
}

void RefCountObjectBindingPointer::set(RefCountObject *newObject)
{
    // addRef first in case newObject == mObject and this is the last reference to it.
    if (newObject != NULL) newObject->addRef();
    if (mObject != NULL) mObject->release();

    mObject = newObject;
}
