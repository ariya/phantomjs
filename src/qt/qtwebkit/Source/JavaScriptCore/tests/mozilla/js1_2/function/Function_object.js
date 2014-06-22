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
	Filename:     Function_object.js
	Description:  'Testing Function objects'

	Author:       Nick Lerissa
	Date:         April 17, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'functions: Function_object';

	writeHeaderToLog('Executing script: Function_object.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


    function a_test_function(a,b,c)
    {
        return a + b + c;
    }

    f = a_test_function;


    testcases[count++] = new TestCase( SECTION, "f.name",
                                                'a_test_function',  f.name);

    testcases[count++] = new TestCase( SECTION, "f.length",
                                                3,  f.length);

    testcases[count++] = new TestCase( SECTION, "f.arity",
                                                3,  f.arity);

    testcases[count++] = new TestCase( SECTION, "f(2,3,4)",
                                                9,  f(2,3,4));

    var fnName = version() == 120 ? '' : 'anonymous';

    testcases[count++] = new TestCase( SECTION, "(new Function()).name",
                                                fnName, (new Function()).name);

    testcases[count++] = new TestCase( SECTION, "(new Function()).toString()",
                                                '\nfunction ' + fnName + '() {\n}\n',  (new Function()).toString());

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

