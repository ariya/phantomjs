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

#ifndef EWKTestBase_h
#define EWKTestBase_h

#include "EWKTestConfig.h"
#include "EWKTestView.h"
#include <gtest/gtest.h>

namespace EWKUnitTests {

class EWKTestBase: public ::testing::Test {
public:
    static void onLoadFinished(void* data, Evas_Object* webView, void* eventInfo);

    Evas_Object* webView();
    virtual void SetUp();
protected:
    EWKTestBase();

    void loadUrl(const char* url = Config::defaultTestPage);
    void waitUntilLoadFinished();

    EWKTestView m_ewkTestView;
};

}

#endif
