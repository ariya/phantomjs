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
	Filename:     interval.js
	Description:  'Tests regular expressions containing {}'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: {}';

	writeHeaderToLog('Executing script: interval.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{2}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{2}c'))",
	                                    String(["bbc"]), String('aaabbbbcccddeeeefffff'.match(new RegExp('b{2}c'))));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{8}'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{8}'))",
	                                    null, 'aaabbbbcccddeeeefffff'.match(new RegExp('b{8}')));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{2,}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{2,}c'))",
	                                    String(["bbbbc"]), String('aaabbbbcccddeeeefffff'.match(new RegExp('b{2,}c'))));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{8,}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{8,}c'))",
	                                    null, 'aaabbbbcccddeeeefffff'.match(new RegExp('b{8,}c')));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{2,3}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{2,3}c'))",
	                                    String(["bbbc"]), String('aaabbbbcccddeeeefffff'.match(new RegExp('b{2,3}c'))));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{42,93}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{42,93}c'))",
	                                    null, 'aaabbbbcccddeeeefffff'.match(new RegExp('b{42,93}c')));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('b{0,93}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('b{0,93}c'))",
	                                    String(["bbbbc"]), String('aaabbbbcccddeeeefffff'.match(new RegExp('b{0,93}c'))));

    // 'aaabbbbcccddeeeefffff'.match(new RegExp('bx{0,93}c'))
	testcases[count++] = new TestCase ( SECTION, "'aaabbbbcccddeeeefffff'.match(new RegExp('bx{0,93}c'))",
	                                    String(["bc"]), String('aaabbbbcccddeeeefffff'.match(new RegExp('bx{0,93}c'))));

    // 'weirwerdf'.match(new RegExp('.{0,93}'))
	testcases[count++] = new TestCase ( SECTION, "'weirwerdf'.match(new RegExp('.{0,93}'))",
	                                    String(["weirwerdf"]), String('weirwerdf'.match(new RegExp('.{0,93}'))));

    // 'wqe456646dsff'.match(new RegExp('\d{1,}'))
	testcases[count++] = new TestCase ( SECTION, "'wqe456646dsff'.match(new RegExp('\\d{1,}'))",
	                                    String(["456646"]), String('wqe456646dsff'.match(new RegExp('\\d{1,}'))));

    // '123123'.match(new RegExp('(123){1,}'))
	testcases[count++] = new TestCase ( SECTION, "'123123'.match(new RegExp('(123){1,}'))",
	                                    String(["123123","123"]), String('123123'.match(new RegExp('(123){1,}'))));

    // '123123x123'.match(new RegExp('(123){1,}x\1'))
	testcases[count++] = new TestCase ( SECTION, "'123123x123'.match(new RegExp('(123){1,}x\\1'))",
	                                    String(["123123x123","123"]), String('123123x123'.match(new RegExp('(123){1,}x\\1'))));

    // '123123x123'.match(/(123){1,}x\1/)
	testcases[count++] = new TestCase ( SECTION, "'123123x123'.match(/(123){1,}x\\1/)",
	                                    String(["123123x123","123"]), String('123123x123'.match(/(123){1,}x\1/)));

    // 'xxxxxxx'.match(new RegExp('x{1,2}x{1,}'))
	testcases[count++] = new TestCase ( SECTION, "'xxxxxxx'.match(new RegExp('x{1,2}x{1,}'))",
	                                    String(["xxxxxxx"]), String('xxxxxxx'.match(new RegExp('x{1,2}x{1,}'))));

    // 'xxxxxxx'.match(/x{1,2}x{1,}/)
	testcases[count++] = new TestCase ( SECTION, "'xxxxxxx'.match(/x{1,2}x{1,}/)",
	                                    String(["xxxxxxx"]), String('xxxxxxx'.match(/x{1,2}x{1,}/)));

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
