/**
    File Name:          global-001
    Corresponds To:     ecma/GlobalObject/15.1-1-n.js
    ECMA Section:       The global object
    Description:

    The global object does not have a [[Construct]] property; it is not
    possible to use the global object as a constructor with the new operator.


    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "global-001";
    var VERSION = "ECMA_1";
    var TITLE   = "The Global Object";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();


    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = new this();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "result = new this()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
