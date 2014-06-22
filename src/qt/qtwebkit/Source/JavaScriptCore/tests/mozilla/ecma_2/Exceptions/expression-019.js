/**
    File Name:          expression-019.js
    Corresponds To:     11.2.2-7-n.js
    ECMA Section:       11.2.2. The new operator
    Description:

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-019";
    var VERSION = "JS1_4";
    var TITLE   = "The new operator";
    var BUGNUMBER= "327765";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var STRING = new String("hi");
        result = new STRING();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "var STRING = new String(\"hi\"); result = new STRING();" +
        " (threw " + exception + ")",
        expect,
        result );

    test();

