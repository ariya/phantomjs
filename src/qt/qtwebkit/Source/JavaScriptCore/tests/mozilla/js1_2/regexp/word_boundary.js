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
	Filename:     word_boundary.js
	Description:  'Tests regular expressions containing \b and \B'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: \\b and \\B';

	writeHeaderToLog('Executing script: word_boundary.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'cowboy boyish boy'.match(new RegExp('\bboy\b'))
	testcases[count++] = new TestCase ( SECTION, "'cowboy boyish boy'.match(new RegExp('\\bboy\\b'))",
	                                    String(["boy"]), String('cowboy boyish boy'.match(new RegExp('\\bboy\\b'))));

	var boundary_characters = "\f\n\r\t\v~`!@#$%^&*()-+={[}]|\\:;'<,>./? " + '"';
	var non_boundary_characters = '1234567890_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
	var s     = '';
	var i;

    // testing whether all boundary characters are matched when they should be
	for (i = 0; i < boundary_characters.length; ++i)
	{
	    s = '123ab' + boundary_characters.charAt(i) + '123c' + boundary_characters.charAt(i);

    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\b123[a-z]\\b'))",
    	                                    String(["123c"]), String(s.match(new RegExp('\\b123[a-z]\\b'))));
	}

    // testing whether all non-boundary characters are matched when they should be
	for (i = 0; i < non_boundary_characters.length; ++i)
	{
	    s = '123ab' + non_boundary_characters.charAt(i) + '123c' + non_boundary_characters.charAt(i);

    	testcases[count++] = new TestCase ( SECTION,
    	                                    "'" + s + "'.match(new RegExp('\\B123[a-z]\\B'))",
    	                                    String(["123c"]), String(s.match(new RegExp('\\B123[a-z]\\B'))));
	}

	s = '';

    // testing whether all boundary characters are not matched when they should not be
	for (i = 0; i < boundary_characters.length; ++i)
	{
	    s += boundary_characters[i] + "a" + i + "b";
	}
	s += "xa1111bx";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\Ba\\d+b\\B'))",
	                                    String(["a1111b"]), String(s.match(new RegExp('\\Ba\\d+b\\B'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(/\\Ba\\d+b\\B/)",
	                                    String(["a1111b"]), String(s.match(/\Ba\d+b\B/)));

	s = '';

    // testing whether all non-boundary characters are not matched when they should not be
	for (i = 0; i < non_boundary_characters.length; ++i)
	{
	    s += non_boundary_characters[i] + "a" + i + "b";
	}
	s += "(a1111b)";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(new RegExp('\\ba\\d+b\\b'))",
	                                    String(["a1111b"]), String(s.match(new RegExp('\\ba\\d+b\\b'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + s + "'.match(/\\ba\\d+b\\b/)",
	                                    String(["a1111b"]), String(s.match(/\ba\d+b\b/)));


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
