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
	Filename:     switch.js
	Description:  'Tests the switch statement'

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=323696

	Author:       Nick Lerissa
	Date:         March 19, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'statements: switch';
	var BUGNUMBER="323696";

	writeHeaderToLog("Executing script: switch.js");
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	var var1 = "match string";
	var match1 = false;
	var match2 = false;
	var match3 = false;

	switch (var1)
	{
	    case "match string":
	        match1 = true;
	    case "bad string 1":
	        match2 = true;
	        break;
	    case "bad string 2":
	        match3 = true;
	}

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    true, match1);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    true, match2);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    false, match3);

	var var2 = 3;

	var match1 = false;
	var match2 = false;
	var match3 = false;
	var match4 = false;
	var match5 = false;

	switch (var2)
	{
	    case 1:
/*	        switch (var1)
	        {
	            case "foo":
	                match1 = true;
	            break;
	            case 3:
	                match2 = true;
	            break;
	        }*/
	        match3 = true;
	    break;
	    case 2:
	        match4 = true;
	    break;
	    case 3:
	        match5 = true;
	    break;
	}
	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    false, match1);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    false, match2);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    false, match3);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    false, match4);

	testcases[count++] = new TestCase ( SECTION, 'switch statement',
	                                    true, match5);

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
