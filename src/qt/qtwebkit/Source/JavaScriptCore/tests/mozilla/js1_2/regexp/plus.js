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
	Filename:     plus.js
	Description:  'Tests regular expressions containing +'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: +';

	writeHeaderToLog('Executing script: plus.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcdddddefg'.match(new RegExp('d+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdddddefg'.match(new RegExp('d+'))",
	                                    String(["ddddd"]), String('abcdddddefg'.match(new RegExp('d+'))));

    // 'abcdefg'.match(new RegExp('o+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('o+'))",
	                                    null, 'abcdefg'.match(new RegExp('o+')));

    // 'abcdefg'.match(new RegExp('d+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('d+'))",
	                                    String(['d']), String('abcdefg'.match(new RegExp('d+'))));

    // 'abbbbbbbc'.match(new RegExp('(b+)(b+)(b+)'))
	testcases[count++] = new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('(b+)(b+)(b+)'))",
	                                    String(["bbbbbbb","bbbbb","b","b"]), String('abbbbbbbc'.match(new RegExp('(b+)(b+)(b+)'))));

    // 'abbbbbbbc'.match(new RegExp('(b+)(b*)'))
	testcases[count++] = new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('(b+)(b*)'))",
	                                    String(["bbbbbbb","bbbbbbb",""]), String('abbbbbbbc'.match(new RegExp('(b+)(b*)'))));

    // 'abbbbbbbc'.match(new RegExp('b*b+'))
	testcases[count++] = new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('b*b+'))",
	                                    String(['bbbbbbb']), String('abbbbbbbc'.match(new RegExp('b*b+'))));

    // 'abbbbbbbc'.match(/(b+)(b*)/)
	testcases[count++] = new TestCase ( SECTION, "'abbbbbbbc'.match(/(b+)(b*)/)",
	                                    String(["bbbbbbb","bbbbbbb",""]), String('abbbbbbbc'.match(/(b+)(b*)/)));

    // 'abbbbbbbc'.match(new RegExp('b*b+'))
	testcases[count++] = new TestCase ( SECTION, "'abbbbbbbc'.match(/b*b+/)",
	                                    String(['bbbbbbb']), String('abbbbbbbc'.match(/b*b+/)));

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
