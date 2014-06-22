/**
 *  File Name:          instanceof-001.js
 *  ECMA Section:       11.8.6
 *  Description:
 *
 *  RelationalExpression instanceof Identifier
 *
 *  Author:             christine@netscape.com
 *  Date:               2 September 1998
 */
    var SECTION = "instanceof-001";
    var VERSION = "ECMA_2";
    var TITLE   = "instanceof"

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    function InstanceOf( object_1, object_2, expect ) {
        result = object_1 instanceof object_2;

        testcases[tc++] = new TestCase(
            SECTION,
            "(" + object_1 + ") instanceof " + object_2,
            expect,
            result );
    }

    function Gen3(value) {
        this.value = value;
        this.generation = 3;
        this.toString = new Function ( "return \"(Gen\"+this.generation+\" instance)\"" );
    }
    Gen3.name = 3;
    Gen3.__proto__.toString = new Function( "return \"(\"+this.name+\" object)\"");

    function Gen2(value) {
        this.value = value;
        this.generation = 2;
    }
    Gen2.name = 2;
    Gen2.prototype = new Gen3();

    function Gen1(value) {
        this.value = value;
        this.generation = 1;
    }
    Gen1.name = 1;
    Gen1.prototype = new Gen2();

    function Gen0(value) {
        this.value = value;
        this.generation = 0;
    }
    Gen0.name = 0;
    Gen0.prototype = new Gen1();


    function GenA(value) {
        this.value = value;
        this.generation = "A";
        this.toString = new Function ( "return \"(instance of Gen\"+this.generation+\")\"" );

    }
    GenA.prototype = new Gen0();
    GenA.name = "A";

    function GenB(value) {
        this.value = value;
        this.generation = "B";
        this.toString = new Function ( "return \"(instance of Gen\"+this.generation+\")\"" );
    }
    GenB.name = "B"
    GenB.prototype = void 0;

    // RelationalExpression is not an object.

    InstanceOf( true, Boolean, false );
    InstanceOf( new Boolean(false), Boolean, true );

    // Identifier is not a function

    InstanceOf( new Boolean(true), false, false );

    // Identifier is a function, prototype of Identifier is not an object

//    InstanceOf( new GenB(), GenB, false );


    test();