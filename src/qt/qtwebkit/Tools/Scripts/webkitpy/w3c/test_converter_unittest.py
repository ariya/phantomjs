#!/usr/bin/env python

# Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above
#    copyright notice, this list of conditions and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials
#    provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

import os
import re
import unittest2 as unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.thirdparty.BeautifulSoup import BeautifulSoup
from webkitpy.w3c.test_converter import W3CTestConverter


DUMMY_FILENAME = 'dummy.html'

class W3CTestConverterTest(unittest.TestCase):

    def fake_dir_path(self, converter, dirname):
        return converter.path_from_webkit_root("LayoutTests", "css", dirname)

    def test_read_prefixed_property_list(self):
        """ Tests that the current list of properties requiring the -webkit- prefix load correctly """

        # FIXME: We should be passing in a MockHost here ...
        converter = W3CTestConverter()
        prop_list = converter.prefixed_properties
        self.assertTrue(prop_list, 'No prefixed properties found')
        for prop in prop_list:
            self.assertTrue(prop.startswith('-webkit-'))

    def test_convert_for_webkit_nothing_to_convert(self):
        """ Tests convert_for_webkit() using a basic test that has nothing to convert """

        test_html = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>CSS Test: DESCRIPTION OF TEST</title>
<link rel="author" title="NAME_OF_AUTHOR"
href="mailto:EMAIL OR http://CONTACT_PAGE"/>
<link rel="help" href="RELEVANT_SPEC_SECTION"/>
<meta name="assert" content="TEST ASSERTION"/>
<style type="text/css"><![CDATA[
CSS FOR TEST
]]></style>
</head>
<body>
CONTENT OF TEST
</body>
</html>
"""
        converter = W3CTestConverter()

        oc = OutputCapture()
        oc.capture_output()
        try:
            converted = converter.convert_html('/nothing/to/convert', test_html, DUMMY_FILENAME)
        finally:
            oc.restore_output()

        self.verify_no_conversion_happened(converted)

    def test_convert_for_webkit_harness_only(self):
        """ Tests convert_for_webkit() using a basic JS test that uses testharness.js only and has no prefixed properties """

        test_html = """<head>
<link href="/resources/testharness.css" rel="stylesheet" type="text/css">
<script src="/resources/testharness.js"></script>
</head>
"""
        converter = W3CTestConverter()
        fake_dir_path = self.fake_dir_path(converter, "harnessonly")

        converted = converter.convert_html(fake_dir_path, test_html, DUMMY_FILENAME)

        self.verify_conversion_happened(converted)
        self.verify_test_harness_paths(converter, converted[1], fake_dir_path, 1, 1)
        self.verify_prefixed_properties(converted, [])

    def test_convert_for_webkit_properties_only(self):
        """ Tests convert_for_webkit() using a test that has 2 prefixed properties: 1 in a style block + 1 inline style """

        test_html = """<html>
<head>
<link href="/resources/testharness.css" rel="stylesheet" type="text/css">
<script src="/resources/testharness.js"></script>
<style type="text/css">

#block1 { @test0@: propvalue; }

</style>
</head>
<body>
<div id="elem1" style="@test1@: propvalue;"></div>
</body>
</html>
"""
        converter = W3CTestConverter()
        fake_dir_path = self.fake_dir_path(converter, 'harnessandprops')
        test_content = self.generate_test_content(converter.prefixed_properties, 1, test_html)

        oc = OutputCapture()
        oc.capture_output()
        try:
            converted = converter.convert_html(fake_dir_path, test_content[1], DUMMY_FILENAME)
        finally:
            oc.restore_output()

        self.verify_conversion_happened(converted)
        self.verify_test_harness_paths(converter, converted[1], fake_dir_path, 1, 1)
        self.verify_prefixed_properties(converted, test_content[0])

    def test_convert_for_webkit_harness_and_properties(self):
        """ Tests convert_for_webkit() using a basic JS test that uses testharness.js and testharness.css and has 4 prefixed properties: 3 in a style block + 1 inline style """

        test_html = """<html>
<head>
<link href="/resources/testharness.css" rel="stylesheet" type="text/css">
<script src="/resources/testharness.js"></script>
<style type="text/css">

#block1 { @test0@: propvalue; }
#block2 { @test1@: propvalue; }
#block3 { @test2@: propvalue; }

</style>
</head>
<body>
<div id="elem1" style="@test3@: propvalue;"></div>
</body>
</html>
"""
        converter = W3CTestConverter()
        fake_dir_path = self.fake_dir_path(converter, 'harnessandprops')

        oc = OutputCapture()
        oc.capture_output()
        try:
            test_content = self.generate_test_content(converter.prefixed_properties, 2, test_html)
            converted = converter.convert_html(fake_dir_path, test_content[1], DUMMY_FILENAME)
        finally:
            oc.restore_output()

        self.verify_conversion_happened(converted)
        self.verify_test_harness_paths(converter, converted[1], fake_dir_path, 1, 1)
        self.verify_prefixed_properties(converted, test_content[0])

    def test_convert_test_harness_paths(self):
        """ Tests convert_testharness_paths() with a test that uses all three testharness files """

        test_html = """<head>
