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
	Filename:     continue.js
	Description:  'Tests the continue statement'

	Author:       Nick Lerissa
	Date:         March 18, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'statements: continue';

	writeHeaderToLog("Executing script: continue.js");
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	var i,j;

    j = 0;
	for (i = 0; i < 200; i++)
	{
	    if (i == 100)
	        continue;
	    j++;
	}

    // '"continue" in a "for" loop'
	testcases[count++] = new TestCase ( SECTION, '"continue" in "for" loop',
	                                    199, j);


    j = 0;
    out1:
	for (i = 0; i < 1000; i++)
	{
	    if (i == 100)
	    {
	        out2:
	        for (var k = 0; k < 1000; k++)
	        {
	            if (k == 500) continue out1;
	        }
	        j = 3000;
	    }
	    j++;
	}

    // '"continue" in a "for" loop with a "label"'
	testcases[count++] = new TestCase ( SECTION, '"continue" in "for" loop with a "label"',
	                                    999, j);

	i = 0;
	j = 1;

	while (i != j)
	{
	    i++;
	    if (i == 100) continue;
	    j++;
	}

	// '"continue" in a "while" loop'
	testcases[count++] = new TestCase ( SECTION, '"continue" in a "while" loop',
	                                    100, j );

    j = 0;
    i = 0;
    out3:
	while (i < 1000)
	{
	    if (i == 100)
	    {
	        var k = 0;
	        out4:
	        while (k < 1000)
	        {
	            if (k == 500)
	            {
	                i++;
	                continue out3;
	            }
	            k++;
	        }
	        j = 3000;
	    }
	    j++;
	    i++;
	}

    // '"continue" in a "while" loop with a "label"'
	testcases[count++] = new TestCase ( SECTION, '"continue" in a "while" loop with a "label"',
	                                    999, j);

	i = 0;
	j = 1;

	do
	{
	    i++;
	    if (i == 100) continue;
	    j++;
	} while (i != j);


	// '"continue" in a "do" loop'
	testcases[count++] = new TestCase ( SECTION, '"continue" in a "do" loop',
	                                    100, j );

    j = 0;
    i = 0;
    out5:
	do
	{
	    if (i == 100)
	    {
	        var k = 0;
	        out6:
	        do
	        {
	            if (k == 500)
	            {
	                i++;
	                continue out5;
	            }
	            k++;
	        }while (k < 1000);
	        j = 3000;
	    }
	    j++;
	    i++;
	}while (i < 1000);

    // '"continue" in a "do" loop with a "label"'
	testcases[count++] = new TestCase ( SECTION, '"continue" in a "do" loop with a "label"',
	                                    999, j);

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
