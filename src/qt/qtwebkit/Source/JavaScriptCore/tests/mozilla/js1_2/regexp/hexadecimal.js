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
	Filename:     hexadecimal.js
	Description:  'Tests regular expressions containing \<number> '

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: \\x# (hex) ';

	writeHeaderToLog('Executing script: hexadecimal.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	var testPattern = '\\x41\\x42\\x43\\x44\\x45\\x46\\x47\\x48\\x49\\x4A\\x4B\\x4C\\x4D\\x4E\\x4F\\x50\\x51\\x52\\x53\\x54\\x55\\x56\\x57\\x58\\x59\\x5A';

	var testString = "12345ABCDEFGHIJKLMNOPQRSTUVWXYZ67890";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["ABCDEFGHIJKLMNOPQRSTUVWXYZ"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\x61\\x62\\x63\\x64\\x65\\x66\\x67\\x68\\x69\\x6A\\x6B\\x6C\\x6D\\x6E\\x6F\\x70\\x71\\x72\\x73\\x74\\x75\\x76\\x77\\x78\\x79\\x7A';

	testString = "12345AabcdefghijklmnopqrstuvwxyzZ67890";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["abcdefghijklmnopqrstuvwxyz"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\x20\\x21\\x22\\x23\\x24\\x25\\x26\\x27\\x28\\x29\\x2A\\x2B\\x2C\\x2D\\x2E\\x2F\\x30\\x31\\x32\\x33';

	testString = "abc !\"#$%&'()*+,-./0123ZBC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String([" !\"#$%&'()*+,-./0123"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\x34\\x35\\x36\\x37\\x38\\x39\\x3A\\x3B\\x3C\\x3D\\x3E\\x3F\\x40';

	testString = "123456789:;<=>?@ABC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["456789:;<=>?@"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\x7B\\x7C\\x7D\\x7E';

	testString = "1234{|}~ABC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["{|}~"]), String(testString.match(new RegExp(testPattern))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(new RegExp('[A-\\x5A]+'))",
	                                    String(["FOUND"]), String('canthisbeFOUND'.match(new RegExp('[A-\\x5A]+'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(new RegExp('[\\x61-\\x7A]+'))",
	                                    String(["canthisbe"]), String('canthisbeFOUND'.match(new RegExp('[\\x61-\\x7A]+'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(/[\\x61-\\x7A]+/)",
	                                    String(["canthisbe"]), String('canthisbeFOUND'.match(/[\x61-\x7A]+/)));

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
