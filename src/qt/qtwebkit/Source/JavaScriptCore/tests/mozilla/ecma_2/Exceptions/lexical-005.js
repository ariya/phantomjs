/**
    File Name:          lexical-005.js
    Corresponds To:     7.4.1-2.js
    ECMA Section:       7.4.1

    Description:

    Reserved words cannot be used as identifiers.

    ReservedWord ::
    Keyword
    FutureReservedWord
    NullLiteral
    BooleanLiteral

    Author:             christine@netscape.com
    Date:               12 november 1997

*/
    var SECTION = "lexical-005";
    var VERSION = "JS1_4";
    var TITLE   = "Keywords";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();


    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval("true = false;");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "true = false" +
        " (threw " + exception +")",
        expect,
        result );

    test();

