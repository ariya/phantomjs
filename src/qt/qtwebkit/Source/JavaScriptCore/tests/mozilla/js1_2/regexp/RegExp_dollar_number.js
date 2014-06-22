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
	Filename:     RegExp_dollar_number.js
	Description:  'Tests RegExps $1, ..., $9 properties'

	Author:       Nick Lerissa
	Date:         March 12, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: $1, ..., $9';
	var BUGNUMBER="123802";

	writeHeaderToLog('Executing script: RegExp_dollar_number.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$1
    'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/);
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$1",
	                                    'abcdefghi', RegExp.$1);

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$2
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$2",
	                                    'bcdefgh', RegExp.$2);

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$3
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$3",
	                                    'cdefg', RegExp.$3);

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$4
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$4",
	                                    'def', RegExp.$4);

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$5
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$5",
	                                    'e', RegExp.$5);

    // 'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$6
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/(a(b(c(d(e)f)g)h)i)/); RegExp.$6",
	                                    '', RegExp.$6);

    var a_to_z = 'abcdefghijklmnopqrstuvwxyz';
    var regexp1 = /(a)b(c)d(e)f(g)h(i)j(k)l(m)n(o)p(q)r(s)t(u)v(w)x(y)z/
    // 'abcdefghijklmnopqrstuvwxyz'.match(/(a)b(c)d(e)f(g)h(i)j(k)l(m)n(o)p(q)r(s)t(u)v(w)x(y)z/); RegExp.$1
    a_to_z.match(regexp1);

	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$1",
	                                    'a', RegExp.$1);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$2",
	                                    'c', RegExp.$2);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$3",
	                                    'e', RegExp.$3);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$4",
	                                    'g', RegExp.$4);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$5",
	                                    'i', RegExp.$5);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$6",
	                                    'k', RegExp.$6);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$7",
	                                    'm', RegExp.$7);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$8",
	                                    'o', RegExp.$8);
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$9",
	                                    'q', RegExp.$9);
/*
	testcases[count++] = new TestCase ( SECTION, "'" + a_to_z + "'.match((a)b(c)....(y)z); RegExp.$10",
	                                    's', RegExp.$10);
*/
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
