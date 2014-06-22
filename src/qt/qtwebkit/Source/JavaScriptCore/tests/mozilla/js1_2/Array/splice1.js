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
	Filename:     splice1.js
	Description:  'Tests Array.splice(x,y) w/no var args'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'String:splice 1';
	var BUGNUMBER="123795";

	writeHeaderToLog('Executing script: splice1.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	function mySplice(testArray, splicedArray, first, len, elements)
	{
	    var removedArray  = [];
	    var adjustedFirst = first;
	    var adjustedLen   = len;

	    if (adjustedFirst < 0) adjustedFirst = testArray.length + first;
	    if (adjustedFirst < 0) adjustedFirst = 0;

	    if (adjustedLen < 0) adjustedLen = 0;

	    for (i = 0; (i < adjustedFirst)&&(i < testArray.length); ++i)
	        splicedArray.push(testArray[i]);

	    if (adjustedFirst < testArray.length)
	        for (i = adjustedFirst; (i < adjustedFirst + adjustedLen) &&
	            (i < testArray.length); ++i)
	        {
	                removedArray.push(testArray[i]);
            }

	    for (i = 0; i < elements.length; i++) splicedArray.push(elements[i]);

	    for (i = adjustedFirst + adjustedLen; i < testArray.length; i++)
	        splicedArray.push(testArray[i]);

	    return removedArray;
	}

	function exhaustiveSpliceTest(testname, testArray)
	{
	    var errorMessage;
	    var passed = true;
	    var reason = "";

	    for (var first = -(testArray.length+2); first <= 2 + testArray.length; first++)
	    {
	        var actualSpliced   = [];
	        var expectedSpliced = [];
	        var actualRemoved   = [];
	        var expectedRemoved = [];

	        for (var len = 0; len < testArray.length + 2; len++)
	        {
	            actualSpliced   = [];
	            expectedSpliced = [];

	            for (var i = 0; i < testArray.length; ++i)
	                actualSpliced.push(testArray[i]);

	            actualRemoved   = actualSpliced.splice(first,len);
	            expectedRemoved = mySplice(testArray,expectedSpliced,first,len,[]);

	            var adjustedFirst = first;
	            if (adjustedFirst < 0) adjustedFirst = testArray.length + first;
	            if (adjustedFirst < 0) adjustedFirst = 0;

	            if (  (String(actualSpliced) != String(expectedSpliced))
	                ||(String(actualRemoved) != String(expectedRemoved)))
	            {
	                if (  (String(actualSpliced) == String(expectedSpliced))
	                    &&(String(actualRemoved) != String(expectedRemoved)) )
	                    {
	                        if ( (expectedRemoved.length == 1)
	                            &&(String(actualRemoved) == String(expectedRemoved[0]))) continue;
	                        if ( expectedRemoved.length == 0 && actualRemoved == void 0) continue;
                        }

	                errorMessage =
	                    "ERROR: 'TEST FAILED'\n" +
	                    "             test: " + "a.splice(" + first + "," + len + ",-97,new String('test arg'),[],9.8)\n" +
	                    "                a: " + String(testArray) + "\n" +
	                    "   actual spliced: " + String(actualSpliced) + "\n" +
	                    " expected spliced: " + String(expectedSpliced) + "\n" +
	                    "   actual removed: " + String(actualRemoved) + "\n" +
	                    " expected removed: " + String(expectedRemoved) + "\n";
	                writeHeaderToLog(errorMessage);
	                reason = reason + errorMessage;
	                passed = false;
	            }
	        }
	    }
	    var testcase = new TestCase( SECTION, testname, true, passed);
	    if (!passed)
	        testcase.reason = reason;
	    return testcase;
	}

	var a = ['a','test string',456,9.34,new String("string object"),[],['h','i','j','k']];
	var b = [1,2,3,4,5,6,7,8,9,0];

	testcases[count++] = exhaustiveSpliceTest("exhaustive splice w/no optional args 1",a);
	testcases[count++] = exhaustiveSpliceTest("exhaustive splice w/no optional args 1",b);

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

