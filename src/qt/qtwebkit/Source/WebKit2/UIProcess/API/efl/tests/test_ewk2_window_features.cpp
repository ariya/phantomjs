/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include <EWebKit2.h>
#include <Ecore.h>
#include <WebCore/IntRect.h>

using namespace EWK2UnitTest;
using namespace WebCore;

class EWK2WindowFeaturesTest : public EWK2UnitTestBase {
public:
    static Evas_Object* createDefaultWindow(Ewk_View_Smart_Data* smartData, const char*, const Ewk_Window_Features* windowFeatures)
    {
        // default values of WebCore:WindowFeatures()
        // - menuBarVisible(true)
        // - statusBarVisible(true)
        // - toolBarVisible(true)
        // - locationBarVisible(true)
        // - scrollbarsVisible(true)
        // - resizable(true)
        // - fullscreen(false)

        EXPECT_TRUE(ewk_window_features_toolbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_statusbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_scrollbars_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_menubar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_locationbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_resizable_get(windowFeatures));

        EXPECT_FALSE(ewk_window_features_fullscreen_get(windowFeatures));

        int x, y, width, height;
        ewk_window_features_geometry_get(windowFeatures, &x, &y, &width, &height);

        EXPECT_EQ(0, x);
        EXPECT_EQ(0, y);
        EXPECT_EQ(0, width);
        EXPECT_EQ(0, height);

        return 0;
    }

    static Evas_Object* createWindow(Ewk_View_Smart_Data *smartData, const char*, const Ewk_Window_Features *windowFeatures)
    {
        EXPECT_FALSE(ewk_window_features_toolbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_statusbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_scrollbars_visible_get(windowFeatures));
        EXPECT_FALSE(ewk_window_features_menubar_visible_get(windowFeatures));
        EXPECT_FALSE(ewk_window_features_locationbar_visible_get(windowFeatures));
        EXPECT_TRUE(ewk_window_features_resizable_get(windowFeatures));
        EXPECT_FALSE(ewk_window_features_fullscreen_get(windowFeatures));

        int x, y, width, height;
        ewk_window_features_geometry_get(windowFeatures, &x, &y, &width, &height);

        EXPECT_EQ(100, x);
        EXPECT_EQ(150, y);
        EXPECT_EQ(400, width);
        EXPECT_EQ(400, height);

        return 0;
    }
};

TEST_F(EWK2WindowFeaturesTest, ewk_window_features_default_property_get)
{
    Evas_Object* view = webView();

    const char windowHTML[] = "<html><body onLoad=\"window.open('', '');\"></body></html>";

    ewkViewClass()->window_create = createDefaultWindow;

    ewk_view_html_string_load(view, windowHTML, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());
}

TEST_F(EWK2WindowFeaturesTest, ewk_window_features_property_get)
{
    Evas_Object* view = webView();

    const char windowHTML[] = "<html><body onLoad=\"window.open('', '', 'left=100,top=150,width=400,height=400,location=no,menubar=no,status=yes,toolbar=no,scrollbars=yes,resizable=yes,fullscreen=no');\"></body></html>";

    ewkViewClass()->window_create = createWindow;

    ewk_view_html_string_load(view, windowHTML, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());
}
