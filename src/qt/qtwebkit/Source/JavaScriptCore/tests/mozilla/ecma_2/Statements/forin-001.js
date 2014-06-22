/**
 *  File Name:          forin-001.js
 *  ECMA Section:
 *  Description:        The forin-001 statement
 *
 *  Verify that the property name is assigned to the property on the left
 *  hand side of the for...in expression.
 *
 *  Author:             christine@netscape.com
 *  Date:               28 August 1998
 */
    var SECTION = "forin-001";
    var VERSION = "ECMA_2";
    var TITLE   = "The for...in  statement";
    var BUGNUMBER="330890";
    var BUGNUMBER="http://scopus.mcom.com/bugsplat/show_bug.cgi?id=344855";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    ForIn_1( { length:4, company:"netscape", year:2000, 0:"zero" } );
    ForIn_2( { length:4, company:"netscape", year:2000, 0:"zero" } );
    ForIn_3( { length:4, company:"netscape", year:2000, 0:"zero" } );

//    ForIn_6({ length:4, company:"netscape", year:2000, 0:"zero" });
//    ForIn_7({ length:4, company:"netscape", year:2000, 0:"zero" });
    ForIn_8({ length:4, company:"netscape", year:2000, 0:"zero" });

    test();

    /**
     *  Verify that the left side argument is evaluated with every iteration.
     *  Verify that the name of each property of the object is assigned to a
     *  a property.
     *
     */
    function ForIn_1( object ) {
        PropertyArray = new Array();
        ValueArray = new Array();

        for ( PropertyArray[PropertyArray.length] in object ) {
            ValueArray[ValueArray.length] =
                object[PropertyArray[PropertyArray.length-1]];
        }

        for ( var i = 0; i < PropertyArray.length; i++ ) {
            testcases[tc++] = new TestCase(
                SECTION,
                "object[" + PropertyArray[i] +"]",
                object[PropertyArray[i]],
                ValueArray[i]
            );
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "object.length",
            PropertyArray.length,
            object.length );
    }

    /**
     *  Similar to ForIn_1, except it should increment the counter variable
     *  every time the left hand expression is evaluated.
     */
    function ForIn_2( object ) {
        PropertyArray = new Array();
        ValueArray = new Array();
        var i = 0;

        for ( PropertyArray[i++] in object ) {
            ValueArray[ValueArray.length] =
                object[PropertyArray[PropertyArray.length-1]];
        }

        for ( i = 0; i < PropertyArray.length; i++ ) {
            testcases[tc++] = new TestCase(
                SECTION,
                "object[" + PropertyArray[i] +"]",
                object[PropertyArray[i]],
                ValueArray[i]
            );
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "object.length",
            PropertyArray.length,
            object.length );
    }

    /**
     *  Break out of a for...in loop
     *
     *
     */
    function ForIn_3( object ) {
        var checkBreak = "pass";
        var properties = new Array();
        var values = new Array();

        for ( properties[properties.length] in object ) {
            values[values.length] = object[properties[properties.length-1]];
            break;
            checkBreak = "fail";
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "check break out of for...in",
            "pass",
            checkBreak );

        testcases[tc++] = new TestCase(
            SECTION,
            "properties.length",
            1,
            properties.length );

        testcases[tc++] = new TestCase(
            SECTION,
            "object["+properties[0]+"]",
            values[0],
            object[properties[0]] );
    }

    /**
     *  Break out of a labeled for...in loop.
     */
    function ForIn_4( object ) {
        var result1 = 0;
        var result2 = 0;
        var result3 = 0;
        var result4 = 0;
        var i = 0;
        var property = new Array();

        butterbean: {
            result1++;

            for ( property[i++] in object ) {
                result2++;
                break;
                result4++;
            }
            result3++;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "verify labeled statement is only executed once",
            true,
            result1 == 1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify statements in for loop are evaluated",
            true,
            result2 == i );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled for...in loop",
            true,
            result4 == 0 );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled block",
            true,
            result3 == 0 );
    }

    /**
     *  Labeled break out of a labeled for...in loop.
     */
    function ForIn_5 (object) {
        var result1 = 0;
        var result2 = 0;
        var result3 = 0;
        var result4 = 0;
        var i = 0;
        var property = new Array();

        bigredbird: {
            result1++;
            for ( property[i++] in object ) {
                result2++;
                break bigredbird;
                result4++;
            }
            result3++;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "verify labeled statement is only executed once",
            true,
            result1 == 1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify statements in for loop are evaluated",
            true,
            result2 == i );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled for...in loop",
            true,
            result4 == 0 );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled block",
            true,
            result3 == 0 );
    }

    /**
     *  Labeled continue from a labeled for...in loop
     */
    function ForIn_7( object ) {
        var result1 = 0;
        var result2 = 0;
        var result3 = 0;
        var result4 = 0;
        var i = 0;
        var property = new Array();

        bigredbird:
            for ( property[i++] in object ) {
                result2++;
                continue bigredbird;
                result4++;
            }

        testcases[tc++] = new TestCase(
            SECTION,
            "verify statements in for loop are evaluated",
            true,
            result2 == i );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled for...in loop",
            true,
            result4 == 0 );

        testcases[tc++] = new TestCase(
            SECTION,
            "verify break out of labeled block",
            true,
            result3 == 1 );
    }


    /**
     *  continue in a for...in loop
     *
     */
    function ForIn_8( object ) {
        var checkBreak = "pass";
        var properties = new Array();
        var values = new Array();

        for ( properties[properties.length] in object ) {
            values[values.length] = object[properties[properties.length-1]];
            break;
            checkBreak = "fail";
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "check break out of for...in",
            "pass",
            checkBreak );

        testcases[tc++] = new TestCase(
            SECTION,
            "properties.length",
            1,
            properties.length );

        testcases[tc++] = new TestCase(
            SECTION,
            "object["+properties[0]+"]",
            values[0],
            object[properties[0]] );
    }

