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

#include "config.h"
#include "EWK2UnitTestEnvironment.h"

#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/WTFString.h>

namespace EWK2UnitTest {

EWK2UnitTestEnvironment::EWK2UnitTestEnvironment()
    : m_defaultWidth(800)
    , m_defaultHeight(600)
{
}

const char* EWK2UnitTestEnvironment::defaultTestPageUrl() const
{
    return "file://"TEST_RESOURCES_DIR"/default_test_page.html";
}

const char* EWK2UnitTestEnvironment::defaultTheme() const
{
    return TEST_THEME_DIR"/default.edj";
}

const char* EWK2UnitTestEnvironment::injectedBundleSample() const
{
    return TEST_LIB_DIR "/libewk2UnitTestInjectedBundleSample.so";
}

CString EWK2UnitTestEnvironment::urlForResource(const char* resource)
{
    return makeString("file://"TEST_RESOURCES_DIR"/", resource).utf8();
}

CString EWK2UnitTestEnvironment::pathForResource(const char* resource)
{
    StringBuilder builder;
    builder.appendLiteral(TEST_RESOURCES_DIR "/");
    builder.append(resource);
    return builder.toString().utf8();
}

CString EWK2UnitTestEnvironment::pathForTheme(const char* theme)
{
    StringBuilder builder;
    builder.appendLiteral(TEST_THEME_DIR "/");
    builder.append(theme);
    return builder.toString().utf8();
}

} // namespace EWK2UnitTest
