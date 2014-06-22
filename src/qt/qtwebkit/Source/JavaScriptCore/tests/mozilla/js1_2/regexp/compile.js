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
	Filename:     compile.js
	Description:  'Tests regular expressions method compile'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: compile';

	writeHeaderToLog('Executing script: compile.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    var regularExpression = new RegExp();

    regularExpression.compile("[0-9]{3}x[0-9]{4}","i");

	testcases[count++] = new TestCase ( SECTION,
	                                    "(compile '[0-9]{3}x[0-9]{4}','i')",
	                                    String(["456X7890"]), String('234X456X7890'.match(regularExpression)));

	testcases[count++] = new TestCase ( SECTION,
	                                    "source of (compile '[0-9]{3}x[0-9]{4}','i')",
	                                    "[0-9]{3}x[0-9]{4}", regularExpression.source);

	testcases[count++] = new TestCase ( SECTION,
	                                    "global of (compile '[0-9]{3}x[0-9]{4}','i')",
	                                    false, regularExpression.global);

	testcases[count++] = new TestCase ( SECTION,
	                                    "ignoreCase of (compile '[0-9]{3}x[0-9]{4}','i')",
	                                    true, regularExpression.ignoreCase);

    regularExpression.compile("[0-9]{3}X[0-9]{3}","g");

	testcases[count++] = new TestCase ( SECTION,
	                                    "(compile '[0-9]{3}X[0-9]{3}','g')",
	                                    String(["234X456"]), String('234X456X7890'.match(regularExpression)));

	testcases[count++] = new TestCase ( SECTION,
	                                    "source of (compile '[0-9]{3}X[0-9]{3}','g')",
	                                    "[0-9]{3}X[0-9]{3}", regularExpression.source);

	testcases[count++] = new TestCase ( SECTION,
	                                    "global of (compile '[0-9]{3}X[0-9]{3}','g')",
	                                    true, regularExpression.global);

	testcases[count++] = new TestCase ( SECTION,
	                                    "ignoreCase of (compile '[0-9]{3}X[0-9]{3}','g')",
	                                    false, regularExpression.ignoreCase);


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
