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
	Filename:     Number.js
	Description:  'This tests the function Number(Object)'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'functions: Number';
	var BUGNUMBER="123435";

	writeHeaderToLog('Executing script: Number.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	date = new Date(2200);

	testcases[count++] = new TestCase( SECTION, "Number(new Date(2200))  ",
	                                            2200,  (Number(date)));
	testcases[count++] = new TestCase( SECTION, "Number(true)            ",
	                                            1,  (Number(true)));
	testcases[count++] = new TestCase( SECTION, "Number(false)           ",
	                                            0,  (Number(false)));
	testcases[count++] = new TestCase( SECTION, "Number('124')           ",
	                                            124,  (Number('124')));
	testcases[count++] = new TestCase( SECTION, "Number('1.23')          ",
	                                            1.23,  (Number('1.23')));
	testcases[count++] = new TestCase( SECTION, "Number({p:1})           ",
	                                            NaN,  (Number({p:1})));
	testcases[count++] = new TestCase( SECTION, "Number(null)            ",
	                                            0,  (Number(null)));
	testcases[count++] = new TestCase( SECTION, "Number(-45)             ",
	                                            -45,  (Number(-45)));

        // http://scopus.mcom.com/bugsplat/show_bug.cgi?id=123435
        // under js1.2, Number([1,2,3]) should return 3.

	/*
	http://bugs.webkit.org/show_bug.cgi?id=11545#c4
	According to ECMA 9.3, when input type was Object, should call
	ToPrimitive(input arg, hint Number) first, and than ToNumber() later. However,
	ToPrimitive() will use [[DefaultValue]](hint) rule when input Type was Object
	(ECMA 8.6.2.6). So the input [1,2,3] will applied [[DefaultValue]](hint) rule
	with hint Number, and it looks like this:

	toString(valuOf([1,2,3]))  => toString(1,2,3) => '1,2,3'

	Than ToNumber('1,2,3') results NaN based on ECMA 9.3.1: If the grammar cannot
	interpret the string as an expansion of StringNumericLiteral, then the result 
	of ToNumber is NaN.
	*/
	
	//testcases[count++] = new TestCase( SECTION, "Number([1,2,3])         ",
	//                                            3,  (Number([1,2,3])));

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

