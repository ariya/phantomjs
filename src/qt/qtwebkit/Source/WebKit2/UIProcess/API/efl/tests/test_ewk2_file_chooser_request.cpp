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

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

class EWK2FileChooserRequestTest : public EWK2UnitTestBase {
public:
    static void onFileChooserRequest(void* userData, Evas_Object*, void* eventInfo)
    {
        Ewk_File_Chooser_Request** returnRequest = static_cast<Ewk_File_Chooser_Request**>(userData);
        ASSERT_TRUE(returnRequest);
        Ewk_File_Chooser_Request* request = static_cast<Ewk_File_Chooser_Request*>(eventInfo);
        ASSERT_TRUE(request);

        // Ref the request to process asynchronously.
        *returnRequest = ewk_object_ref(request);
    }

    static int compareStrings(const void* string1, const void* string2)
    {
        ASSERT(string1);
        ASSERT(string2);

        return strcmp(static_cast<const char*>(string1), static_cast<const char*>(string2));
    }

protected:
    void freeStringList(Eina_List** list)
    {
        void* data;
        EINA_LIST_FREE(*list, data) {
        eina_stringshare_del(static_cast<char*>(data));
        }
    }

    void clickFileInput()
    {
        mouseClick(15, 15);
    }
};

TEST_F(EWK2FileChooserRequestTest, ewk_file_chooser_request_files_choose)
{
    Ewk_File_Chooser_Request* request = 0;
    evas_object_smart_callback_add(webView(), "file,chooser,request", onFileChooserRequest, &request);
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("file_chooser.html").data()));

    clickFileInput();

    // Wait for the file chooser request.
    while (!request)
        ecore_main_loop_iterate();

    evas_object_smart_callback_del(webView(), "file,chooser,request", onFileChooserRequest);
    ASSERT_TRUE(request);
    // Validate file chooser request.
    EXPECT_TRUE(ewk_file_chooser_request_allow_multiple_files_get(request));
    Eina_List* mimeTypes = ewk_file_chooser_request_accepted_mimetypes_get(request);
    mimeTypes = eina_list_sort(mimeTypes, eina_list_count(mimeTypes), compareStrings);

    ASSERT_EQ(2, eina_list_count(mimeTypes));
    EXPECT_STREQ("image/*", static_cast<char*>(eina_list_nth(mimeTypes, 0)));
    EXPECT_STREQ("video/*", static_cast<char*>(eina_list_nth(mimeTypes, 1)));
    freeStringList(&mimeTypes);

    ASSERT_FALSE(ewk_file_chooser_request_files_choose(request, 0));
    Eina_List* files = 0;
    files = eina_list_append(files, eina_stringshare_add("/tmp/file1.png"));
    files = eina_list_append(files, eina_stringshare_add("/tmp/file2.png"));
    ASSERT_TRUE(ewk_file_chooser_request_files_choose(request, files));
    ASSERT_FALSE(ewk_file_chooser_request_files_choose(request, files));
    freeStringList(&files);

    ewk_object_unref(request);

    // Check that the JS side received the files.
    EXPECT_TRUE(waitUntilTitleChangedTo("file1.png|file2.png"));
}

TEST_F(EWK2FileChooserRequestTest, ewk_file_chooser_request_file_choose)
{
    Ewk_File_Chooser_Request* request = 0;
    evas_object_smart_callback_add(webView(), "file,chooser,request", onFileChooserRequest, &request);
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("file_chooser.html").data()));

    clickFileInput();

    // Wait for the file chooser request.
    while (!request)
        ecore_main_loop_iterate();

    evas_object_smart_callback_del(webView(), "file,chooser,request", onFileChooserRequest);
    ASSERT_TRUE(request);

    ASSERT_FALSE(ewk_file_chooser_request_file_choose(request, 0));
    ASSERT_TRUE(ewk_file_chooser_request_file_choose(request, "/tmp/file3.png"));
    ASSERT_FALSE(ewk_file_chooser_request_file_choose(request, "/tmp/file3.png"));

    ewk_object_unref(request);

    // Check that the JS side received the file.
    EXPECT_TRUE(waitUntilTitleChangedTo("file3.png"));
}

TEST_F(EWK2FileChooserRequestTest, ewk_file_chooser_request_file_cancel)
{
    Ewk_File_Chooser_Request* request = 0;
    evas_object_smart_callback_add(webView(), "file,chooser,request", onFileChooserRequest, &request);
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("file_chooser.html").data()));

    clickFileInput();

    // Wait for the file chooser request.
    while (!request)
        ecore_main_loop_iterate();

    evas_object_smart_callback_del(webView(), "file,chooser,request", onFileChooserRequest);
    ASSERT_TRUE(request);

    ASSERT_TRUE(ewk_file_chooser_request_cancel(request));
    ASSERT_FALSE(ewk_file_chooser_request_cancel(request));

    ewk_object_unref(request);

    ecore_main_loop_iterate();
    EXPECT_STREQ("File chooser test", ewk_view_title_get(webView()));

    // Default behavior is to cancel if the client does not act on the request.
    request = 0;
    evas_object_smart_callback_add(webView(), "file,chooser,request", onFileChooserRequest, &request);

    clickFileInput();

    // Wait for the file chooser request.
    while (!request)
        ecore_main_loop_iterate();

    evas_object_smart_callback_del(webView(), "file,chooser,request", onFileChooserRequest);
    ASSERT_TRUE(request);

    ewk_object_unref(request);

    ecore_main_loop_iterate();
    EXPECT_STREQ("File chooser test", ewk_view_title_get(webView()));
}
