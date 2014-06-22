/**
    File Name:          date-002.js
    Corresponds To:     15.9.5.23-3-n.js
    ECMA Section:       15.9.5.23
    Description:        Date.prototype.setTime

    1.  If the this value is not a Date object, generate a runtime error.
    2.  Call ToNumber(time).
    3.  Call TimeClip(Result(1)).
    4.  Set the [[Value]] property of the this value to Result(2).
    5.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "date-002";
    var VERSION = "JS1_4";
    var TITLE   = "Date.prototype.setTime()";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var MYDATE = new MyDate();
        result = MYDATE.setTime(0);
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "MYDATE = new MyDate(); MYDATE.setTime(0)" +
        " (threw " + exception +")",
        expect,
        result );

    test();

function MyDate(value) {
    this.value = value;
    this.setTime = Date.prototype.setTime;
    return this;
}
