/**
 *  File Name:          boolean-001.js
 *  Description:
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=99232
 *
 *  eval("function f(){}function g(){}") at top level is an error for JS1.2
 *     and above (missing ; between named function expressions), but declares f
 *     and g as functions below 1.2.
 *
 * Fails to produce error regardless of version:
 * js> version(100)
 * 120
 * js> eval("function f(){}function g(){}")
 * js> version(120);
 * 100
 * js> eval("function f(){}function g(){}")
 * js>
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "function-001.js";
    var VERSION = "JS_12";
    var TITLE   = "functions not separated by semicolons are errors in version 120 and higher";
    var BUGNUMBER="10278";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "fail";
    var exception = "no exception thrown";

    try {
        eval("function f(){}function g(){}");
    } catch ( e ) {
        result = "pass"
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "eval(\"function f(){}function g(){}\") (threw "+exception,
        "pass",
        result );

    test();


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
