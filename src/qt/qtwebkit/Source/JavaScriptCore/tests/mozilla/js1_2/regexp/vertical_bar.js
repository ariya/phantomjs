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
	Filename:     vertical_bar.js
	Description:  'Tests regular expressions containing |'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: |';

	writeHeaderToLog('Executing script: vertical_bar.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abc'.match(new RegExp('xyz|abc'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('xyz|abc'))",
	                                    String(["abc"]), String('abc'.match(new RegExp('xyz|abc'))));

    // 'this is a test'.match(new RegExp('quiz|exam|test|homework'))
	testcases[count++] = new TestCase ( SECTION, "'this is a test'.match(new RegExp('quiz|exam|test|homework'))",
	                                    String(["test"]), String('this is a test'.match(new RegExp('quiz|exam|test|homework'))));

    // 'abc'.match(new RegExp('xyz|...'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('xyz|...'))",
	                                    String(["abc"]), String('abc'.match(new RegExp('xyz|...'))));

    // 'abc'.match(new RegExp('(.)..|abc'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('(.)..|abc'))",
	                                    String(["abc","a"]), String('abc'.match(new RegExp('(.)..|abc'))));

    // 'color: grey'.match(new RegExp('.+: gr(a|e)y'))
	testcases[count++] = new TestCase ( SECTION, "'color: grey'.match(new RegExp('.+: gr(a|e)y'))",
	                                    String(["color: grey","e"]), String('color: grey'.match(new RegExp('.+: gr(a|e)y'))));

    // 'no match'.match(new RegExp('red|white|blue'))
	testcases[count++] = new TestCase ( SECTION, "'no match'.match(new RegExp('red|white|blue'))",
	                                    null, 'no match'.match(new RegExp('red|white|blue')));

    // 'Hi Bob'.match(new RegExp('(Rob)|(Bob)|(Robert)|(Bobby)'))
	testcases[count++] = new TestCase ( SECTION, "'Hi Bob'.match(new RegExp('(Rob)|(Bob)|(Robert)|(Bobby)'))",
	                                    String(["Bob",undefined,"Bob", undefined, undefined]), String('Hi Bob'.match(new RegExp('(Rob)|(Bob)|(Robert)|(Bobby)'))));

    // 'abcdef'.match(new RegExp('abc|bcd|cde|def'))
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(new RegExp('abc|bcd|cde|def'))",
	                                    String(["abc"]), String('abcdef'.match(new RegExp('abc|bcd|cde|def'))));

    // 'Hi Bob'.match(/(Rob)|(Bob)|(Robert)|(Bobby)/)
	testcases[count++] = new TestCase ( SECTION, "'Hi Bob'.match(/(Rob)|(Bob)|(Robert)|(Bobby)/)",
	                                    String(["Bob",undefined,"Bob", undefined, undefined]), String('Hi Bob'.match(/(Rob)|(Bob)|(Robert)|(Bobby)/)));

    // 'abcdef'.match(/abc|bcd|cde|def/)
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/abc|bcd|cde|def/)",
	                                    String(["abc"]), String('abcdef'.match(/abc|bcd|cde|def/)));

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
