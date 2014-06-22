/**
    File Name:          expression-012.js
    Corresponds To:     ecma/Expressions/11.2.2-6-n.js
    ECMA Section:       11.2.2. The new operator
    Description:
    http://scopus/bugsplat/show_bug.cgi?id=327765
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "expression-012";
    var VERSION = "JS1_4";
    var TITLE   = "The new operator";
    var BUGNUMBER= "327765";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var STRING = new String("hi");
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
        "STRING = new String(\"hi\"); result = new STRING()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

