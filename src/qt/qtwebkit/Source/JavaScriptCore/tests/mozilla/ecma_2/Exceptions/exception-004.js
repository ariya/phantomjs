/**
 *  File Name:          exception-004
 *  ECMA Section:
 *  Description:        Tests for JavaScript Standard Exceptions
 *
 *  ToObject error.
 *
 *  Author:             christine@netscape.com
 *  Date:               31 August 1998
 */
    var SECTION = "exception-004";
    var VERSION = "js1_4";
    var TITLE   = "Tests for JavaScript Standard Exceptions: ToObjectError";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    ToObject_1();

    test();

    function ToObject_1() {
        result = "failed: no exception thrown";
        exception = null;

        try {
           result = foo["bar"];
        } catch ( e ) {
            result = "passed:  threw exception",
            exception = e.toString();
        } finally {
            testcases[tc++] = new TestCase(
                SECTION,
                "foo[\"bar\"] [ exception is " + exception +" ]",
                "passed:  threw exception",
                result );
        }
    }

