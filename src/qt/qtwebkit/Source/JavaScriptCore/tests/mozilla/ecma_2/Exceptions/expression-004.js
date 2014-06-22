/**
    File Name:          expression-004.js
    Corresponds To:     11.2.1-4-n.js
    ECMA Section:       11.2.1 Property Accessors
    Description:

    Author:             christine@netscape.com
    Date:               09 september 1998
*/
    var SECTION = "expression-004";
    var VERSION = "JS1_4";
    var TITLE   = "Property Accessors";
    writeHeaderToLog( SECTION + " "+TITLE );
    startTest();

    var tc = 0;
    var testcases = new Array();

    var OBJECT = new Property( "null", null, "null", 0 );

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
        "Get the toString value of an object whose value is null "+
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
