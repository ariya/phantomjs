/**
    File Name:          expression-008
    Corresponds To:     11.2.2-3-n.js
    ECMA Section:       11.2.2. The new operator
    Description:
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-008";
   var VERSION = "JS1_4";
    var TITLE   = "The new operator";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var NULL = null;
    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = new NULL();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "NULL = null; result = new NULL()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
