/**
    File Name:          statement-007.js
    Corresponds To:     12.7-1-n.js
    ECMA Section:       12.7 The continue statement
    Description:

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "statement-007";
    var VERSION = "JS1_4";
    var TITLE   = "The continue statment";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval("continue;");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "continue outside of an iteration statement" +
        " (threw " + exception +")",
        expect,
        result );

    test();

