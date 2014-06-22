var completed = false;
var	testcases;
var tc = 0;

SECTION	= "";
VERSION	= "";
BUGNUMBER =	"";

var	GLOBAL = "[object global]";
var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";

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

    this.passed = getTestCaseResult( this.expect, this.actual );
}
function startTest() {
/*
    //  JavaScript 1.3 is supposed to be compliant ecma version 1.0
    if ( VERSION == "ECMA_1" ) {
        version ( 130 );
    }
    if ( VERSION == "JS_1.3" ) {
        version ( 130 );
    }
    if ( VERSION == "JS_1.2" ) {
        version ( 120 );
    }
    if ( VERSION  == "JS_1.1" ) {
        version ( 110 );
    }
    // for ecma version 2.0, we will leave the javascript version to
    // the default ( for now ).
*/

    if ( BUGNUMBER ) {
            writeLineToLog ("BUGNUMBER: " + BUGNUMBER );
    }

    testcases = new Array();
    tc = 0;
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
   var gc;
   if ( gc != undefined ) {
        gc();
   }
}
