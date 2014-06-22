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
	Filename:     backspace.js
	Description:  'Tests regular expressions containing [\b]'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: [\b]';

	writeHeaderToLog('Executing script: backspace.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abc\bdef'.match(new RegExp('.[\b].'))
	testcases[count++] = new TestCase ( SECTION, "'abc\bdef'.match(new RegExp('.[\\b].'))",
	                                    String(["c\bd"]), String('abc\bdef'.match(new RegExp('.[\\b].'))));

    // 'abc\\bdef'.match(new RegExp('.[\b].'))
	testcases[count++] = new TestCase ( SECTION, "'abc\\bdef'.match(new RegExp('.[\\b].'))",
	                                    null, 'abc\\bdef'.match(new RegExp('.[\\b].')));

    // 'abc\b\b\bdef'.match(new RegExp('c[\b]{3}d'))
	testcases[count++] = new TestCase ( SECTION, "'abc\b\b\bdef'.match(new RegExp('c[\\b]{3}d'))",
	                                    String(["c\b\b\bd"]), String('abc\b\b\bdef'.match(new RegExp('c[\\b]{3}d'))));

    // 'abc\bdef'.match(new RegExp('[^\\[\b\\]]+'))
	testcases[count++] = new TestCase ( SECTION, "'abc\bdef'.match(new RegExp('[^\\[\\b\\]]+'))",
	                                    String(["abc"]), String('abc\bdef'.match(new RegExp('[^\\[\\b\\]]+'))));

    // 'abcdef'.match(new RegExp('[^\\[\b\\]]+'))
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(new RegExp('[^\\[\\b\\]]+'))",
	                                    String(["abcdef"]), String('abcdef'.match(new RegExp('[^\\[\\b\\]]+'))));

    // 'abcdef'.match(/[^\[\b\]]+/)
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/[^\\[\\b\\]]+/)",
	                                    String(["abcdef"]), String('abcdef'.match(/[^\[\b\]]+/)));

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
