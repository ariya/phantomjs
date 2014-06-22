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
	Filename:     RegExp_multiline.js
	Description:  'Tests RegExps multiline property'

	Author:       Nick Lerissa
	Date:         March 12, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: multiline';

	writeHeaderToLog('Executing script: RegExp_multiline.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // First we do a series of tests with RegExp.multiline set to false (default value)
    // Following this we do the same tests with RegExp.multiline set true(**).
    // RegExp.multiline
	testcases[count++] = new TestCase ( SECTION, "RegExp.multiline",
	                                    false, RegExp.multiline);

    // (multiline == false) '123\n456'.match(/^4../)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) '123\\n456'.match(/^4../)",
	                                    null, '123\n456'.match(/^4../));

    // (multiline == false) 'a11\na22\na23\na24'.match(/^a../g)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(/^a../g)",
	                                    String(['a11']), String('a11\na22\na23\na24'.match(/^a../g)));

    // (multiline == false) 'a11\na22'.match(/^.+^./)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'a11\na22'.match(/^.+^./)",
	                                    null, 'a11\na22'.match(/^.+^./));

    // (multiline == false) '123\n456'.match(/.3$/)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) '123\\n456'.match(/.3$/)",
	                                    null, '123\n456'.match(/.3$/));

    // (multiline == false) 'a11\na22\na23\na24'.match(/a..$/g)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(/a..$/g)",
	                                    String(['a24']), String('a11\na22\na23\na24'.match(/a..$/g)));

    // (multiline == false) 'abc\ndef'.match(/c$...$/)
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'abc\ndef'.match(/c$...$/)",
	                                    null, 'abc\ndef'.match(/c$...$/));

    // (multiline == false) 'a11\na22\na23\na24'.match(new RegExp('a..$','g'))
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','g'))",
	                                    String(['a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','g'))));

    // (multiline == false) 'abc\ndef'.match(new RegExp('c$...$'))
	testcases[count++] = new TestCase ( SECTION, "(multiline == false) 'abc\ndef'.match(new RegExp('c$...$'))",
	                                    null, 'abc\ndef'.match(new RegExp('c$...$')));

    // **Now we do the tests with RegExp.multiline set to true
    // RegExp.multiline = true; RegExp.multiline
    RegExp.multiline = true;
    testcases[count++] = new TestCase ( SECTION, "RegExp.multiline = true; RegExp.multiline",
	                                    true, RegExp.multiline);

    // (multiline == true) '123\n456'.match(/^4../)
    testcases[count++] = new TestCase ( SECTION, "(multiline == true) '123\\n456'.match(/^4../m)",
                                        String(['456']), String('123\n456'.match(/^4../m)));

    // (multiline == true) 'a11\na22\na23\na24'.match(/^a../g)
    testcases[count++] = new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(/^a../gm)",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/^a../gm)));

    // (multiline == true) 'a11\na22'.match(/^.+^./)
	//testcases[count++] = new TestCase ( SECTION, "(multiline == true) 'a11\na22'.match(/^.+^./)",
	//                                    String(['a11\na']), String('a11\na22'.match(/^.+^./)));

    // (multiline == true) '123\n456'.match(/.3$/)
    testcases[count++] = new TestCase ( SECTION, "(multiline == true) '123\\n456'.match(/.3$/m)",
                                        String(['23']), String('123\n456'.match(/.3$/m)));

    // (multiline == true) 'a11\na22\na23\na24'.match(/a..$/g)
    testcases[count++] = new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(/a..$/gm)",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/a..$/gm)));

    // (multiline == true) 'a11\na22\na23\na24'.match(new RegExp('a..$','g'))
    testcases[count++] = new TestCase ( SECTION, "(multiline == true) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','gm'))",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','gm'))));

    // (multiline == true) 'abc\ndef'.match(/c$....$/)
	//testcases[count++] = new TestCase ( SECTION, "(multiline == true) 'abc\ndef'.match(/c$.+$/)",
	//                                    'c\ndef', String('abc\ndef'.match(/c$.+$/)));

	RegExp.multiline = false;

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
