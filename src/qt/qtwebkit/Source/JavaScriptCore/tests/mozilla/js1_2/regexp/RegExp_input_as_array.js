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
	Filename:     RegExp_input_as_array.js
	Description:  'Tests RegExps $_ property  (same tests as RegExp_input.js but using $_)'

	Author:       Nick Lerissa
	Date:         March 13, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: input';

	writeHeaderToLog('Executing script: RegExp_input.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    RegExp['$_'] = "abcd12357efg";

    // RegExp['$_'] = "abcd12357efg"; RegExp['$_']
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; RegExp['$_']",
	                                    "abcd12357efg", RegExp['$_']);

    // RegExp['$_'] = "abcd12357efg"; /\d+/.exec('2345')
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.exec('2345')",
	                                    String(["2345"]), String(/\d+/.exec('2345')));

    // RegExp['$_'] = "abcd12357efg"; /\d+/.exec(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.exec(RegExp.input)",
	                                    String(["12357"]), String(/\d+/.exec(RegExp.input)));

    // RegExp['$_'] = "abcd12357efg"; /[h-z]+/.exec(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /[h-z]+/.exec(RegExp.input)",
	                                    null, /[h-z]+/.exec(RegExp.input));

    // RegExp['$_'] = "abcd12357efg"; /\d+/.test('2345')
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.test('2345')",
	                                    true, /\d+/.test('2345'));

    // RegExp['$_'] = "abcd12357efg"; /\d+/.test(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.test(RegExp.input)",
	                                    true, /\d+/.test(RegExp.input));

    // RegExp['$_'] = "abcd12357efg"; /[h-z]+/.test(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /[h-z]+/.test(RegExp.input)",
	                                    false, /[h-z]+/.test(RegExp.input));

    // RegExp['$_'] = "abcd12357efg"; (new RegExp('\d+')).test(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; (new RegExp('\d+')).test(RegExp.input)",
	                                    true, (new RegExp('\d+')).test(RegExp.input));

    // RegExp['$_'] = "abcd12357efg"; (new RegExp('[h-z]+')).test(RegExp.input)
    RegExp['$_'] = "abcd12357efg";
	testcases[count++] = new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; (new RegExp('[h-z]+')).test(RegExp.input)",
	                                    false, (new RegExp('[h-z]+')).test(RegExp.input));

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
