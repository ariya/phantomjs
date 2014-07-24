/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#ifndef EWK2UnitTestBase_h
#define EWK2UnitTestBase_h

#include "EWK2UnitTestEnvironment.h"
#include <EWebKit2.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Eina.h>
#include <Evas.h>
#include <gtest/gtest.h>

namespace EWK2UnitTest {

class EWK2UnitTestBase : public ::testing::Test {
public:
    Ecore_Evas* backingStore() { return m_ecoreEvas; }
    Evas* canvas() { return ecore_evas_get(m_ecoreEvas); }
    Evas_Object* webView() { return m_webView; }
    void setWebView(Evas_Object* webView) { m_webView = webView; }
    Ewk_View_Smart_Class* ewkViewClass() { return &m_ewkViewClass; }

protected:
    EWK2UnitTestBase();

    virtual void SetUp();
    virtual void TearDown();

    static const double defaultTimeoutSeconds = 10.0;

    bool loadUrlSync(const char* url, double timeoutSeconds = defaultTimeoutSeconds);
    bool waitUntilLoadFinished(double timeoutSeconds = defaultTimeoutSeconds);
    bool waitUntilTitleChangedTo(const char* expectedTitle, double timeoutSeconds = defaultTimeoutSeconds);
    bool waitUntilURLChangedTo(const char* expectedURL, double timeoutSeconds = defaultTimeoutSeconds);
    bool waitUntilTrue(bool &flag, double timeoutSeconds = defaultTimeoutSeconds);

    void mouseClick(int x, int y, int button = 1 /*Left*/);
    void mouseDoubleClick(int x, int y, int button = 1 /*Left*/);
    void mouseDown(int x, int y, int button = 1 /*Left*/);
    void mouseUp(int x, int y, int button = 1 /*Left*/);
    void mouseMove(int x, int y);
    void multiDown(int id, int x, int y);
    void multiUp(int id, int x, int y);
    void multiMove(int id, int x, int y);

private:
    Evas_Object* m_webView;
    Ecore_Evas* m_ecoreEvas;
    Ewk_View_Smart_Class m_ewkViewClass;
};

} // namespace EWK2UnitTest

#endif // EWK2UnitTestBase_h
