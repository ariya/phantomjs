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
	Filename:     parentheses.js
	Description:  'Tests regular expressions containing ()'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: ()';

	writeHeaderToLog('Executing script: parentheses.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abc'.match(new RegExp('(abc)'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.match(new RegExp('(abc)'))",
	                                    String(["abc","abc"]), String('abc'.match(new RegExp('(abc)'))));

    // 'abcdefg'.match(new RegExp('a(bc)d(ef)g'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('a(bc)d(ef)g'))",
	                                    String(["abcdefg","bc","ef"]), String('abcdefg'.match(new RegExp('a(bc)d(ef)g'))));

    // 'abcdefg'.match(new RegExp('(.{3})(.{4})'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('(.{3})(.{4})'))",
	                                    String(["abcdefg","abc","defg"]), String('abcdefg'.match(new RegExp('(.{3})(.{4})'))));

    // 'aabcdaabcd'.match(new RegExp('(aa)bcd\1'))
	testcases[count++] = new TestCase ( SECTION, "'aabcdaabcd'.match(new RegExp('(aa)bcd\\1'))",
	                                    String(["aabcdaa","aa"]), String('aabcdaabcd'.match(new RegExp('(aa)bcd\\1'))));

    // 'aabcdaabcd'.match(new RegExp('(aa).+\1'))
	testcases[count++] = new TestCase ( SECTION, "'aabcdaabcd'.match(new RegExp('(aa).+\\1'))",
	                                    String(["aabcdaa","aa"]), String('aabcdaabcd'.match(new RegExp('(aa).+\\1'))));

    // 'aabcdaabcd'.match(new RegExp('(.{2}).+\1'))
	testcases[count++] = new TestCase ( SECTION, "'aabcdaabcd'.match(new RegExp('(.{2}).+\\1'))",
	                                    String(["aabcdaa","aa"]), String('aabcdaabcd'.match(new RegExp('(.{2}).+\\1'))));

    // '123456123456'.match(new RegExp('(\d{3})(\d{3})\1\2'))
	testcases[count++] = new TestCase ( SECTION, "'123456123456'.match(new RegExp('(\\d{3})(\\d{3})\\1\\2'))",
	                                    String(["123456123456","123","456"]), String('123456123456'.match(new RegExp('(\\d{3})(\\d{3})\\1\\2'))));

    // 'abcdefg'.match(new RegExp('a(..(..)..)'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('a(..(..)..)'))",
	                                    String(["abcdefg","bcdefg","de"]), String('abcdefg'.match(new RegExp('a(..(..)..)'))));

    // 'abcdefg'.match(/a(..(..)..)/)
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/a(..(..)..)/)",
	                                    String(["abcdefg","bcdefg","de"]), String('abcdefg'.match(/a(..(..)..)/)));

    // 'xabcdefg'.match(new RegExp('(a(b(c)))(d(e(f)))'))
	testcases[count++] = new TestCase ( SECTION, "'xabcdefg'.match(new RegExp('(a(b(c)))(d(e(f)))'))",
	                                    String(["abcdef","abc","bc","c","def","ef","f"]), String('xabcdefg'.match(new RegExp('(a(b(c)))(d(e(f)))'))));

    // 'xabcdefbcefg'.match(new RegExp('(a(b(c)))(d(e(f)))\2\5'))
	testcases[count++] = new TestCase ( SECTION, "'xabcdefbcefg'.match(new RegExp('(a(b(c)))(d(e(f)))\\2\\5'))",
	                                    String(["abcdefbcef","abc","bc","c","def","ef","f"]), String('xabcdefbcefg'.match(new RegExp('(a(b(c)))(d(e(f)))\\2\\5'))));

    // 'abcd'.match(new RegExp('a(.?)b\1c\1d\1'))
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(new RegExp('a(.?)b\\1c\\1d\\1'))",
	                                    String(["abcd",""]), String('abcd'.match(new RegExp('a(.?)b\\1c\\1d\\1'))));

    // 'abcd'.match(/a(.?)b\1c\1d\1/)
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(/a(.?)b\\1c\\1d\\1/)",
	                                    String(["abcd",""]), String('abcd'.match(/a(.?)b\1c\1d\1/)));

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
