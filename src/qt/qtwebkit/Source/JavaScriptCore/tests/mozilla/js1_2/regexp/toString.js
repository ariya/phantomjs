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
	Filename:     toString.js
	Description:  'Tests RegExp method toString'

	Author:       Nick Lerissa
	Date:         March 13, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: toString';

	writeHeaderToLog('Executing script: toString.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // var re = new RegExp(); re.toString()
	var re = new RegExp();
	testcases[count++] = new TestCase ( SECTION, "var re = new RegExp(); re.toString()",
	                                    '/(?:)/', re.toString());

    // re = /.+/; re.toString();
    re = /.+/;
	testcases[count++] = new TestCase ( SECTION, "re = /.+/; re.toString()",
	                                    '/.+/', re.toString());

    // re = /test/gi; re.toString()
    re = /test/gi;
	testcases[count++] = new TestCase ( SECTION, "re = /test/gi; re.toString()",
	                                    '/test/gi', re.toString());

    // re = /test2/ig; re.toString()
    re = /test2/ig;
	testcases[count++] = new TestCase ( SECTION, "re = /test2/ig; re.toString()",
	                                    '/test2/gi', re.toString());

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
