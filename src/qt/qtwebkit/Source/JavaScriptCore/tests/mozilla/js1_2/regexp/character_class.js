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
	Filename:     character_class.js
	Description:  'Tests regular expressions containing []'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: []';

	writeHeaderToLog('Executing script: character_class.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcde'.match(new RegExp('ab[ercst]de'))
	testcases[count++] = new TestCase ( SECTION, "'abcde'.match(new RegExp('ab[ercst]de'))",
	                                    String(["abcde"]), String('abcde'.match(new RegExp('ab[ercst]de'))));

    // 'abcde'.match(new RegExp('ab[erst]de'))
	testcases[count++] = new TestCase ( SECTION, "'abcde'.match(new RegExp('ab[erst]de'))",
	                                    null, 'abcde'.match(new RegExp('ab[erst]de')));

    // 'abcdefghijkl'.match(new RegExp('[d-h]+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefghijkl'.match(new RegExp('[d-h]+'))",
	                                    String(["defgh"]), String('abcdefghijkl'.match(new RegExp('[d-h]+'))));

    // 'abc6defghijkl'.match(new RegExp('[1234567].{2}'))
	testcases[count++] = new TestCase ( SECTION, "'abc6defghijkl'.match(new RegExp('[1234567].{2}'))",
	                                    String(["6de"]), String('abc6defghijkl'.match(new RegExp('[1234567].{2}'))));

    // '\n\n\abc324234\n'.match(new RegExp('[a-c\d]+'))
	testcases[count++] = new TestCase ( SECTION, "'\n\n\abc324234\n'.match(new RegExp('[a-c\\d]+'))",
	                                    String(["abc324234"]), String('\n\n\abc324234\n'.match(new RegExp('[a-c\\d]+'))));

    // 'abc'.match(new RegExp('ab[.]?c'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('ab[.]?c'))",
	                                    String(["abc"]), String('abc'.match(new RegExp('ab[.]?c'))));

    // 'abc'.match(new RegExp('a[b]c'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('a[b]c'))",
	                                    String(["abc"]), String('abc'.match(new RegExp('a[b]c'))));

    // 'a1b  b2c  c3d  def  f4g'.match(new RegExp('[a-z][^1-9][a-z]'))
	testcases[count++] = new TestCase ( SECTION, "'a1b  b2c  c3d  def  f4g'.match(new RegExp('[a-z][^1-9][a-z]'))",
	                                    String(["def"]), String('a1b  b2c  c3d  def  f4g'.match(new RegExp('[a-z][^1-9][a-z]'))));

    // '123*&$abc'.match(new RegExp('[*&$]{3}'))
	testcases[count++] = new TestCase ( SECTION, "'123*&$abc'.match(new RegExp('[*&$]{3}'))",
	                                    String(["*&$"]), String('123*&$abc'.match(new RegExp('[*&$]{3}'))));

    // 'abc'.match(new RegExp('a[^1-9]c'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('a[^1-9]c'))",
	                                    String(["abc"]), String('abc'.match(new RegExp('a[^1-9]c'))));

    // 'abc'.match(new RegExp('a[^b]c'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('a[^b]c'))",
	                                    null, 'abc'.match(new RegExp('a[^b]c')));

    // 'abc#$%def%&*@ghi)(*&'.match(new RegExp('[^a-z]{4}'))
	testcases[count++] = new TestCase ( SECTION, "'abc#$%def%&*@ghi)(*&'.match(new RegExp('[^a-z]{4}'))",
	                                    String(["%&*@"]), String('abc#$%def%&*@ghi)(*&'.match(new RegExp('[^a-z]{4}'))));

    // 'abc#$%def%&*@ghi)(*&'.match(/[^a-z]{4}/)
	testcases[count++] = new TestCase ( SECTION, "'abc#$%def%&*@ghi)(*&'.match(/[^a-z]{4}/)",
	                                    String(["%&*@"]), String('abc#$%def%&*@ghi)(*&'.match(/[^a-z]{4}/)));

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
