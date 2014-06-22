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
	Filename:     switch2.js
	Description:  'Tests the switch statement'

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=323696

	Author:       Norris Boyd
	Date:         July 31, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'statements: switch';
	var BUGNUMBER="323626";

	writeHeaderToLog("Executing script: switch2.js");
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	// test defaults not at the end; regression test for a bug that
	// nearly made it into 4.06
	function f0(i) {
	    switch(i) {
		default:
		case "a":
		case "b":
		    return "ab*"
		case "c":
		    return "c";
		case "d":
		    return "d";
	    }
	    return "";
	}
	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f0("a"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f0("b"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f0("*"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f0("c"), "c");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f0("d"), "d");

	function f1(i) {
	    switch(i) {
		case "a":
		case "b":
		default:
		    return "ab*"
		case "c":
		    return "c";
		case "d":
		    return "d";
	    }
	    return "";
	}

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f1("a"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f1("b"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f1("*"), "ab*");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f1("c"), "c");

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f1("d"), "d");

	// Switch on integer; will use TABLESWITCH opcode in C engine
	function f2(i) {
	    switch (i) {
	        case 0:
	        case 1:
                    return 1;
	        case 2:
 	            return 2;
	    }
	    // with no default, control will fall through
	    return 3;
	}

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f2(0), 1);

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f2(1), 1);

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f2(2), 2);

	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  f2(3), 3);

	// empty switch: make sure expression is evaluated
	var se = 0;
	switch (se = 1) {
	}
	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  se, 1);

	// only default
	se = 0;
	switch (se) {
	    default:
  	        se = 1;
	}
	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  se, 1);

	// in loop, break should only break out of switch
	se = 0;
	for (var i=0; i < 2; i++) {
	    switch (i) {
	        case 0:
	        case 1:
	            break;
	    }
	    se = 1;
	}
	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  se, 1);

	// test "fall through"
	se = 0;
	i = 0;
	switch (i) {
	    case 0:
	        se++;
		/* fall through */
	    case 1:
	        se++;
	        break;
	}
	testcases[count++] = new TestCase(SECTION, 'switch statement',
	                                  se, 2);

    test();

	// Needed: tests for evaluation time of case expressions.
	// This issue was under debate at ECMA, so postponing for now.

	function test()	{
	    writeLineToLog("hi");
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
