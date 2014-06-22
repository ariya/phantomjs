/**
    File Name:          expression-007.js
    Corresponds To:     11.2.2-2-n.js
    ECMA Section:       11.2.2. The new operator
    Description:


    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-007";
    var VERSION = "JS1_4";
    var TITLE   = "The new operator";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        UNDEFINED = void 0;
        result = new UNDEFINED();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "UNDEFINED = void 0; result = new UNDEFINED()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

