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
	Filename:     control_characters.js
	Description:  'Tests regular expressions containing .'

	Author:       Nick Lerissa
	Date:         April 8, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'RegExp: .';
	var BUGNUMBER="123802";

	writeHeaderToLog('Executing script: control_characters.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // '‡O– Í:i¢ÿ'.match(new RegExp('.+'))
	testcases[count++] = new TestCase ( SECTION, "'‡O– Í:i¢ÿ'.match(new RegExp('.+'))",
	                                    String(['‡O– Í:i¢ÿ']), String('‡O– Í:i¢ÿ'.match(new RegExp('.+'))));

    // string1.match(new RegExp(string1))
    var string1 = '‡O– Í:i¢ÿ';
	testcases[count++] = new TestCase ( SECTION, "string1 = " + string1 + " string1.match(string1)",
	                                    String([string1]), String(string1.match(string1)));

    string1 = "";
    for (var i = 0; i < 32; i++)
        string1 += String.fromCharCode(i);
	testcases[count++] = new TestCase ( SECTION, "string1 = " + string1 + " string1.match(string1)",
	                                    String([string1]), String(string1.match(string1)));

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
