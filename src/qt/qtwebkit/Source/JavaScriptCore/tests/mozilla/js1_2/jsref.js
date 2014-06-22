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
var	completed =	false;
var	testcases;
var tc = 0;

SECTION	= "";
VERSION	= "";
BUGNUMBER =	"";
EXCLUDE = "";

/*
 * constant strings
 */
var	GLOBAL = "[object global]";
var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";

var	DEBUG =	false;

version("120");
/*
 * change this for date tests if you're not in PST
 */

TZ_DIFF	= -8;
/* wrapper for test cas constructor that doesn't require the SECTION
 * argument.
 */

function AddTestCase( description, expect, actual ) {
    testcases[tc++] = new TestCase( SECTION, description, expect, actual );
}
function TestCase( n, d, e, a ) {
    this.name        = n;
    this.description = d;
    this.expect      = e;
    this.actual      = a;
    this.passed      = true;
    this.reason      = "";
    this.bugnumber   = BUGNUMBER;

    this.passed = getTestCaseResult( this.expect, this.actual );
}
function startTest() {
    version(120);

    // for ecma version 2.0, we will leave the javascript version to
    // the default ( for now ).
    // print out bugnumber

    if ( BUGNUMBER ) {
            writeLineToLog ("BUGNUMBER: " + BUGNUMBER );
    }

    testcases = new Array();
    tc = 0;

}
function test() {
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
function getTestCaseResult( expect, actual ) {
    //  because ( NaN == NaN ) always returns false, need to do
    //  a special compare to see if we got the right result.
        if ( actual != actual ) {
            if ( typeof actual == "object" ) {
                actual = "NaN object";
            } else {
                actual = "NaN number";
            }
        }
        if ( expect != expect ) {
            if ( typeof expect == "object" ) {
                expect = "NaN object";
            } else {
                expect = "NaN number";
            }
        }

        var passed = ( expect == actual ) ? true : false;

    //  if both objects are numbers, give a little leeway for rounding.
        if (    !passed
                && typeof(actual) == "number"
                && typeof(expect) == "number"
            ) {
                if ( Math.abs(actual-expect) < 0.0000001 ) {
                    passed = true;
                }
        }

    //  verify type is the same
        if ( typeof(expect) != typeof(actual) ) {
            passed = false;
        }

        return passed;
}
/*
 * Begin printing functions.  These functions use the shell's
 * print function.  When running tests in the browser, these
 * functions, override these functions with functions that use
 * document.write.
 */

function writeTestCaseResult( expect, actual, string ) {
		var	passed = getTestCaseResult(	expect,	actual );
		writeFormattedResult( expect, actual, string, passed );
		return passed;
}
function writeFormattedResult( expect, actual, string, passed ) {
        var s = string ;
        s += ( passed ) ? PASSED : FAILED + expect;
        writeLineToLog( s);
        return passed;
}
function writeLineToLog( string	) {
	print( string );
}
function writeHeaderToLog( string )	{
	print( string );
}
/* end of print functions */


function stopTest() {
    var sizeTag  = "<#TEST CASES SIZE>";
    var doneTag  = "<#TEST CASES DONE>";
    var beginTag = "<#TEST CASE ";
    var endTag   = ">";

    print(sizeTag);
    print(testcases.length);
    for (tc = 0; tc < testcases.length; tc++)
    {
        print(beginTag + 'PASSED'      + endTag);
        print(testcases[tc].passed);
        print(beginTag + 'NAME'        + endTag);
        print(testcases[tc].name);
        print(beginTag + 'EXPECTED'    + endTag);
        print(testcases[tc].expect);
        print(beginTag + 'ACTUAL'      + endTag);
        print(testcases[tc].actual);
        print(beginTag + 'DESCRIPTION' + endTag);
        print(testcases[tc].description);
        print(beginTag + 'REASON'      + endTag);
        print(( testcases[tc].passed ) ? "" : "wrong value ");
        print(beginTag + 'BUGNUMBER'   + endTag);
        print( BUGNUMBER );
    }
    print(doneTag);
    gc();
}

function getFailedCases() {
  for ( var i = 0; i < testcases.length; i++ ) {
     if ( ! testcases[i].passed ) {
        print( testcases[i].description +" = " +testcases[i].actual +" expected: "+ testcases[i].expect );
     }
  }
}
function err( msg, page, line ) {
    testcases[tc].actual = "error";
    testcases[tc].reason = msg;
    writeTestCaseResult( testcases[tc].expect,
                         testcases[tc].actual,
                         testcases[tc].description +" = "+ testcases[tc].actual +
                         ": " + testcases[tc].reason );
    stopTest();
    return true;
}
function Enumerate ( o ) {
    var p;
    for ( p in o ) {
        print( p +": " + o[p] );
    }
}
function GetContext() {
    return Packages.com.netscape.javascript.Context.getCurrentContext();
}
function OptLevel( i ) {
    i = Number(i);
    var cx = GetContext();
    cx.setOptimizationLevel(i);
}
