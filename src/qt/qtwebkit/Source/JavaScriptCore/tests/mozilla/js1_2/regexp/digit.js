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
	Filename:     digit.js
	Description:  'Tests regular expressions containing \d'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: \\d';

	writeHeaderToLog('Executing script: digit.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	var non_digits = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\f\n\r\t\v~`!@#$%^&*()-+={[}]|\\:;'<,>./? " + '"';

	var digits = "1234567890";

    // be sure all digits are matched by \d
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + digits + "'.match(new RegExp('\\d+'))",
	                                    String([digits]), String(digits.match(new RegExp('\\d+'))));

    // be sure all non-digits are matched by \D
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + non_digits + "'.match(new RegExp('\\D+'))",
	                                    String([non_digits]), String(non_digits.match(new RegExp('\\D+'))));

    // be sure all non-digits are not matched by \d
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + non_digits + "'.match(new RegExp('\\d'))",
	                                    null, non_digits.match(new RegExp('\\d')));

    // be sure all digits are not matched by \D
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + digits + "'.match(new RegExp('\\D'))",
	                                    null, digits.match(new RegExp('\\D')));

	var s = non_digits + digits;

    // be sure all digits are matched by \d
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\d+'))",
	                                    String([digits]), String(s.match(new RegExp('\\d+'))));

	var s = digits + non_digits;

    // be sure all non-digits are matched by \D
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\D+'))",
	                                    String([non_digits]), String(s.match(new RegExp('\\D+'))));

	var i;

    // be sure all digits match individually
	for (i = 0; i < digits.length; ++i)
	{
	    s = 'ab' + digits[i] + 'cd';
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\d'))",
    	                                    String([digits[i]]), String(s.match(new RegExp('\\d'))));
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(/\\d/)",
    	                                    String([digits[i]]), String(s.match(/\d/)));
	}
    // be sure all non_digits match individually
	for (i = 0; i < non_digits.length; ++i)
	{
	    s = '12' + non_digits[i] + '34';
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\D'))",
    	                                    String([non_digits[i]]), String(s.match(new RegExp('\\D'))));
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(/\\D/)",
    	                                    String([non_digits[i]]), String(s.match(/\D/)));
	}


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
