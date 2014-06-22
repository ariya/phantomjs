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
	Filename:     whitespace.js
	Description:  'Tests regular expressions containing \f\n\r\t\v\s\S\ '

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: \\f\\n\\r\\t\\v\\s\\S ';

	writeHeaderToLog('Executing script: whitespace.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	var non_whitespace = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`!@#$%^&*()-+={[}]|\\:;'<,>./?1234567890" + '"';
    var whitespace     = "\f\n\r\t\v ";

    // be sure all whitespace is matched by \s
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + whitespace + "'.match(new RegExp('\\s+'))",
	                                    String([whitespace]), String(whitespace.match(new RegExp('\\s+'))));

    // be sure all non-whitespace is matched by \S
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + non_whitespace + "'.match(new RegExp('\\S+'))",
	                                    String([non_whitespace]), String(non_whitespace.match(new RegExp('\\S+'))));

    // be sure all non-whitespace is not matched by \s
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + non_whitespace + "'.match(new RegExp('\\s'))",
	                                    null, non_whitespace.match(new RegExp('\\s')));

    // be sure all whitespace is not matched by \S
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + whitespace + "'.match(new RegExp('\\S'))",
	                                    null, whitespace.match(new RegExp('\\S')));

	var s = non_whitespace + whitespace;

    // be sure all digits are matched by \s
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\s+'))",
	                                    String([whitespace]), String(s.match(new RegExp('\\s+'))));

	s = whitespace + non_whitespace;

    // be sure all non-whitespace are matched by \S
	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\S+'))",
	                                    String([non_whitespace]), String(s.match(new RegExp('\\S+'))));

    // '1233345find me345'.match(new RegExp('[a-z\\s][a-z\\s]+'))
	testcases[count++] = new TestCase ( SECTION, "'1233345find me345'.match(new RegExp('[a-z\\s][a-z\\s]+'))",
	                                    String(["find me"]), String('1233345find me345'.match(new RegExp('[a-z\\s][a-z\\s]+'))));

	var i;

    // be sure all whitespace characters match individually
	for (i = 0; i < whitespace.length; ++i)
	{
	    s = 'ab' + whitespace[i] + 'cd';
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\\\s'))",
    	                                    String([whitespace[i]]), String(s.match(new RegExp('\\s'))));
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(/\s/)",
    	                                    String([whitespace[i]]), String(s.match(/\s/)));
	}
    // be sure all non_whitespace characters match individually
	for (i = 0; i < non_whitespace.length; ++i)
	{
	    s = '  ' + non_whitespace[i] + '  ';
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\\\S'))",
    	                                    String([non_whitespace[i]]), String(s.match(new RegExp('\\S'))));
    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(/\S/)",
    	                                    String([non_whitespace[i]]), String(s.match(/\S/)));
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
