/**
 *  File Name:          exception-009
 *  ECMA Section:
 *  Description:        Tests for JavaScript Standard Exceptions
 *
 *  Regression test for nested try blocks.
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=312964
 *
 *  Author:             christine@netscape.com
 *  Date:               31 August 1998
 */
    var SECTION = "exception-009";
    var VERSION = "JS1_4";
    var TITLE   = "Tests for JavaScript Standard Exceptions: SyntaxError";
    var BUGNUMBER= "312964";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    try {
        expect = "passed:  no exception thrown";
        result = expect;
        Nested_1();
    } catch ( e ) {
        result = "failed: threw " + e;
    } finally {
            testcases[tc++] = new TestCase(
                SECTION,
                "nested try",
                expect,
                result );
    }


    test();

    function Nested_1() {
        try {
            try {
            } catch (a) {
            } finally {
            }
        } catch (b) {
        } finally {
        }
    }
