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
	Filename:     nesting.js
	Description:  'This tests the nesting of functions'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'functions: nesting';

	writeHeaderToLog('Executing script: nesting.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	function outer_func(x)
	{
	    var y = "outer";

	    testcases[count++] = new TestCase( SECTION, "outer:x    ",
	                                                1111,  x);
	    testcases[count++] = new TestCase( SECTION, "outer:y    ",
	                                                'outer', y);
	    function inner_func(x)
	    {
	        var y = "inner";
	        testcases[count++] = new TestCase( SECTION, "inner:x    ",
	                                                    2222,  x);
	        testcases[count++] = new TestCase( SECTION, "inner:y    ",
	                                                    'inner', y);
	    };

	    inner_func(2222);
	    testcases[count++] = new TestCase( SECTION, "outer:x    ",
	                                                1111,  x);
	    testcases[count++] = new TestCase( SECTION, "outer:y    ",
	                                                'outer', y);
	}

	outer_func(1111);

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

