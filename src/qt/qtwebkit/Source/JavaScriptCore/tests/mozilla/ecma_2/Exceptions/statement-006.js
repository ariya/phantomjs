/**
    File Name:          statement-006.js
    Corresponds To:     12.6.3-9-n.js
    ECMA Section:       12.6.3 The for...in Statement
    Description:

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "statement-006";
    var VERSION = "JS1_4";
    var TITLE   = "The for..in statment";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var o = new MyObject();
        var result = 0;
        for ( var o in foo) {
            result += this[o];
        }
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "object is not defined" +
        " (threw " + exception +")",
        expect,
        result );

    test();

function MyObject() {
    this.value = 2;
    this[0] = 4;
    return this;
}