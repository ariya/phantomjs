/**
    File Name:          expression-001.js
    Corresponds to:     ecma/Expressions/11.12-2-n.js
    ECMA Section:       11.12
    Description:

    The grammar for a ConditionalExpression in ECMAScript is a little bit
    different from that in C and Java, which each allow the second
    subexpression to be an Expression but restrict the third expression to
    be a ConditionalExpression.  The motivation for this difference in
    ECMAScript is to allow an assignment expression to be governed by either
    arm of a conditional and to eliminate the confusing and fairly useless
    case of a comma expression as the center expression.

    Author:             christine@netscape.com
    Date:               09 september 1998
*/
    var SECTION = "expression-001";
    var VERSION = "JS1_4";
    var TITLE   = "Conditional operator ( ? : )"
    startTest();
    writeHeaderToLog( SECTION + " " + TITLE );

    var tc = 0;
    var testcases = new Array();

    // the following expression should be an error in JS.

    var result = "Failed"
    var exception = "No exception was thrown";

    try {
        eval("var MY_VAR = true ? \"EXPR1\", \"EXPR2\" : \"EXPR3\"");
    } catch ( e ) {
        result = "Passed";
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "comma expression in a conditional statement "+
        "(threw "+ exception +")",
        "Passed",
        result );


    test();
