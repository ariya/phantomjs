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
	Filename:     RegExp_lastMatch_as_array.js
	Description:  'Tests RegExps $& property (same tests as RegExp_lastMatch.js but using $&)'

	Author:       Nick Lerissa
	Date:         March 13, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: $&';

	writeHeaderToLog('Executing script: RegExp_lastMatch_as_array.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'foo'.match(/foo/); RegExp['$&']
    'foo'.match(/foo/);
	testcases[count++] = new TestCase ( SECTION, "'foo'.match(/foo/); RegExp['$&']",
	                                    'foo', RegExp['$&']);

    // 'foo'.match(new RegExp('foo')); RegExp['$&']
    'foo'.match(new RegExp('foo'));
	testcases[count++] = new TestCase ( SECTION, "'foo'.match(new RegExp('foo')); RegExp['$&']",
	                                    'foo', RegExp['$&']);

    // 'xxx'.match(/bar/); RegExp['$&']
    'xxx'.match(/bar/);
	testcases[count++] = new TestCase ( SECTION, "'xxx'.match(/bar/); RegExp['$&']",
	                                    'foo', RegExp['$&']);

    // 'xxx'.match(/$/); RegExp['$&']
    'xxx'.match(/$/);
	testcases[count++] = new TestCase ( SECTION, "'xxx'.match(/$/); RegExp['$&']",
	                                    '', RegExp['$&']);

    // 'abcdefg'.match(/^..(cd)[a-z]+/); RegExp['$&']
    'abcdefg'.match(/^..(cd)[a-z]+/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/^..(cd)[a-z]+/); RegExp['$&']",
	                                    'abcdefg', RegExp['$&']);

    // 'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\1/); RegExp['$&']
    'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\1/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\\1/); RegExp['$&']",
	                                    'abcdefgabcdefg', RegExp['$&']);

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
