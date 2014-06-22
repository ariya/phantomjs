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

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "UnitTestUtils/EWK2UnitTestServer.h"
#include "WKEinaSharedString.h"
#include "ewk_object_private.h"
#include <wtf/PassRefPtr.h>

using namespace EWK2UnitTest;
using namespace WTF;

extern EWK2UnitTestEnvironment* environment;

class TestEwkObject1 : public EwkObject {
public:
    EWK_OBJECT_DECLARE(TestEwkObject)

    static PassRefPtr<TestEwkObject1> create()
    {
        wasDeleted = false;
        return adoptRef(new TestEwkObject1());
    }

    static bool wasDeleted; // We always test only one instance of TestEwkObject.

    ~TestEwkObject1()
    {
        wasDeleted = true;
    }
};

bool TestEwkObject1::wasDeleted = false;

class TestEwkObject2 : public EwkObject {
public:
    EWK_OBJECT_DECLARE(TestEwkObject2)

    static PassRefPtr<TestEwkObject2> create()
    {
        return adoptRef(new TestEwkObject2());
    }
};

TEST_F(EWK2UnitTestBase, ewk_object_ref)
{
    Ewk_Object* objectRef = 0;

    {
        RefPtr<Ewk_Object> object = TestEwkObject1::create();
        ASSERT_FALSE(TestEwkObject1::wasDeleted);
        ASSERT_EQ(1, object->refCount());

        objectRef = object.get();
        ewk_object_ref(objectRef);
        ASSERT_EQ(2, objectRef->refCount());
    }

    ASSERT_FALSE(TestEwkObject1::wasDeleted);
    ASSERT_EQ(1, objectRef->refCount());

    ewk_object_unref(objectRef);
    ASSERT_TRUE(TestEwkObject1::wasDeleted);
}

TEST_F(EWK2UnitTestBase, ewk_object_is_of_type)
{
    RefPtr<EwkObject> object1 = TestEwkObject1::create();
    RefPtr<EwkObject> object2 = TestEwkObject2::create();

    ASSERT_TRUE(ewk_object_is_of_type<TestEwkObject1*>(object1.get()));
    ASSERT_TRUE(ewk_object_is_of_type<TestEwkObject2*>(object2.get()));

    ASSERT_FALSE(ewk_object_is_of_type<TestEwkObject1*>(object2.get()));
    ASSERT_FALSE(ewk_object_is_of_type<TestEwkObject2*>(object1.get()));
}

TEST_F(EWK2UnitTestBase, ewk_object_cast)
{
    RefPtr<EwkObject> object1 = TestEwkObject1::create();
    RefPtr<EwkObject> object2 = TestEwkObject2::create();

    TestEwkObject1* objectRef1 = ewk_object_cast<TestEwkObject1*>(object1.get());
    ASSERT_TRUE(objectRef1);

    TestEwkObject2* objectRef2 = ewk_object_cast<TestEwkObject2*>(object2.get());
    ASSERT_TRUE(objectRef2);
}
