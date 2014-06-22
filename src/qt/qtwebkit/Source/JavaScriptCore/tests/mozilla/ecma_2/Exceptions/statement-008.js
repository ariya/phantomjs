/**
    File Name:          statement-008.js
    Corresponds To:     12.8-1-n.js
    ECMA Section:       12.8 The break statement
    Description:

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "statement-008";
    var VERSION = "JS1_4";
    var TITLE   = "The break in statment";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval("break;");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "break outside of an iteration statement" +
        " (threw " + exception +")",
        expect,
        result );

    test();

