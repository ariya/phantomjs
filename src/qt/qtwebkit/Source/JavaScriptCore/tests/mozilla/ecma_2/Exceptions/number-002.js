/**
    File Name:          number-002.js
    Corresponds To:     ecma/Number/15.7.4.3-2-n.js
    ECMA Section:       15.7.4.3.1 Number.prototype.valueOf()
    Description:
    Returns this number value.

    The valueOf function is not generic; it generates a runtime error if its
    this value is not a Number object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "number-002";
    var VERSION = "JS1_4";
    var TITLE   = "Exceptions for Number.valueOf()";

    startTest();
    writeHeaderToLog( SECTION + " Number.prototype.valueOf()");

    var testcases = new Array();
    var tc = 0;

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        object= new Object();
        object.toString = Number.prototype.valueOf;
        result = object.toString();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "object = new Object(); object.valueOf = Number.prototype.valueOf; object.valueOf()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
