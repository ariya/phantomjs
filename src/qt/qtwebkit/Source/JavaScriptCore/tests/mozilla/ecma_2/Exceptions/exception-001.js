/**
 *  File Name:          exception-001
 *  ECMA Section:
 *  Description:        Tests for JavaScript Standard Exceptions
 *
 *  Call error.
 *
 *  Author:             christine@netscape.com
 *  Date:               31 August 1998
 */
    var SECTION = "exception-001";
    var VERSION = "js1_4";
    var TITLE   = "Tests for JavaScript Standard Exceptions:  CallError";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    Call_1();

    test();

    function Call_1() {
        result = "failed: no exception thrown";
        exception = null;

        try {
            Math();
        } catch ( e ) {
            result = "passed:  threw exception",
            exception = e.toString();
        } finally {
            testcases[tc++] = new TestCase(
                SECTION,
                "Math() [ exception is " + exception +" ]",
                "passed:  threw exception",
                result );
        }
    }

