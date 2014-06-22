/**
    File Name:          expression-013.js
    Corresponds To:     ecma/Expressions/11.2.2-8-n.js
    ECMA Section:       11.2.2. The new operator
    Description:
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-013";
    var VERSION = "JS1_4";
    var TITLE   = "The new operator";
    var BUGNUMBER= "327765";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var NUMBER = new Number(1);

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = new NUMBER();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "NUMBER = new Number(1); result = new NUMBER()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

