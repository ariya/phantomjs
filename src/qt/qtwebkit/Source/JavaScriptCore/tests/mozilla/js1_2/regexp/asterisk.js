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
	Filename:     asterisk.js
	Description:  'Tests regular expressions containing *'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: *';

	writeHeaderToLog('Executing script: aterisk.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcddddefg'.match(new RegExp('d*'))
	testcases[count++] = new TestCase ( SECTION, "'abcddddefg'.match(new RegExp('d*'))",
	                                    String([""]), String('abcddddefg'.match(new RegExp('d*'))));

    // 'abcddddefg'.match(new RegExp('cd*'))
	testcases[count++] = new TestCase ( SECTION, "'abcddddefg'.match(new RegExp('cd*'))",
	                                    String(["cdddd"]), String('abcddddefg'.match(new RegExp('cd*'))));

    // 'abcdefg'.match(new RegExp('cx*d'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('cx*d'))",
	                                    String(["cd"]), String('abcdefg'.match(new RegExp('cx*d'))));

    // 'xxxxxxx'.match(new RegExp('(x*)(x+)'))
	testcases[count++] = new TestCase ( SECTION, "'xxxxxxx'.match(new RegExp('(x*)(x+)'))",
	                                    String(["xxxxxxx","xxxxxx","x"]), String('xxxxxxx'.match(new RegExp('(x*)(x+)'))));

    // '1234567890'.match(new RegExp('(\\d*)(\\d+)'))
	testcases[count++] = new TestCase ( SECTION, "'1234567890'.match(new RegExp('(\\d*)(\\d+)'))",
	                                    String(["1234567890","123456789","0"]),
	                                    String('1234567890'.match(new RegExp('(\\d*)(\\d+)'))));

    // '1234567890'.match(new RegExp('(\\d*)\\d(\\d+)'))
	testcases[count++] = new TestCase ( SECTION, "'1234567890'.match(new RegExp('(\\d*)\\d(\\d+)'))",
	                                    String(["1234567890","12345678","0"]),
	                                    String('1234567890'.match(new RegExp('(\\d*)\\d(\\d+)'))));

    // 'xxxxxxx'.match(new RegExp('(x+)(x*)'))
	testcases[count++] = new TestCase ( SECTION, "'xxxxxxx'.match(new RegExp('(x+)(x*)'))",
	                                    String(["xxxxxxx","xxxxxxx",""]), String('xxxxxxx'.match(new RegExp('(x+)(x*)'))));

    // 'xxxxxxyyyyyy'.match(new RegExp('x*y+$'))
	testcases[count++] = new TestCase ( SECTION, "'xxxxxxyyyyyy'.match(new RegExp('x*y+$'))",
	                                    String(["xxxxxxyyyyyy"]), String('xxxxxxyyyyyy'.match(new RegExp('x*y+$'))));

    // 'abcdef'.match(/[\d]*[\s]*bc./)
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/[\\d]*[\\s]*bc./)",
	                                    String(["bcd"]), String('abcdef'.match(/[\d]*[\s]*bc./)));

    // 'abcdef'.match(/bc..[\d]*[\s]*/)
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/bc..[\\d]*[\\s]*/)",
	                                    String(["bcde"]), String('abcdef'.match(/bc..[\d]*[\s]*/)));

    // 'a1b2c3'.match(/.*/)
	testcases[count++] = new TestCase ( SECTION, "'a1b2c3'.match(/.*/)",
	                                    String(["a1b2c3"]), String('a1b2c3'.match(/.*/)));

    // 'a0.b2.c3'.match(/[xyz]*1/)
	testcases[count++] = new TestCase ( SECTION, "'a0.b2.c3'.match(/[xyz]*1/)",
	                                    null, 'a0.b2.c3'.match(/[xyz]*1/));

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
