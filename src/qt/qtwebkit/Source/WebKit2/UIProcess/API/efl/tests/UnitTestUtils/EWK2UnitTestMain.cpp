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

#include "EWK2UnitTestBase.h"
#include "EWK2UnitTestEnvironment.h"
#include <getopt.h>

using namespace EWK2UnitTest;

EWK2UnitTestEnvironment* environment = 0;

int main(int argc, char** argv)
{
    ewk_init();

    ::testing::InitGoogleTest(&argc, argv);

    environment = new EWK2UnitTestEnvironment();
    testing::AddGlobalTestEnvironment(environment);

    int result = RUN_ALL_TESTS();

    ewk_shutdown();

    return result;
}
