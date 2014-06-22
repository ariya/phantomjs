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
	Filename:     dot.js
	Description:  'Tests regular expressions containing .'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: .';

	writeHeaderToLog('Executing script: dot.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcde'.match(new RegExp('ab.de'))
	testcases[count++] = new TestCase ( SECTION, "'abcde'.match(new RegExp('ab.de'))",
	                                    String(["abcde"]), String('abcde'.match(new RegExp('ab.de'))));

    // 'line 1\nline 2'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'line 1\nline 2'.match(new RegExp('.+'))",
	                                    String(["line 1"]), String('line 1\nline 2'.match(new RegExp('.+'))));

    // 'this is a test'.match(new RegExp('.*a.*'))
	testcases[count++] = new TestCase ( SECTION, "'this is a test'.match(new RegExp('.*a.*'))",
	                                    String(["this is a test"]), String('this is a test'.match(new RegExp('.*a.*'))));

    // 'this is a *&^%$# test'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'this is a *&^%$# test'.match(new RegExp('.+'))",
	                                    String(["this is a *&^%$# test"]), String('this is a *&^%$# test'.match(new RegExp('.+'))));

    // '....'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'....'.match(new RegExp('.+'))",
	                                    String(["...."]), String('....'.match(new RegExp('.+'))));

    // 'abcdefghijklmnopqrstuvwxyz'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdefghijklmnopqrstuvwxyz'.match(new RegExp('.+'))",
	                                    String(["abcdefghijklmnopqrstuvwxyz"]), String('abcdefghijklmnopqrstuvwxyz'.match(new RegExp('.+'))));

    // 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.match(new RegExp('.+'))",
	                                    String(["ABCDEFGHIJKLMNOPQRSTUVWXYZ"]), String('ABCDEFGHIJKLMNOPQRSTUVWXYZ'.match(new RegExp('.+'))));

    // '`1234567890-=~!@#$%^&*()_+'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'`1234567890-=~!@#$%^&*()_+'.match(new RegExp('.+'))",
	                                    String(["`1234567890-=~!@#$%^&*()_+"]), String('`1234567890-=~!@#$%^&*()_+'.match(new RegExp('.+'))));

    // '|\\[{]};:"\',<>.?/'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'|\\[{]};:\"\',<>.?/'.match(new RegExp('.+'))",
	                                    String(["|\\[{]};:\"\',<>.?/"]), String('|\\[{]};:\"\',<>.?/'.match(new RegExp('.+'))));

    // '|\\[{]};:"\',<>.?/'.match(/.+/)
	testcases[count++] = new TestCase ( SECTION, "'|\\[{]};:\"\',<>.?/'.match(/.+/)",
	                                    String(["|\\[{]};:\"\',<>.?/"]), String('|\\[{]};:\"\',<>.?/'.match(/.+/)));

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
