/**
    File Name:          expression-009
    Corresponds to:     ecma/Expressions/11.2.2-4-n.js
    ECMA Section:       11.2.2. The new operator
    Description:
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-009";
    var VERSION = "JS1_4";
    var TITLE   = "The new operator";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var STRING = "";

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = new STRING();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "STRING = ''; result = new STRING()" +
        " (threw " + exception +")",
        expect,
        result );

    test();