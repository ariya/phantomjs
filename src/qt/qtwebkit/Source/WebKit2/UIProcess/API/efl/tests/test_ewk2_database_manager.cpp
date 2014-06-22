/*
 * Copyright (C) 2012 Samsung Electronics
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

using namespace EWK2UnitTest;

class EWK2DatabaseManagerTest : public EWK2UnitTestBase {
public:
    struct OriginData {
        Eina_List* originList;
        Ewk_Database_Manager* manager;
        bool didReceiveOriginsCallback;
        bool isSynchronized;
        unsigned timeoutSeconds;

        OriginData()
            : originList(0)
            , manager(0)
            , didReceiveOriginsCallback(false)
            , isSynchronized(false)
            , timeoutSeconds(10)
        { }
    };

    static void databaseOriginsCallback(Eina_List* origins, Ewk_Error* error, void* userData)
    {
        ASSERT_FALSE(error);

        OriginData* originData = static_cast<OriginData*>(userData);
        originData->didReceiveOriginsCallback = true;

        Eina_List* l;
        void* data;
        EINA_LIST_FOREACH(origins, l, data) {
            originData->originList = eina_list_append(originData->originList, data);
            Ewk_Security_Origin* origin = static_cast<Ewk_Security_Origin*>(data);
            if (!strcmp(ewk_security_origin_protocol_get(origin), "http")
                && !strcmp(ewk_security_origin_host_get(origin), "www.databasetest.com")
                && !ewk_security_origin_port_get(origin)) {
                    originData->isSynchronized = true;
                    ecore_main_loop_quit();
            }
        }
    }

    static Eina_Bool timerCallback(void* userData)
    {
        OriginData* originData = static_cast<OriginData*>(userData);

        if (originData->isSynchronized || !--(originData->timeoutSeconds)) {
            ecore_main_loop_quit();
            return ECORE_CALLBACK_CANCEL;
        }

        if (originData->didReceiveOriginsCallback) {
            originData->didReceiveOriginsCallback = false;
            ewk_database_manager_origins_get(originData->manager, databaseOriginsCallback, originData);
        }

        return ECORE_CALLBACK_RENEW;
    }
};

TEST_F(EWK2DatabaseManagerTest, ewk_database_manager_origins_get)
{
    Evas_Object* view = webView();
    const char* databaseHTML =
        "<html><head><title>original title</title></head>"
        "<body>"
        "<script type='text/javascript'>"
        " var db = openDatabase(\"DBTest\", \"1.0\", \"HTML5 Database example\", 200000);"
        "</script>"
        "</body></html>";

    ASSERT_TRUE(ewk_view_html_string_load(view, databaseHTML, "http://www.databasetest.com", 0));
    ASSERT_TRUE(waitUntilLoadFinished());

    OriginData originData;
    originData.manager = ewk_context_database_manager_get(ewk_view_context_get(view));
    ASSERT_TRUE(ewk_database_manager_origins_get(originData.manager, databaseOriginsCallback, &originData));
    Ecore_Timer* database_timer = ecore_timer_add(1, timerCallback, &originData);

    ecore_main_loop_begin();
    if (database_timer)
        ecore_timer_del(database_timer);

    ASSERT_TRUE(originData.isSynchronized);
    ASSERT_LE(1, eina_list_count(originData.originList));
}
