/**
 *  File Name:          exception-007
 *  ECMA Section:
 *  Description:        Tests for JavaScript Standard Exceptions
 *
 *  DefaultValue error.
 *
 *  Author:             christine@netscape.com
 *  Date:               31 August 1998
 */
    var SECTION = "exception-007";
    var VERSION = "js1_4";
    var TITLE   = "Tests for JavaScript Standard Exceptions:  TypeError";
    var BUGNUMBER="318250";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    DefaultValue_1();

    test();


    /**
     * Getting the [[DefaultValue]] of any instances of MyObject
     * should result in a runtime error in ToPrimitive.
     */

    function MyObject() {
        this.toString = void 0;
        this.valueOf = new Object();
    }

    function DefaultValue_1() {
        result = "failed: no exception thrown";
        exception = null;

        try {
           result = new MyObject() + new MyObject();
        } catch ( e ) {
            result = "passed:  threw exception",
            exception = e.toString();
        } finally {
            testcases[tc++] = new TestCase(
                SECTION,
                "new MyObject() + new MyObject() [ exception is " + exception +" ]",
                "passed:  threw exception",
                result );
        }
    }

