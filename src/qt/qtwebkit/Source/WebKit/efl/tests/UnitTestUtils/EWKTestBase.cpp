/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"
#include "EWKTestBase.h"

#include "EWKTestEnvironment.h"
#include <EWebKit.h>
#include <Ecore.h>
#include <Edje.h>

extern EWKUnitTests::EWKTestEnvironment* environment;

namespace EWKUnitTests {

EWKTestBase::EWKTestBase()
{
}

Evas_Object* EWKTestBase::webView()
{
    return m_ewkTestView.webView();
}

void EWKTestBase::SetUp()
{
    ASSERT_TRUE(m_ewkTestView.init());
}

void EWKTestBase::onLoadFinished(void* data, Evas_Object* webView, void* eventInfo)
{
    ecore_main_loop_quit();
}

void EWKTestBase::waitUntilLoadFinished()
{
    evas_object_smart_callback_add(webView(), "load,finished", onLoadFinished, 0);
    ecore_main_loop_begin();
    evas_object_smart_callback_del(webView(), "load,finished", onLoadFinished);
}

void EWKTestBase::loadUrl(const char* url)
{
    ASSERT_TRUE(ewk_view_uri_set(webView(), url));
    waitUntilLoadFinished();
}

}
