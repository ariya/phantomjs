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
	Filename:     String.js
	Description:  'This tests the function String(Object)'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'functions: String';

	writeHeaderToLog('Executing script: String.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	testcases[count++] = new TestCase( SECTION, "String(true)            ",
	                                            'true',  (String(true)));
	testcases[count++] = new TestCase( SECTION, "String(false)           ",
	                                            'false',  (String(false)));
	testcases[count++] = new TestCase( SECTION, "String(-124)           ",
	                                            '-124',  (String(-124)));
	testcases[count++] = new TestCase( SECTION, "String(1.23)          ",
	                                            '1.23',  (String(1.23)));
	/*
	http://bugs.webkit.org/show_bug.cgi?id=11545#c5
	According to ECMA 9.8, when input type of String object argument was Object, we
	should applied ToPrimitive(input arg, hint String) first, and later ToString(). 
	And just like previous one, ToPrimitive() will use [[DefaultValue]](hint)
	with hint String to convert the input (toString() below uses the rule in ECMA 15.2.4.2):

	valueOf(toString({p:1}) => valueOf('[object Object]') => '[object Object]'

	And ToString() called after ToPrimitive(), so the correct result would be:

	[object Object]
	*/
	//testcases[count++] = new TestCase( SECTION, "String({p:1})           ",
	//                                            '{p:1}',  (String({p:1})));
	testcases[count++] = new TestCase( SECTION, "String(null)            ",
	                                            'null',  (String(null)));
	/*
	http://bugs.webkit.org/show_bug.cgi?id=11545#c5
	According to ECMA 9.8, when input type of String object argument was Object, we
	should applied ToPrimitive(input arg, hint String) first, and later ToString(). 
	And just like previous one, ToPrimitive() will use [[DefaultValue]](hint)
	with hint String to convert the input (toString() below uses the rule in ECMA 15.2.4.2):

	valueOf(toString([1,2,3])) => valueOf('1,2,3') => '1,2,3'

	And ToString() called after ToPrimitive(), so the correct result would be:

	1,2,3
	*/
	//testcases[count++] = new TestCase( SECTION, "String([1,2,3])            ",
	//                                            '[1, 2, 3]',  (String([1,2,3])));


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

