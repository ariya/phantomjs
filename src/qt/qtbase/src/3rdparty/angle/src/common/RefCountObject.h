//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RefCountObject.h: Defines the gl::RefCountObject base class that provides
// lifecycle support for GL objects using the traditional BindObject scheme, but
// that need to be reference counted for correct cross-context deletion.
// (Concretely, textures, buffers and renderbuffers.)

#ifndef COMMON_REFCOUNTOBJECT_H_
#define COMMON_REFCOUNTOBJECT_H_

#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"

class RefCountObject
{
  public:
    explicit RefCountObject(GLuint id);
    virtual ~RefCountObject();

    virtual void addRef() const;
    virtual void release() const;

    GLuint id() const { return mId; }
    
  private:
    GLuint mId;

    mutable std::size_t mRefCount;
};

class RefCountObjectBindingPointer
{
  protected:
    RefCountObjectBindingPointer() : mObject(NULL) { }
    ~RefCountObjectBindingPointer() { ASSERT(mObject == NULL); } // Objects have to be released before the resource manager is destroyed, so they must be explicitly cleaned up.

    void set(RefCountObject *newObject);
    RefCountObject *get() const { return mObject; }

  public:
    GLuint id() const { return (mObject != NULL) ? mObject->id() : 0; }
    bool operator ! () const { return (get() == NULL); }

  private:
    RefCountObject *mObject;
};

template <class ObjectType>
class BindingPointer : public RefCountObjectBindingPointer
{
  public:
    void set(ObjectType *newObject) { RefCountObjectBindingPointer::set(newObject); }
    ObjectType *get() const { return static_cast<ObjectType*>(RefCountObjectBindingPointer::get()); }
    ObjectType *operator -> () const { return get(); }
};

#endif   // COMMON_REFCOUNTOBJECT_H_
