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
	Filename:     RegExp_leftContext.js
	Description:  'Tests RegExps leftContext property'

	Author:       Nick Lerissa
	Date:         March 12, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: leftContext';

	writeHeaderToLog('Executing script: RegExp_leftContext.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'abc123xyz'.match(/123/); RegExp.leftContext
    'abc123xyz'.match(/123/);
	testcases[count++] = new TestCase ( SECTION, "'abc123xyz'.match(/123/); RegExp.leftContext",
	                                    'abc', RegExp.leftContext);

    // 'abc123xyz'.match(/456/); RegExp.leftContext
    'abc123xyz'.match(/456/);
	testcases[count++] = new TestCase ( SECTION, "'abc123xyz'.match(/456/); RegExp.leftContext",
	                                    'abc', RegExp.leftContext);

    // 'abc123xyz'.match(/abc123xyz/); RegExp.leftContext
    'abc123xyz'.match(/abc123xyz/);
	testcases[count++] = new TestCase ( SECTION, "'abc123xyz'.match(/abc123xyz/); RegExp.leftContext",
	                                    '', RegExp.leftContext);

    // 'xxxx'.match(/$/); RegExp.leftContext
    'xxxx'.match(/$/);
	testcases[count++] = new TestCase ( SECTION, "'xxxx'.match(/$/); RegExp.leftContext",
	                                    'xxxx', RegExp.leftContext);

    // 'test'.match(/^/); RegExp.leftContext
    'test'.match(/^/);
	testcases[count++] = new TestCase ( SECTION, "'test'.match(/^/); RegExp.leftContext",
	                                    '', RegExp.leftContext);

    // 'xxxx'.match(new RegExp('$')); RegExp.leftContext
    'xxxx'.match(new RegExp('$'));
	testcases[count++] = new TestCase ( SECTION, "'xxxx'.match(new RegExp('$')); RegExp.leftContext",
	                                    'xxxx', RegExp.leftContext);

    // 'test'.match(new RegExp('^')); RegExp.leftContext
    'test'.match(new RegExp('^'));
	testcases[count++] = new TestCase ( SECTION, "'test'.match(new RegExp('^')); RegExp.leftContext",
	                                    '', RegExp.leftContext);

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
