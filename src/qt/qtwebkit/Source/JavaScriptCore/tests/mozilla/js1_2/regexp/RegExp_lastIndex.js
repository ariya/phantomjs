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
	Filename:     RegExp_lastIndex.js
	Description:  'Tests RegExps lastIndex property'

	Author:       Nick Lerissa
	Date:         March 17, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: lastIndex';
	var BUGNUMBER="123802";

	writeHeaderToLog('Executing script: RegExp_lastIndex.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // re=/x./g; re.lastIndex=4; re.exec('xyabcdxa');
    re=/x./g;
    re.lastIndex=4;
	testcases[count++] = new TestCase ( SECTION, "re=/x./g; re.lastIndex=4; re.exec('xyabcdxa')",
	                                    '["xa"]', String(re.exec('xyabcdxa')));

    // re.lastIndex
	testcases[count++] = new TestCase ( SECTION, "re.lastIndex",
	                                    8, re.lastIndex);

    // re.exec('xyabcdef');
	testcases[count++] = new TestCase ( SECTION, "re.exec('xyabcdef')",
	                                    null, re.exec('xyabcdef'));

    // re.lastIndex
	testcases[count++] = new TestCase ( SECTION, "re.lastIndex",
	                                    0, re.lastIndex);

    // re.exec('xyabcdef');
	testcases[count++] = new TestCase ( SECTION, "re.exec('xyabcdef')",
	                                    '["xy"]', String(re.exec('xyabcdef')));

    // re.lastIndex=30; re.exec('123xaxbxc456');
    re.lastIndex=30;
	testcases[count++] = new TestCase ( SECTION, "re.lastIndex=30; re.exec('123xaxbxc456')",
	                                    null, re.exec('123xaxbxc456'));

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
