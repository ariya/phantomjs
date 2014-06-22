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
	Filename:     octal.js
	Description:  'Tests regular expressions containing \<number> '

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: \# (octal) ';

	writeHeaderToLog('Executing script: octal.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	var testPattern = '\\101\\102\\103\\104\\105\\106\\107\\110\\111\\112\\113\\114\\115\\116\\117\\120\\121\\122\\123\\124\\125\\126\\127\\130\\131\\132';

	var testString = "12345ABCDEFGHIJKLMNOPQRSTUVWXYZ67890";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["ABCDEFGHIJKLMNOPQRSTUVWXYZ"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\141\\142\\143\\144\\145\\146\\147\\150\\151\\152\\153\\154\\155\\156\\157\\160\\161\\162\\163\\164\\165\\166\\167\\170\\171\\172';

	testString = "12345AabcdefghijklmnopqrstuvwxyzZ67890";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["abcdefghijklmnopqrstuvwxyz"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\40\\41\\42\\43\\44\\45\\46\\47\\50\\51\\52\\53\\54\\55\\56\\57\\60\\61\\62\\63';

	testString = "abc !\"#$%&'()*+,-./0123ZBC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String([" !\"#$%&'()*+,-./0123"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\64\\65\\66\\67\\70\\71\\72\\73\\74\\75\\76\\77\\100';

	testString = "123456789:;<=>?@ABC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["456789:;<=>?@"]), String(testString.match(new RegExp(testPattern))));

    testPattern = '\\173\\174\\175\\176';

	testString = "1234{|}~ABC";

	testcases[count++] = new TestCase ( SECTION,
	                                    "'" + testString + "'.match(new RegExp('" + testPattern + "'))",
	                                    String(["{|}~"]), String(testString.match(new RegExp(testPattern))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(new RegExp('[A-\\132]+'))",
	                                    String(["FOUND"]), String('canthisbeFOUND'.match(new RegExp('[A-\\132]+'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(new RegExp('[\\141-\\172]+'))",
	                                    String(["canthisbe"]), String('canthisbeFOUND'.match(new RegExp('[\\141-\\172]+'))));

	testcases[count++] = new TestCase ( SECTION,
	                                    "'canthisbeFOUND'.match(/[\\141-\\172]+/)",
	                                    String(["canthisbe"]), String('canthisbeFOUND'.match(/[\141-\172]+/)));

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
