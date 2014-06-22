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
	Filename:     string_split.js
	Description:  'Tests the split method on Strings using regular expressions'

	Author:       Nick Lerissa
	Date:         March 11, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'String: split';

	writeHeaderToLog('Executing script: string_split.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'a b c de f'.split(/\s/)
	testcases[count++] = new TestCase ( SECTION, "'a b c de f'.split(/\s/)",
	                                    String(["a","b","c","de","f"]), String('a b c de f'.split(/\s/)));

    // 'a b c de f'.split(/\s/,3)
	testcases[count++] = new TestCase ( SECTION, "'a b c de f'.split(/\s/,3)",
	                                    String(["a","b","c"]), String('a b c de f'.split(/\s/,3)));

    // 'a b c de f'.split(/X/)
	testcases[count++] = new TestCase ( SECTION, "'a b c de f'.split(/X/)",
	                                    String(["a b c de f"]), String('a b c de f'.split(/X/)));

    // 'dfe23iu 34 =+65--'.split(/\d+/)
	testcases[count++] = new TestCase ( SECTION, "'dfe23iu 34 =+65--'.split(/\d+/)",
	                                    String(["dfe","iu "," =+","--"]), String('dfe23iu 34 =+65--'.split(/\d+/)));

    // 'dfe23iu 34 =+65--'.split(new RegExp('\d+'))
	testcases[count++] = new TestCase ( SECTION, "'dfe23iu 34 =+65--'.split(new RegExp('\\d+'))",
	                                    String(["dfe","iu "," =+","--"]), String('dfe23iu 34 =+65--'.split(new RegExp('\\d+'))));

    // 'abc'.split(/[a-z]/)
	testcases[count++] = new TestCase ( SECTION, "'abc'.split(/[a-z]/)",
	                                    String(["","",""]), String('abc'.split(/[a-z]/)));

    // 'abc'.split(/[a-z]/)
	testcases[count++] = new TestCase ( SECTION, "'abc'.split(/[a-z]/)",
	                                    String(["","",""]), String('abc'.split(/[a-z]/)));

    // 'abc'.split(new RegExp('[a-z]'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.split(new RegExp('[a-z]'))",
	                                    String(["","",""]), String('abc'.split(new RegExp('[a-z]'))));

    // 'abc'.split(new RegExp('[a-z]'))
	testcases[count++] = new TestCase ( SECTION, "'abc'.split(new RegExp('[a-z]'))",
	                                    String(["","",""]), String('abc'.split(new RegExp('[a-z]'))));

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
