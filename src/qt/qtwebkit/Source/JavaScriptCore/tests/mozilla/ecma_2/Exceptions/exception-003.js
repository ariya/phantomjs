/**
 *  File Name:          exception-003
 *  ECMA Section:
 *  Description:        Tests for JavaScript Standard Exceptions
 *
 *  Target error.
 *
 *  Author:             christine@netscape.com
 *  Date:               31 August 1998
 */
    var SECTION = "exception-003";
    var VERSION = "js1_4";
    var TITLE   = "Tests for JavaScript Standard Exceptions: TargetError";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    Target_1();

    test();

    function Target_1() {
        result = "failed: no exception thrown";
        exception = null;

        try {
            string = new String("hi");
            string.toString = Boolean.prototype.toString;
            string.toString();
        } catch ( e ) {
            result = "passed:  threw exception",
            exception = e.toString();
        } finally {
            testcases[tc++] = new TestCase(
                SECTION,
                "string = new String(\"hi\");"+
                "string.toString = Boolean.prototype.toString" +
                "string.toString() [ exception is " + exception +" ]",
                "passed:  threw exception",
                result );
        }
    }

