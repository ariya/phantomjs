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
	Filename:     RegExp_lastParen.js
	Description:  'Tests RegExps lastParen property'

	Author:       Nick Lerissa
	Date:         March 12, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: lastParen';

	writeHeaderToLog('Executing script: RegExp_lastParen.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcd'.match(/(abc)d/); RegExp.lastParen
    'abcd'.match(/(abc)d/);
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(/(abc)d/); RegExp.lastParen",
	                                    'abc', RegExp.lastParen);

    // 'abcd'.match(new RegExp('(abc)d')); RegExp.lastParen
    'abcd'.match(new RegExp('(abc)d'));
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(new RegExp('(abc)d')); RegExp.lastParen",
	                                    'abc', RegExp.lastParen);

    // 'abcd'.match(/(bcd)e/); RegExp.lastParen
    'abcd'.match(/(bcd)e/);
	testcases[count++] = new TestCase ( SECTION, "'abcd'.match(/(bcd)e/); RegExp.lastParen",
	                                    'abc', RegExp.lastParen);

    // 'abcdefg'.match(/(a(b(c(d)e)f)g)/); RegExp.lastParen
    'abcdefg'.match(/(a(b(c(d)e)f)g)/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/(a(b(c(d)e)f)g)/); RegExp.lastParen",
	                                    'd', RegExp.lastParen);

    // 'abcdefg'.match(/(a(b)c)(d(e)f)/); RegExp.lastParen
    'abcdefg'.match(/(a(b)c)(d(e)f)/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/(a(b)c)(d(e)f)/); RegExp.lastParen",
	                                    'e', RegExp.lastParen);

    // 'abcdefg'.match(/(^)abc/); RegExp.lastParen
    'abcdefg'.match(/(^)abc/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/(^)abc/); RegExp.lastParen",
	                                    '', RegExp.lastParen);

    // 'abcdefg'.match(/(^a)bc/); RegExp.lastParen
    'abcdefg'.match(/(^a)bc/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/(^a)bc/); RegExp.lastParen",
	                                    'a', RegExp.lastParen);

    // 'abcdefg'.match(new RegExp('(^a)bc')); RegExp.lastParen
    'abcdefg'.match(new RegExp('(^a)bc'));
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(new RegExp('(^a)bc')); RegExp.lastParen",
	                                    'a', RegExp.lastParen);

    // 'abcdefg'.match(/bc/); RegExp.lastParen
    'abcdefg'.match(/bc/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefg'.match(/bc/); RegExp.lastParen",
	                                    '', RegExp.lastParen);

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
