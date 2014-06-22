/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
	Filename:     test.js
	Description:  'Tests regular expressions method compile'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: test';

	writeHeaderToLog('Executing script: test.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	testcases[count++] = new TestCase ( SECTION,
	                                    "/[0-9]{3}/.test('23 2 34 678 9 09')",
	                                    true, /[0-9]{3}/.test('23 2 34 678 9 09'));

	testcases[count++] = new TestCase ( SECTION,
	                                    "/[0-9]{3}/.test('23 2 34 78 9 09')",
	                                    false, /[0-9]{3}/.test('23 2 34 78 9 09'));

    testcases[count++] = new TestCase ( SECTION,
                                        "/\w+ \w+ \w+/.test('do a test')",
                                        true, /\w+ \w+ \w+/.test("do a test"));

    testcases[count++] = new TestCase ( SECTION,
                                        "/\w+ \w+ \w+/.test('a test')",
                                        false, /\w+ \w+ \w+/.test("a test"));

	testcases[count++] = new TestCase ( SECTION,
	                                    "(new RegExp('[0-9]{3}')).test('23 2 34 678 9 09')",
	                                    true, (new RegExp('[0-9]{3}')).test('23 2 34 678 9 09'));

	testcases[count++] = new TestCase ( SECTION,
	                                    "(new RegExp('[0-9]{3}')).test('23 2 34 78 9 09')",
	                                    false, (new RegExp('[0-9]{3}')).test('23 2 34 78 9 09'));

    testcases[count++] = new TestCase ( SECTION,
                                        "(new RegExp('\\\\w+ \\\\w+ \\\\w+')).test('do a test')",
                                        true, (new RegExp('\\w+ \\w+ \\w+')).test("do a test"));

    testcases[count++] = new TestCase ( SECTION,
                                        "(new RegExp('\\\\w+ \\\\w+ \\\\w+')).test('a test')",
                                        false, (new RegExp('\\w+ \\w+ \\w+')).test("a test"));

	function test()
	{
	   for ( tc=0; tc < testcases.length; tc++ ) {
	        testcases[tc].passed = writeTestCaseResult(
	        testcases[tc].expect,
	        testcases[tc].actual,
	        testcases[tc].description +" = "+
	        testcases[tc].actual );
	        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
	   }
	   stopTest();
	   return ( testcases );
	}

	test();
