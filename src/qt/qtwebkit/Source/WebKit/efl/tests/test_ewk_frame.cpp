/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Red istributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "UnitTestUtils/EWKTestBase.h"
#include "UnitTestUtils/EWKTestConfig.h"
#include <EWebKit.h>

using namespace EWKUnitTests;

static void initBuffer(char** buffer)
{
    free(*buffer);
    *buffer = 0;
}

/**
 * @brief Unit test for ewk_frame_source_get.
 */
TEST_F(EWKTestBase, ewk_frame_source_get)
{
    char* buffer = 0;
    ssize_t failed = -1; 

    // Checking if function works properly without loading url.
    ssize_t read = ewk_frame_source_get(ewk_view_frame_main_get(webView()), &buffer);
    ASSERT_EQ(read, failed);
    initBuffer(&buffer);

    // FIXME: BUG 49246 has changed load behavior when malformed url is inputed. Timer operation might be needed to sync with WK2.
    // See https://bugs.webkit.org/show_bug.cgi?id=105620 for more details.
    // Checking if function works properly when loading non-existing url.
    // loadUrl("http://www.abcdefg^^.com");
    // read = ewk_frame_source_get(ewk_view_frame_main_get(webView()), &buffer);
    // ASSERT_EQ(read, failed);
    // initBuffer(&buffer);

    // Checking if function works properly when finishing load url.
    loadUrl();
    read = ewk_frame_source_get(ewk_view_frame_main_get(webView()), &buffer);
    ASSERT_GT(read, 0);
    initBuffer(&buffer);
}
