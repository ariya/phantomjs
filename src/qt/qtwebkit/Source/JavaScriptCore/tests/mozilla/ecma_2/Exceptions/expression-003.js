/**
    File Name:          expressions-003.js
    Corresponds to:     ecma/Expressions/11.2.1-3-n.js
    ECMA Section:       11.2.1 Property Accessors
    Description:

    Try to access properties of an object whose value is undefined.

    Author:             christine@netscape.com
    Date:               09 september 1998
*/
    var SECTION = "expressions-003.js";
    var VERSION = "JS1_4";
    var TITLE   = "Property Accessors";
    writeHeaderToLog( SECTION + " "+TITLE );

    startTest();

    var tc = 0;
    var testcases = new Array();

    // try to access properties of primitive types

    OBJECT = new Property(  "undefined",    void 0,   "undefined",   NaN );

    var result    = "Failed";
    var exception = "No exception thrown";
    var expect    = "Passed";

    try {
        result = OBJECT.value.toString();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }


    testcases[tc++] = new TestCase(
        SECTION,
        "Get the toString value of an object whose value is undefined "+
        "(threw " + exception +")",
        expect,
        result );

    test();

function Property( object, value, string, number ) {
    this.object = object;
    this.string = String(value);
    this.number = Number(value);
    this.value = value;
}