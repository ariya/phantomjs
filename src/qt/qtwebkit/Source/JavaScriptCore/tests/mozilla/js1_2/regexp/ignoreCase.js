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
	Filename:     ignoreCase.js
	Description:  'Tests RegExp attribute ignoreCase'

	Author:       Nick Lerissa
	Date:         March 13, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: ignoreCase';

	writeHeaderToLog('Executing script: ignoreCase.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // /xyz/i.ignoreCase
	testcases[count++] = new TestCase ( SECTION, "/xyz/i.ignoreCase",
	                                    true, /xyz/i.ignoreCase);

    // /xyz/.ignoreCase
	testcases[count++] = new TestCase ( SECTION, "/xyz/.ignoreCase",
	                                    false, /xyz/.ignoreCase);

    // 'ABC def ghi'.match(/[a-z]+/ig)
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/ig)",
	                                    String(["ABC","def","ghi"]), String('ABC def ghi'.match(/[a-z]+/ig)));

    // 'ABC def ghi'.match(/[a-z]+/i)
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/i)",
	                                    String(["ABC"]), String('ABC def ghi'.match(/[a-z]+/i)));

    // 'ABC def ghi'.match(/([a-z]+)/ig)
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(/([a-z]+)/ig)",
	                                    String(["ABC","def","ghi"]), String('ABC def ghi'.match(/([a-z]+)/ig)));

    // 'ABC def ghi'.match(/([a-z]+)/i)
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(/([a-z]+)/i)",
	                                    String(["ABC","ABC"]), String('ABC def ghi'.match(/([a-z]+)/i)));

    // 'ABC def ghi'.match(/[a-z]+/)
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(/[a-z]+/)",
	                                    String(["def"]), String('ABC def ghi'.match(/[a-z]+/)));

    // (new RegExp('xyz','i')).ignoreCase
	testcases[count++] = new TestCase ( SECTION, "(new RegExp('xyz','i')).ignoreCase",
	                                    true, (new RegExp('xyz','i')).ignoreCase);

    // (new RegExp('xyz')).ignoreCase
	testcases[count++] = new TestCase ( SECTION, "(new RegExp('xyz')).ignoreCase",
	                                    false, (new RegExp('xyz')).ignoreCase);

    // 'ABC def ghi'.match(new RegExp('[a-z]+','ig'))
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+','ig'))",
	                                    String(["ABC","def","ghi"]), String('ABC def ghi'.match(new RegExp('[a-z]+','ig'))));

    // 'ABC def ghi'.match(new RegExp('[a-z]+','i'))
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+','i'))",
	                                    String(["ABC"]), String('ABC def ghi'.match(new RegExp('[a-z]+','i'))));

    // 'ABC def ghi'.match(new RegExp('([a-z]+)','ig'))
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('([a-z]+)','ig'))",
	                                    String(["ABC","def","ghi"]), String('ABC def ghi'.match(new RegExp('([a-z]+)','ig'))));

    // 'ABC def ghi'.match(new RegExp('([a-z]+)','i'))
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('([a-z]+)','i'))",
	                                    String(["ABC","ABC"]), String('ABC def ghi'.match(new RegExp('([a-z]+)','i'))));

    // 'ABC def ghi'.match(new RegExp('[a-z]+'))
	testcases[count++] = new TestCase ( SECTION, "'ABC def ghi'.match(new RegExp('[a-z]+'))",
	                                    String(["def"]), String('ABC def ghi'.match(new RegExp('[a-z]+'))));

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
