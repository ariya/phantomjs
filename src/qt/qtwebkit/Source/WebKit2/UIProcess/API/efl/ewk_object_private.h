/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_object_private_h
#define ewk_object_private_h

#include <wtf/RefCounted.h>

class EwkObject : public RefCounted<EwkObject> {
public:
    virtual ~EwkObject() { }
    virtual const char* instanceClassName() const = 0;
};

template <class T>
inline bool ewk_object_is_of_type(const EwkObject* object)
{
    return (reinterpret_cast<T>(0)->className() == object->instanceClassName());
}

template <class T>
inline bool ewk_object_cast_check(const EwkObject* object)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(object, false);

    if (!ewk_object_is_of_type<T>(object)) {
        EINA_LOG_CRIT("attempt to convert object of type %s to type %s",
            object->instanceClassName(), reinterpret_cast<T>(0)->className());
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

template <class T>
inline const T ewk_object_cast(const EwkObject* object)
{
    return ewk_object_cast_check<T>(object) ? static_cast<T>(object) : 0;
}

template <class T>
inline T ewk_object_cast(EwkObject* object)
{
    return ewk_object_cast_check<T>(object) ? static_cast<T>(object) : 0;
}

#define EWK_OBJ_GET_IMPL_OR_RETURN(ImplClass, object, impl, ...)           \
    ImplClass* impl = ewk_object_cast<ImplClass*>(object);                 \
    if (!impl)                                                             \
        return __VA_ARGS__


#define EWK_OBJECT_DECLARE(_className)                                     \
static const char* className()                                             \
{                                                                          \
    static const char* name = #_className;                                 \
    return name;                                                           \
}                                                                          \
virtual const char* instanceClassName() const                              \
{                                                                          \
    return className();                                                    \
}

#endif // ewk_object_private_h
