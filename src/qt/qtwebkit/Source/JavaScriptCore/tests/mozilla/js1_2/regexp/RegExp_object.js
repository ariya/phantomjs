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
	Filename:     RegExp_object.js
	Description:  'Tests regular expressions creating RexExp Objects'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: object';

	writeHeaderToLog('Executing script: RegExp_object.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    var SSN_pattern = new RegExp("\\d{3}-\\d{2}-\\d{4}");

    // testing SSN pattern
	testcases[count++] = new TestCase ( SECTION, "'Test SSN is 123-34-4567'.match(SSN_pattern))",
	                                    String(["123-34-4567"]), String('Test SSN is 123-34-4567'.match(SSN_pattern)));

    // testing SSN pattern
	testcases[count++] = new TestCase ( SECTION, "'Test SSN is 123-34-4567'.match(SSN_pattern))",
	                                    String(["123-34-4567"]), String('Test SSN is 123-34-4567'.match(SSN_pattern)));

    var PHONE_pattern = new RegExp("\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})");
    // testing PHONE pattern
	testcases[count++] = new TestCase ( SECTION, "'Our phone number is (408)345-2345.'.match(PHONE_pattern))",
	                                    String(["(408)345-2345","408","345","2345"]), String('Our phone number is (408)345-2345.'.match(PHONE_pattern)));

    // testing PHONE pattern
	testcases[count++] = new TestCase ( SECTION, "'The phone number is 408-345-2345!'.match(PHONE_pattern))",
	                                    String(["408-345-2345","408","345","2345"]), String('The phone number is 408-345-2345!'.match(PHONE_pattern)));

    // testing PHONE pattern
	testcases[count++] = new TestCase ( SECTION, "String(PHONE_pattern.toString())",
	                                    "/\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})/", String(PHONE_pattern.toString()));

	// testing conversion to String
	testcases[count++] = new TestCase ( SECTION, "PHONE_pattern + ' is the string'",
	                                    "/\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})/ is the string",PHONE_pattern + ' is the string');

	// testing conversion to int
	testcases[count++] = new TestCase ( SECTION, "SSN_pattern - 8",
	                                    NaN,SSN_pattern - 8);

    var testPattern = new RegExp("(\\d+)45(\\d+)90");

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
