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
	Filename:     string_replace.js
	Description:  'Tests the replace method on Strings using regular expressions'

	Author:       Nick Lerissa
	Date:         March 11, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'String: replace';

	writeHeaderToLog('Executing script: string_replace.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'adddb'.replace(/ddd/,"XX")
	testcases[count++] = new TestCase ( SECTION, "'adddb'.replace(/ddd/,'XX')",
	                                    "aXXb", 'adddb'.replace(/ddd/,'XX'));

    // 'adddb'.replace(/eee/,"XX")
	testcases[count++] = new TestCase ( SECTION, "'adddb'.replace(/eee/,'XX')",
	                                    'adddb', 'adddb'.replace(/eee/,'XX'));

    // '34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**')
	testcases[count++] = new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**')",
	                                    "34 56 ** 12", '34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**'));

    // '34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX')
	testcases[count++] = new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX')",
	                                    "34 56 78b 12", '34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX'));

    // 'original'.replace(new RegExp(),'XX')
	testcases[count++] = new TestCase ( SECTION, "'original'.replace(new RegExp(),'XX')",
	                                    "XXoriginal", 'original'.replace(new RegExp(),'XX'));

    // 'qwe ert x\t\n 345654AB'.replace(new RegExp('x\s*\d+(..)$'),'****')
	testcases[count++] = new TestCase ( SECTION, "'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****')",
	                                    "qwe ert ****", 'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****'));


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
