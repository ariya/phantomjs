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
	Filename:     charCodeAt.js
	Description:  'This tests new String object method: charCodeAt'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'String:charCodeAt';

	writeHeaderToLog('Executing script: charCodeAt.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	
	var aString = new String("tEs5");
	
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt(-2)", NaN, aString.charCodeAt(-2));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt(-1)", NaN, aString.charCodeAt(-1));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 0)", 116, aString.charCodeAt( 0));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 1)",  69, aString.charCodeAt( 1));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 2)", 115, aString.charCodeAt( 2));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 3)",  53, aString.charCodeAt( 3));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 4)", NaN, aString.charCodeAt( 4));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( 5)", NaN, aString.charCodeAt( 5));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt( Infinity)", NaN, aString.charCodeAt( Infinity));
	testcases[count++] = new TestCase( SECTION, "aString.charCodeAt(-Infinity)", NaN, aString.charCodeAt(-Infinity));
	//testcases[count++] = new TestCase( SECTION, "aString.charCodeAt(  )", 116, aString.charCodeAt( ));
	
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

