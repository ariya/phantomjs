/*
 * Copyright (C) 2013 Cisco Systems, Inc. 
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

/**
 * @brief Unit test for checking set/get of default quota for Web Database databases by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_web_database_default_quota)
{
    ASSERT_EQ(1 * 1024 * 1024, ewk_settings_web_database_default_quota_get());

    ewk_settings_web_database_default_quota_set(2 * 1024 * 1024);
    ASSERT_EQ(2 * 1024 * 1024, ewk_settings_web_database_default_quota_get());

    ewk_settings_web_database_default_quota_set(3 * 1024 * 1024);
    ASSERT_EQ(3 * 1024 * 1024, ewk_settings_web_database_default_quota_get());
}

/**
 * @brief Unit test for checking set/get of directory path where Web Database databases is stored by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_web_database_path)
{
    char* homePath = getenv("HOME");
    char* defaultPath = reinterpret_cast<char*>(malloc(strlen(homePath) + strlen("/.cache/WebKitEfl/Databases") + 1));

#if ENABLE(SQL_DATABASE)
    strncpy(defaultPath, homePath, strlen(homePath) + 1);
    ASSERT_STREQ(strcat(defaultPath, "/.cache/WebKitEfl/Databases"), ewk_settings_web_database_path_get());
#else
    ASSERT_STREQ(0, ewk_settings_web_database_path_get());
#endif
    free(defaultPath);

    ewk_settings_web_database_path_set("~/data/webkitDB");
#if ENABLE(SQL_DATABASE)
    ASSERT_STREQ("~/data/webkitDB", ewk_settings_web_database_path_get());
#else
    ASSERT_STREQ(0, ewk_settings_web_database_path_get());
#endif

    ewk_settings_web_database_path_set("~/tmp/webkit");
#if ENABLE(SQL_DATABASE)    
    ASSERT_STREQ("~/tmp/webkit", ewk_settings_web_database_path_get());
#else
    ASSERT_STREQ(0, ewk_settings_web_database_path_get());
#endif
}

/**
 * @brief Unit test for checking set/get of directory path where the HTML5 local storage indexing database is stored by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_local_storage_path)
{
    char* homePath = getenv("HOME");
    char* defaultPath = reinterpret_cast<char*>(malloc(strlen(homePath) + strlen("/.local/share/WebKitEfl/LocalStorage") + 1));

    strncpy(defaultPath, homePath, strlen(homePath) + 1);
    ASSERT_STREQ(strcat(defaultPath, "/.local/share/WebKitEfl/LocalStorage"), ewk_settings_local_storage_path_get());
    free(defaultPath);

    ewk_settings_local_storage_path_set("~/data/webkitDB");
    ASSERT_STREQ("~/data/webkitDB", ewk_settings_local_storage_path_get());

    ewk_settings_local_storage_path_set("~/tmp/webkit");
    ASSERT_STREQ("~/tmp/webkit", ewk_settings_local_storage_path_get());
}

/**
 * @brief Unit test for checking set/get of directory path where icon database is stored by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_icon_database_path)
{
    ASSERT_STREQ(0, ewk_settings_icon_database_path_get());

    ASSERT_TRUE(ewk_settings_icon_database_path_set("/tmp"));
    ASSERT_STREQ("/tmp", ewk_settings_icon_database_path_get());

    ASSERT_TRUE(ewk_settings_icon_database_path_set(0));
    ASSERT_STREQ(0, ewk_settings_icon_database_path_get());
}

/**
 * @brief Unit test for checking set/get of path where the HTML5 application cache is stored by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_application_cache_path)
{
    char* homePath = getenv("HOME");
    char* defaultPath = reinterpret_cast<char*>(malloc(strlen(homePath) + strlen("/.cache/WebKitEfl/Applications") + 1));

    strncpy(defaultPath, homePath, strlen(homePath) + 1);
    ASSERT_STREQ(strcat(defaultPath, "/.cache/WebKitEfl/Applications"), ewk_settings_application_cache_path_get());
    free(defaultPath);

    ewk_settings_application_cache_path_set("~/data/webkitApp");
    ASSERT_STREQ("~/data/webkitApp", ewk_settings_application_cache_path_get());

    ewk_settings_application_cache_path_set("~/tmp/webkitApp");
    ASSERT_STREQ("~/tmp/webkitApp", ewk_settings_application_cache_path_get());
}

/**
 * @brief Unit test for checking set/get of maximum size of the application cache for HTML5 Offline Web Applications by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_application_cache_max_quota)
{
    ASSERT_EQ(std::numeric_limits<int64_t>::max(), ewk_settings_application_cache_max_quota_get());

    ewk_settings_application_cache_max_quota_set(3 * 1024 * 1024);
    ASSERT_EQ(3 * 1024 * 1024, ewk_settings_application_cache_max_quota_get());

    ewk_settings_application_cache_max_quota_set(5 * 1024 * 1024);
    ASSERT_EQ(5 * 1024 * 1024, ewk_settings_application_cache_max_quota_get());
}

/**
 * @brief Unit test for checking set/get of in-memory object cache Enables/Disables status by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_object_cache_enable)
{
    ASSERT_TRUE(ewk_settings_object_cache_enable_get());

    ewk_settings_object_cache_enable_set(false);
    ASSERT_FALSE(ewk_settings_object_cache_enable_get());

    ewk_settings_object_cache_enable_set(true);
    ASSERT_TRUE(ewk_settings_object_cache_enable_get());
}

/**
 * @brief Unit test for checking set/get of maximum number of pages in the memory page cache by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_page_cache_capacity)
{
    ASSERT_EQ(3, ewk_settings_page_cache_capacity_get());

    ewk_settings_page_cache_capacity_set(5);
    ASSERT_EQ(5, ewk_settings_page_cache_capacity_get());
}

/**
 * @brief Unit test for checking set/get of css media type by ewk settings API.
 */
TEST_F(EWKTestBase, ewk_settings_css_media_type)
{
    ASSERT_STREQ(0, ewk_settings_css_media_type_get());

    ewk_settings_css_media_type_set("handheld");
    ASSERT_STREQ("handheld", ewk_settings_css_media_type_get());

    ewk_settings_css_media_type_set("tv");
    ASSERT_STREQ("tv", ewk_settings_css_media_type_get());

    ewk_settings_css_media_type_set("screen");
    ASSERT_STREQ("screen", ewk_settings_css_media_type_get());

    ewk_settings_css_media_type_set(0);
    ASSERT_STREQ(0, ewk_settings_css_media_type_get());
}
