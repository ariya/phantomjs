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
	Filename:     concat.js
	Description:  'This tests the new String object method: concat'

	Author:       NickLerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'String:concat';

	writeHeaderToLog('Executing script: concat.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	var aString = new String("test string");
	var bString = new String(" another ");

	testcases[count++] = new TestCase( SECTION, "aString.concat(' more')", "test string more",     aString.concat(' more').toString());
	testcases[count++] = new TestCase( SECTION, "aString.concat(bString)", "test string another ", aString.concat(bString).toString());
	testcases[count++] = new TestCase( SECTION, "aString                ", "test string",          aString.toString());
	testcases[count++] = new TestCase( SECTION, "bString                ", " another ",            bString.toString());
	testcases[count++] = new TestCase( SECTION, "aString.concat(345)    ", "test string345",       aString.concat(345).toString());
	testcases[count++] = new TestCase( SECTION, "aString.concat(true)   ", "test stringtrue",      aString.concat(true).toString());
	testcases[count++] = new TestCase( SECTION, "aString.concat(null)   ", "test stringnull",      aString.concat(null).toString());
	/*
	http://bugs.webkit.org/show_bug.cgi?id=11545#c3
	According to ECMA 15.5.4.6, the argument of concat should send to ToString and
	convert into a string value (not String object). So these arguments will be
	convert into '' and '1,2,3' under ECMA-262v3, not the js1.2 expected '[]' and
	'[1,2,3]'
	*/
	//testcases[count++] = new TestCase( SECTION, "aString.concat([])     ", "test string[]",          aString.concat([]).toString());
	//testcases[count++] = new TestCase( SECTION, "aString.concat([1,2,3])", "test string[1, 2, 3]",     aString.concat([1,2,3]).toString());

	testcases[count++] = new TestCase( SECTION, "'abcde'.concat(' more')", "abcde more",     'abcde'.concat(' more').toString());
	testcases[count++] = new TestCase( SECTION, "'abcde'.concat(bString)", "abcde another ", 'abcde'.concat(bString).toString());
	testcases[count++] = new TestCase( SECTION, "'abcde'                ", "abcde",          'abcde');
	testcases[count++] = new TestCase( SECTION, "'abcde'.concat(345)    ", "abcde345",       'abcde'.concat(345).toString());
	testcases[count++] = new TestCase( SECTION, "'abcde'.concat(true)   ", "abcdetrue",      'abcde'.concat(true).toString());
	testcases[count++] = new TestCase( SECTION, "'abcde'.concat(null)   ", "abcdenull",      'abcde'.concat(null).toString());
	/*
	http://bugs.webkit.org/show_bug.cgi?id=11545#c3
	According to ECMA 15.5.4.6, the argument of concat should send to ToString and
	convert into a string value (not String object). So these arguments will be
	convert into '' and '1,2,3' under ECMA-262v3, not the js1.2 expected '[]' and
	'[1,2,3]'
	*/
	//testcases[count++] = new TestCase( SECTION, "'abcde'.concat([])     ", "abcde[]",          'abcde'.concat([]).toString());
	//testcases[count++] = new TestCase( SECTION, "'abcde'.concat([1,2,3])", "abcde[1, 2, 3]",     'abcde'.concat([1,2,3]).toString());

	//what should this do:
	testcases[count++] = new TestCase( SECTION, "'abcde'.concat()       ", "abcde",          'abcde'.concat().toString());

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

