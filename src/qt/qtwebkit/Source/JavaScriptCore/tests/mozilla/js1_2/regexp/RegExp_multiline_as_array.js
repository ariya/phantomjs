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
	Filename:     RegExp_multiline_as_array.js
	Description:  'Tests RegExps $* property  (same tests as RegExp_multiline.js but using $*)'

	Author:       Nick Lerissa
	Date:         March 13, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: $*';

	writeHeaderToLog('Executing script: RegExp_multiline_as_array.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // First we do a series of tests with RegExp['$*'] set to false (default value)
    // Following this we do the same tests with RegExp['$*'] set true(**).
    // RegExp['$*']
	testcases[count++] = new TestCase ( SECTION, "RegExp['$*']",
	                                    false, RegExp['$*']);

    // (['$*'] == false) '123\n456'.match(/^4../)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) '123\\n456'.match(/^4../)",
	                                    null, '123\n456'.match(/^4../));

    // (['$*'] == false) 'a11\na22\na23\na24'.match(/^a../g)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'a11\\na22\\na23\\na24'.match(/^a../g)",
	                                    String(['a11']), String('a11\na22\na23\na24'.match(/^a../g)));

    // (['$*'] == false) 'a11\na22'.match(/^.+^./)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'a11\na22'.match(/^.+^./)",
	                                    null, 'a11\na22'.match(/^.+^./));

    // (['$*'] == false) '123\n456'.match(/.3$/)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) '123\\n456'.match(/.3$/)",
	                                    null, '123\n456'.match(/.3$/));

    // (['$*'] == false) 'a11\na22\na23\na24'.match(/a..$/g)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'a11\\na22\\na23\\na24'.match(/a..$/g)",
	                                    String(['a24']), String('a11\na22\na23\na24'.match(/a..$/g)));

    // (['$*'] == false) 'abc\ndef'.match(/c$...$/)
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'abc\ndef'.match(/c$...$/)",
	                                    null, 'abc\ndef'.match(/c$...$/));

    // (['$*'] == false) 'a11\na22\na23\na24'.match(new RegExp('a..$','g'))
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','g'))",
	                                    String(['a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','g'))));

    // (['$*'] == false) 'abc\ndef'.match(new RegExp('c$...$'))
	testcases[count++] = new TestCase ( SECTION, "(['$*'] == false) 'abc\ndef'.match(new RegExp('c$...$'))",
	                                    null, 'abc\ndef'.match(new RegExp('c$...$')));

    // **Now we do the tests with RegExp['$*'] set to true
    // RegExp['$*'] = true; RegExp['$*']
    RegExp['$*'] = true;
	testcases[count++] = new TestCase ( SECTION, "RegExp['$*'] = true; RegExp['$*']",
	                                    true, RegExp['$*']);

    // (['$*'] == true) '123\n456'.match(/^4../)
    testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) '123\\n456'.match(/^4../m)",
                                        String(['456']), String('123\n456'.match(/^4../m)));

    // (['$*'] == true) 'a11\na22\na23\na24'.match(/^a../g)
    testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) 'a11\\na22\\na23\\na24'.match(/^a../gm)",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/^a../gm)));

    // (['$*'] == true) 'a11\na22'.match(/^.+^./)
	//testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) 'a11\na22'.match(/^.+^./)",
	//                                    String(['a11\na']), String('a11\na22'.match(/^.+^./)));

    // (['$*'] == true) '123\n456'.match(/.3$/)
    testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) '123\\n456'.match(/.3$/m)",
                                        String(['23']), String('123\n456'.match(/.3$/m)));

    // (['$*'] == true) 'a11\na22\na23\na24'.match(/a..$/g)
    testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) 'a11\\na22\\na23\\na24'.match(/a..$/gm)",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(/a..$/gm)));

    // (['$*'] == true) 'a11\na22\na23\na24'.match(new RegExp('a..$','g'))
    testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) 'a11\\na22\\na23\\na24'.match(new RegExp('a..$','gm'))",
                                        String(['a11','a22','a23','a24']), String('a11\na22\na23\na24'.match(new RegExp('a..$','gm'))));

    // (['$*'] == true) 'abc\ndef'.match(/c$....$/)
	//testcases[count++] = new TestCase ( SECTION, "(['$*'] == true) 'abc\ndef'.match(/c$.+$/)",
	//                                    'c\ndef', String('abc\ndef'.match(/c$.+$/)));

	RegExp['$*'] = false;

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
