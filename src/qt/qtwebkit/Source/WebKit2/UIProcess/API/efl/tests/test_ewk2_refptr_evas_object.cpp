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
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/efl/RefPtrEfl.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

TEST_F(EWK2UnitTestBase, construction)
{
    RefPtr<Evas_Object> nullObject = 0;
    ASSERT_FALSE(nullObject);

    RefPtr<Evas_Object> object;
    ASSERT_FALSE(object);

    object = adoptRef(evas_object_box_add(canvas()));
    ASSERT_TRUE(object);

    object = 0;
    ASSERT_FALSE(object);

    object = adoptRef(evas_object_box_add(canvas()));
    ASSERT_TRUE(object);

    object.clear();
    ASSERT_FALSE(object);
}

TEST_F(EWK2UnitTestBase, reffing)
{
    RefPtr<Evas_Object> object = adoptRef(evas_object_box_add(canvas()));
    ASSERT_TRUE(object);
    // Evas_Objec external ref count is not as you would expect.
    ASSERT_EQ(0, evas_object_ref_get(object.get()));

    {
        RefPtr<Evas_Object> aRef = object;
        ASSERT_TRUE(object);
        ASSERT_TRUE(aRef);
        ASSERT_EQ(1, evas_object_ref_get(object.get()));
        ASSERT_EQ(1, evas_object_ref_get(aRef.get()));

        {
            RefPtr<Evas_Object> bRef = object;

            ASSERT_TRUE(object);
            ASSERT_TRUE(aRef);
            ASSERT_TRUE(bRef);

            ASSERT_EQ(2, evas_object_ref_get(object.get()));
            ASSERT_EQ(2, evas_object_ref_get(aRef.get()));

            RefPtr<Evas_Object> cRef = bRef;
            ASSERT_TRUE(cRef);

            ASSERT_EQ(3, evas_object_ref_get(object.get()));
            ASSERT_EQ(3, evas_object_ref_get(aRef.get()));
            ASSERT_EQ(3, evas_object_ref_get(bRef.get()));
            ASSERT_EQ(3, evas_object_ref_get(cRef.get()));

            bRef.clear();
            ASSERT_EQ(2, evas_object_ref_get(object.get()));
            ASSERT_EQ(2, evas_object_ref_get(aRef.get()));
            ASSERT_EQ(2, evas_object_ref_get(cRef.get()));
        }
        ASSERT_EQ(1, evas_object_ref_get(object.get()));
        ASSERT_EQ(1, evas_object_ref_get(aRef.get()));
    }
    ASSERT_EQ(0, evas_object_ref_get(object.get()));
}

TEST_F(EWK2UnitTestBase, destruction)
{
    RefPtr<Evas_Object> object = adoptRef(evas_object_box_add(canvas()));
    ASSERT_TRUE(object);
    ASSERT_EQ(0, evas_object_ref_get(object.get()));

    RefPtr<Evas_Object> aRef = object;
    ASSERT_TRUE(object);
    ASSERT_TRUE(aRef);
    ASSERT_EQ(1, evas_object_ref_get(object.get()));
    ASSERT_EQ(1, evas_object_ref_get(aRef.get()));

    object = nullptr;
    ASSERT_EQ(0, evas_object_ref_get(object.get()));
    ASSERT_EQ(0, evas_object_ref_get(aRef.get()));

    object = aRef;
    ASSERT_EQ(1, evas_object_ref_get(object.get()));
    ASSERT_EQ(1, evas_object_ref_get(aRef.get()));

    object = 0;
    ASSERT_EQ(0, evas_object_ref_get(object.get()));
    ASSERT_EQ(0, evas_object_ref_get(aRef.get()));

    aRef.clear();
    ASSERT_FALSE(aRef);
    ASSERT_FALSE(object);
}

