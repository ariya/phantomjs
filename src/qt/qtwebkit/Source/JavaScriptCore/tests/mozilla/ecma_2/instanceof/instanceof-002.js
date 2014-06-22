/**
    File Name:
    ECMA Section:
    Description:        Call Objects



    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "";
    var VERSION = "ECMA_2";
    var TITLE   = "The Call Constructor";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var b = new Boolean();

    testcases[tc++] = new TestCase( SECTION,
                                    "var b = new Boolean(); b instanceof Boolean",
                                    true,
                                    b instanceof Boolean );

    testcases[tc++] = new TestCase( SECTION,
                                    "b instanceof Object",
                                    true,
                                    b instanceof Object );

    testcases[tc++] = new TestCase( SECTION,
                                    "b instanceof Array",
                                    false,
                                    b instanceof Array );

    testcases[tc++] = new TestCase( SECTION,
                                    "true instanceof Boolean",
                                    false,
                                    true instanceof Boolean );

    testcases[tc++] = new TestCase( SECTION,
                                    "Boolean instanceof Object",
                                    true,
                                    Boolean instanceof Object );
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
