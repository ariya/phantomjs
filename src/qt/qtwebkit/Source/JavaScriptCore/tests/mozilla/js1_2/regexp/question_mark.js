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
	Filename:     question_mark.js
	Description:  'Tests regular expressions containing ?'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: ?';

	writeHeaderToLog('Executing script: question_mark.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcdef'.match(new RegExp('cd?e'))
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(new RegExp('cd?e'))",
	                                    String(["cde"]), String('abcdef'.match(new RegExp('cd?e'))));

    // 'abcdef'.match(new RegExp('cdx?e'))
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(new RegExp('cdx?e'))",
	                                    String(["cde"]), String('abcdef'.match(new RegExp('cdx?e'))));

    // 'pqrstuvw'.match(new RegExp('o?pqrst'))
	testcases[count++] = new TestCase ( SECTION, "'pqrstuvw'.match(new RegExp('o?pqrst'))",
	                                    String(["pqrst"]), String('pqrstuvw'.match(new RegExp('o?pqrst'))));

    // 'abcd'.match(new RegExp('x?y?z?'))
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(new RegExp('x?y?z?'))",
	                                    String([""]), String('abcd'.match(new RegExp('x?y?z?'))));

    // 'abcd'.match(new RegExp('x?ay?bz?c'))
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(new RegExp('x?ay?bz?c'))",
	                                    String(["abc"]), String('abcd'.match(new RegExp('x?ay?bz?c'))));

    // 'abcd'.match(/x?ay?bz?c/)
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(/x?ay?bz?c/)",
	                                    String(["abc"]), String('abcd'.match(/x?ay?bz?c/)));

    // 'abbbbc'.match(new RegExp('b?b?b?b'))
	testcases[count++] = new TestCase ( SECTION, "'abbbbc'.match(new RegExp('b?b?b?b'))",
	                                    String(["bbbb"]), String('abbbbc'.match(new RegExp('b?b?b?b'))));

    // '123az789'.match(new RegExp('ab?c?d?x?y?z'))
	testcases[count++] = new TestCase ( SECTION, "'123az789'.match(new RegExp('ab?c?d?x?y?z'))",
	                                    String(["az"]), String('123az789'.match(new RegExp('ab?c?d?x?y?z'))));

    // '123az789'.match(/ab?c?d?x?y?z/)
	testcases[count++] = new TestCase ( SECTION, "'123az789'.match(/ab?c?d?x?y?z/)",
	                                    String(["az"]), String('123az789'.match(/ab?c?d?x?y?z/)));

    // '?????'.match(new RegExp('\\??\\??\\??\\??\\??'))
	testcases[count++] = new TestCase ( SECTION, "'?????'.match(new RegExp('\\??\\??\\??\\??\\??'))",
	                                    String(["?????"]), String('?????'.match(new RegExp('\\??\\??\\??\\??\\??'))));

    // 'test'.match(new RegExp('.?.?.?.?.?.?.?'))
	testcases[count++] = new TestCase ( SECTION, "'test'.match(new RegExp('.?.?.?.?.?.?.?'))",
	                                    String(["test"]), String('test'.match(new RegExp('.?.?.?.?.?.?.?'))));

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