<link href="/resources/testharness.css" rel="stylesheet" type="text/css">
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
</head>
"""
        converter = W3CTestConverter()

        fake_dir_path = self.fake_dir_path(converter, 'testharnesspaths')

        doc = BeautifulSoup(test_html)
        oc = OutputCapture()
        oc.capture_output()
        try:
            converted = converter.convert_testharness_paths(doc, fake_dir_path, DUMMY_FILENAME)
        finally:
            oc.restore_output()

        self.verify_conversion_happened(converted)
        self.verify_test_harness_paths(converter, doc, fake_dir_path, 2, 1)

    def test_convert_prefixed_properties(self):
        """ Tests convert_prefixed_properties() file that has 20 properties requiring the -webkit- prefix:
        10 in one style block + 5 in another style
        block + 5 inline styles, including one with multiple prefixed properties.
        The properties in the test content are in all sorts of wack formatting.
        """

        test_html = """<html>
<style type="text/css"><![CDATA[

.block1 {
    width: 300px;
    height: 300px
}

.block2 {
    @test0@: propvalue;
}

.block3{@test1@: propvalue;}

.block4 { @test2@:propvalue; }

.block5{ @test3@ :propvalue; }

#block6 {    @test4@   :   propvalue;  }

#block7
{
    @test5@: propvalue;
}

#block8 { @test6@: propvalue; }

#block9:pseudo
{

    @test7@: propvalue;
    @test8@:  propvalue propvalue propvalue;;
}

]]></style>
</head>
<body>
    <div id="elem1" style="@test9@: propvalue;"></div>
    <div id="elem2" style="propname: propvalue; @test10@ : propvalue; propname:propvalue;"></div>
    <div id="elem2" style="@test11@: propvalue; @test12@ : propvalue; @test13@   :propvalue;"></div>
    <div id="elem3" style="@test14@:propvalue"></div>
</body>
<style type="text/css"><![CDATA[

.block10{ @test15@: propvalue; }
.block11{ @test16@: propvalue; }
.block12{ @test17@: propvalue; }
#block13:pseudo
{
    @test18@: propvalue;
    @test19@: propvalue;
}

]]></style>
</html>
"""
        converter = W3CTestConverter()

        test_content = self.generate_test_content(converter.prefixed_properties, 20, test_html)

        oc = OutputCapture()
        oc.capture_output()
        try:
            converted = converter.convert_prefixed_properties(BeautifulSoup(test_content[1]), DUMMY_FILENAME)
        finally:
            oc.restore_output()

        self.verify_conversion_happened(converted)
        self.verify_prefixed_properties(converted, test_content[0])

    def verify_conversion_happened(self, converted):
        self.assertTrue(converted, "conversion didn't happen")

    def verify_no_conversion_happened(self, converted):
        self.assertEqual(converted, None, 'test should not have been converted')

    def verify_test_harness_paths(self, converter, converted, test_path, num_src_paths, num_href_paths):
        if isinstance(converted, basestring):
            converted = BeautifulSoup(converted)

        resources_dir = converter.path_from_webkit_root("LayoutTests", "resources")

        # Verify the original paths are gone, and the new paths are present.
        orig_path_pattern = re.compile('\"/resources/testharness')
        self.assertEquals(len(converted.findAll(src=orig_path_pattern)), 0, 'testharness src path was not converted')
        self.assertEquals(len(converted.findAll(href=orig_path_pattern)), 0, 'testharness href path was not converted')

        new_relpath = os.path.relpath(resources_dir, test_path)
        relpath_pattern = re.compile(new_relpath)
        self.assertEquals(len(converted.findAll(src=relpath_pattern)), num_src_paths, 'testharness src relative path not correct')
        self.assertEquals(len(converted.findAll(href=relpath_pattern)), num_href_paths, 'testharness href relative path not correct')

    def verify_prefixed_properties(self, converted, test_properties):
        self.assertEqual(len(converted[0]), len(test_properties), 'Incorrect number of properties converted')
        for test_prop in test_properties:
            self.assertTrue((test_prop in converted[1]), 'Property ' + test_prop + ' not found in converted doc')

    def generate_test_content(self, full_property_list, num_test_properties, html):
        """Inserts properties requiring a -webkit- prefix into the content, replacing \'@testXX@\' with a property."""
        test_properties = []
        count = 0
        while count < num_test_properties:
            test_properties.append(full_property_list[count])
            count += 1

        # Replace the tokens in the testhtml with the test properties. Walk backward
        # through the list to replace the double-digit tokens first
        index = len(test_properties) - 1
        while index >= 0:
            # Use the unprefixed version
            test_prop = test_properties[index].replace('-webkit-', '')
            # Replace the token
            html = html.replace('@test' + str(index) + '@', test_prop)
            index -= 1

        return (test_properties, html)
