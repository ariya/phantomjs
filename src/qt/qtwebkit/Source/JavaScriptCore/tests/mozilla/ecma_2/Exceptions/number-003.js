/**
    File Name:          number-003.js
    Corresponds To:     15.7.4.3-3.js
    ECMA Section:       15.7.4.3.1 Number.prototype.valueOf()
    Description:
    Returns this number value.

    The valueOf function is not generic; it generates a runtime error if its
    this value is not a Number object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               16 september 1997
*/
    var SECTION = "number-003";
    var VERSION = "JS1_4";
    var TITLE   = "Exceptions for Number.valueOf()";

    var tc = 0;
    var testcases = new Array();

    startTest();
    writeHeaderToLog( SECTION + " Number.prototype.valueOf()");

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        VALUE_OF = Number.prototype.valueOf;
        OBJECT = new String("Infinity");
        OBJECT.valueOf = VALUE_OF;
        result = OBJECT.valueOf();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "Assigning Number.prototype.valueOf as the valueOf of a String object " +
        " (threw " + exception +")",
        expect,
        result );

    test();

