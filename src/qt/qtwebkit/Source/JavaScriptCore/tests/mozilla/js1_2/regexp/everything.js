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
	Filename:     everything.js
	Description:  'Tests regular expressions'

	Author:       Nick Lerissa
	Date:         March 24, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp';

	writeHeaderToLog('Executing script: everything.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // 'Sally and Fred are sure to come.'.match(/^[a-z\s]*/i)
	testcases[count++] = new TestCase ( SECTION, "'Sally and Fred are sure to come'.match(/^[a-z\\s]*/i)",
	                                    String(["Sally and Fred are sure to come"]), String('Sally and Fred are sure to come'.match(/^[a-z\s]*/i)));

    // 'test123W+xyz'.match(new RegExp('^[a-z]*[0-9]+[A-Z]?.(123|xyz)$'))
	testcases[count++] = new TestCase ( SECTION, "'test123W+xyz'.match(new RegExp('^[a-z]*[0-9]+[A-Z]?.(123|xyz)$'))",
	                                    String(["test123W+xyz","xyz"]), String('test123W+xyz'.match(new RegExp('^[a-z]*[0-9]+[A-Z]?.(123|xyz)$'))));

    // 'number one 12365 number two 9898'.match(/(\d+)\D+(\d+)/)
	testcases[count++] = new TestCase ( SECTION, "'number one 12365 number two 9898'.match(/(\d+)\D+(\d+)/)",
	                                    String(["12365 number two 9898","12365","9898"]), String('number one 12365 number two 9898'.match(/(\d+)\D+(\d+)/)));

    var simpleSentence = /(\s?[^\!\?\.]+[\!\?\.])+/;
    // 'See Spot run.'.match(simpleSentence)
	testcases[count++] = new TestCase ( SECTION, "'See Spot run.'.match(simpleSentence)",
	                                    String(["See Spot run.","See Spot run."]), String('See Spot run.'.match(simpleSentence)));

    // 'I like it. What's up? I said NO!'.match(simpleSentence)
	testcases[count++] = new TestCase ( SECTION, "'I like it. What's up? I said NO!'.match(simpleSentence)",
	                                    String(["I like it. What's up? I said NO!",' I said NO!']), String('I like it. What\'s up? I said NO!'.match(simpleSentence)));

    // 'the quick brown fox jumped over the lazy dogs'.match(/((\w+)\s*)+/)
	testcases[count++] = new TestCase ( SECTION, "'the quick brown fox jumped over the lazy dogs'.match(/((\\w+)\\s*)+/)",
	                                    String(['the quick brown fox jumped over the lazy dogs','dogs','dogs']),String('the quick brown fox jumped over the lazy dogs'.match(/((\w+)\s*)+/)));

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
