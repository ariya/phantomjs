/**
    File Name:          expression-07.js
    Corresponds To:     ecma/Expressions/11.2.3-4-n.js
    ECMA Section:       11.2.3. Function Calls
    Description:
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-017";
    var VERSION = "JS1_4";
    var TITLE   = "Function Calls";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = nullvalueOf();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "null.valueOf()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
