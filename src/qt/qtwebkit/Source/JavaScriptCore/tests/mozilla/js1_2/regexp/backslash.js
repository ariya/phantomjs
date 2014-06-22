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
	Filename:     backslash.js
	Description:  'Tests regular expressions containing \'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: \\';

	writeHeaderToLog('Executing script: backslash.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcde'.match(new RegExp('\e'))
	testcases[count++] = new TestCase ( SECTION, "'abcde'.match(new RegExp('\e'))",
	                                    String(["e"]), String('abcde'.match(new RegExp('\e'))));

    // 'ab\\cde'.match(new RegExp('\\\\'))
	testcases[count++] = new TestCase ( SECTION, "'ab\\cde'.match(new RegExp('\\\\'))",
	                                    String(["\\"]), String('ab\\cde'.match(new RegExp('\\\\'))));

    // 'ab\\cde'.match(/\\/) (using literal)
	testcases[count++] = new TestCase ( SECTION, "'ab\\cde'.match(/\\\\/)",
	                                    String(["\\"]), String('ab\\cde'.match(/\\/)));

    // 'before ^$*+?.()|{}[] after'.match(new RegExp('\^\$\*\+\?\.\(\)\|\{\}\[\]'))
	testcases[count++] = new TestCase ( SECTION,
	                        "'before ^$*+?.()|{}[] after'.match(new RegExp('\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]'))",
	                        String(["^$*+?.()|{}[]"]),
	                        String('before ^$*+?.()|{}[] after'.match(new RegExp('\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]'))));

    // 'before ^$*+?.()|{}[] after'.match(/\^\$\*\+\?\.\(\)\|\{\}\[\]/) (using literal)
	testcases[count++] = new TestCase ( SECTION,
	                        "'before ^$*+?.()|{}[] after'.match(/\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]/)",
	                        String(["^$*+?.()|{}[]"]),
	                        String('before ^$*+?.()|{}[] after'.match(/\^\$\*\+\?\.\(\)\|\{\}\[\]/)));

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
