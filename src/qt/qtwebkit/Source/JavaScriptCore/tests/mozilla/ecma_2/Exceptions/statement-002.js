/**
    File Name:          statement-002.js
    Corresponds To:     12.6.3-1.js
    ECMA Section:       12.6.3 The for...in Statement
    Description:
    The production IterationStatement : for ( LeftHandSideExpression in Expression )
    Statement is evaluated as follows:

    1.  Evaluate the Expression.
    2.  Call GetValue(Result(1)).
    3.  Call ToObject(Result(2)).
    4.  Let C be "normal completion".
    5.  Get the name of the next property of Result(3) that doesn't have the
        DontEnum attribute. If there is no such property, go to step 14.
    6.  Evaluate the LeftHandSideExpression ( it may be evaluated repeatedly).
    7.  Call PutValue(Result(6), Result(5)).  PutValue( V, W ):
        1.  If Type(V) is not Reference, generate a runtime error.
        2.  Call GetBase(V).
        3.  If Result(2) is null, go to step 6.
        4.  Call the [[Put]] method of Result(2), passing GetPropertyName(V)
            for the property name and W for the value.
        5.  Return.
        6.  Call the [[Put]] method for the global object, passing
            GetPropertyName(V) for the property name and W for the value.
        7.  Return.
    8.  Evaluate Statement.
    9.  If Result(8) is a value completion, change C to be "normal completion
        after value V" where V is the value carried by Result(8).
    10. If Result(8) is a break completion, go to step 14.
    11. If Result(8) is a continue completion, go to step 5.
    12. If Result(8) is a return completion, return Result(8).
    13. Go to step 5.
    14. Return C.

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "statement-002";
    var VERSION = "JS1_4";
    var TITLE   = "The for..in statment";

    var testcases = new Array();
    var tc = 0;

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval(" for ( var i, p in this) { result += this[p]; }");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "more than one member expression" +
        " (threw " + exception +")",
        expect,
        result );

    test();
