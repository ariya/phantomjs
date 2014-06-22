/**
    File Name:          expression-015.js
    Corresponds To:     ecma/Expressions/11.2.3-2-n.js
    ECMA Section:       11.2.3. Function Calls
    Description:
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-015";
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
        eval("result = 3.valueOf();");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "3.valueOf()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
