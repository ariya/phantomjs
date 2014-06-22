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
	Filename:     endLine.js
	Description:  'Tests regular expressions containing $'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: $';

	writeHeaderToLog('Executing script: endLine.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcde'.match(new RegExp('de$'))
	testcases[count++] = new TestCase ( SECTION, "'abcde'.match(new RegExp('de$'))",
	                                    String(["de"]), String('abcde'.match(new RegExp('de$'))));

    // 'ab\ncde'.match(new RegExp('..$e$'))
	testcases[count++] = new TestCase ( SECTION, "'ab\ncde'.match(new RegExp('..$e$'))",
	                                    null, 'ab\ncde'.match(new RegExp('..$e$')));

    // 'yyyyy'.match(new RegExp('xxx$'))
	testcases[count++] = new TestCase ( SECTION, "'yyyyy'.match(new RegExp('xxx$'))",
	                                    null, 'yyyyy'.match(new RegExp('xxx$')));

    // 'a$$$'.match(new RegExp('\\$+$'))
	testcases[count++] = new TestCase ( SECTION, "'a$$$'.match(new RegExp('\\$+$'))",
	                                    String(['$$$']), String('a$$$'.match(new RegExp('\\$+$'))));

    // 'a$$$'.match(/\$+$/)
	testcases[count++] = new TestCase ( SECTION, "'a$$$'.match(/\\$+$/)",
	                                    String(['$$$']), String('a$$$'.match(/\$+$/)));

    RegExp.multiline = true;
    // 'abc\n123xyz890\nxyz'.match(new RegExp('\d+$')) <multiline==true>
    testcases[count++] = new TestCase ( SECTION, "'abc\n123xyz890\nxyz'.match(new RegExp('\\d+$','m'))",
                                        String(['890']), String('abc\n123xyz890\nxyz'.match(new RegExp('\\d+$','m'))));

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
