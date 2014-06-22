/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          15.5.4.6-1.js
    ECMA Section:       15.5.4.6 String.prototype.indexOf( searchString, pos)
    Description:        If the given searchString appears as a substring of the
                        result of converting this object to a string, at one or
                        more positions that are at or to the right of the
                        specified position, then the index of the leftmost such
                        position is returned; otherwise -1 is returned.  If
                        positionis undefined or not supplied, 0 is assumed, so
                        as to search all of the string.

                        When the indexOf method is called with two arguments,
                        searchString and pos, the following steps are taken:

                        1. Call ToString, giving it the this value as its
                           argument.
                        2. Call ToString(searchString).
                        3. Call ToInteger(position). (If position is undefined
                           or not supplied, this step produces the value 0).
                        4. Compute the number of characters in Result(1).
                        5. Compute min(max(Result(3), 0), Result(4)).
                        6. Compute the number of characters in the string that
                           is Result(2).
                        7. Compute the smallest possible integer k not smaller
                           than Result(5) such that k+Result(6) is not greater
                           than Result(4), and for all nonnegative integers j
                           less than Result(6), the character at position k+j
                           of Result(1) is the same as the character at position
                           j of Result(2); but if there is no such integer k,
                           then compute the value -1.
                        8. Return Result(7).

                        Note that the indexOf function is intentionally generic;
                        it does not require that its this value be a String object.
                        Therefore it can be transferred to other kinds of objects
                        for use as a method.

    Author:             christine@netscape.com, pschwartau@netscape.com
    Date:               02 October 1997
    Modified:           14 July 2002
    Reason:             See http://bugzilla.mozilla.org/show_bug.cgi?id=155289
                        ECMA-262 Ed.3  Section 15.5.4.7
                        The length property of the indexOf method is 1
*
*/
    var SECTION = "15.5.4.6-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.protoype.indexOf";
    var BUGNUMBER="105721";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


// the following test regresses http://scopus/bugsplat/show_bug.cgi?id=105721

function f() {
    return this;
}
function g() {
    var h = f;
    return h();
}

function MyObject (v) {
    this.value      = v;
    this.toString   = new Function ( "return this.value +\"\"");
    this.indexOf     = String.prototype.indexOf;
}

function getTestCases() {
    var array = new Array();
    var item = 0;

    // regress http://scopus/bugsplat/show_bug.cgi?id=105721

    array[item++] = new TestCase( SECTION, "function f() { return this; }; function g() { var h = f; return h(); }; g().toString()",    GLOBAL,  g().toString() );


    array[item++] = new TestCase( SECTION, "String.prototype.indexOf.length",                                               1,     String.prototype.indexOf.length );
    array[item++] = new TestCase( SECTION, "String.prototype.indexOf.length = null; String.prototype.indexOf.length",       1,     eval("String.prototype.indexOf.length = null; String.prototype.indexOf.length") );
    array[item++] = new TestCase( SECTION, "delete String.prototype.indexOf.length",                                        false,  delete String.prototype.indexOf.length );
    array[item++] = new TestCase( SECTION, "delete String.prototype.indexOf.length; String.prototype.indexOf.length",       1,      eval("delete String.prototype.indexOf.length; String.prototype.indexOf.length") );

    array[item++] = new TestCase( SECTION, "var s = new String(); s.indexOf()",     -1,     eval("var s = new String(); s.indexOf()") );

    // some Unicode tests.

    // generate a test string.

    var TEST_STRING = "";

    for ( var u = 0x00A1; u <= 0x00FF; u++ ) {
        TEST_STRING += String.fromCharCode( u );
    }

    for ( var u = 0x00A1, i = 0; u <= 0x00FF; u++, i++ ) {
        array[item++] = new TestCase(   SECTION,
                                        "TEST_STRING.indexOf( " + String.fromCharCode(u) + " )",
                                        i,
                                        TEST_STRING.indexOf( String.fromCharCode(u) ) );
    }
    for ( var u = 0x00A1, i = 0; u <= 0x00FF; u++, i++ ) {
        array[item++] = new TestCase(   SECTION,
                                        "TEST_STRING.indexOf( " + String.fromCharCode(u) + ", void 0 )",
                                        i,
                                        TEST_STRING.indexOf( String.fromCharCode(u), void 0 ) );
    }



    var foo = new MyObject('hello');

    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('h')", 0, foo.indexOf("h")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('e')", 1, foo.indexOf("e")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('l')", 2, foo.indexOf("l")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('l')", 2, foo.indexOf("l")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('o')", 4, foo.indexOf("o")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf('X')", -1,  foo.indexOf("X")  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello');foo.indexOf(5) ", -1,  foo.indexOf(5)  );

    var boo = new MyObject(true);

    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('t')", 0, boo.indexOf("t")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('r')", 1, boo.indexOf("r")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('u')", 2, boo.indexOf("u")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('e')", 3, boo.indexOf("e")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('true')", 0, boo.indexOf("true")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('rue')", 1, boo.indexOf("rue")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('ue')", 2, boo.indexOf("ue")  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true);boo.indexOf('oy')", -1, boo.indexOf("oy")  );


    var noo = new MyObject( Math.PI );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('3') ", 0, noo.indexOf('3')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('.') ", 1, noo.indexOf('.')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('1') ", 2, noo.indexOf('1')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('4') ", 3, noo.indexOf('4')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('1') ", 2, noo.indexOf('1')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('5') ", 5, noo.indexOf('5')  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); noo.indexOf('9') ", 6, noo.indexOf('9')  );

    array[item++] = new TestCase( SECTION,
                                  "var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf('new')",
                                  0,
                                  eval("var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf('new')") );

    array[item++] = new TestCase( SECTION,
                                  "var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf(',zoo,')",
                                  3,
                                  eval("var arr = new Array('new','zoo','revue'); arr.indexOf = String.prototype.indexOf; arr.indexOf(',zoo,')") );

    array[item++] = new TestCase( SECTION,
                                  "var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('[object Object]')",
                                  0,
                                  eval("var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('[object Object]')") );

    array[item++] = new TestCase( SECTION,
                                  "var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('bject')",
                                  2,
                                  eval("var obj = new Object(); obj.indexOf = String.prototype.indexOf; obj.indexOf('bject')") );

// This correctly throws, due to ES5 15.5.4.7 step 1
//    array[item++] = new TestCase( SECTION,
//                                  "var f = new Object( String.prototype.indexOf ); f('"+GLOBAL+"')",
//                                  0,
//                                  eval("var f = new Object( String.prototype.indexOf ); f('"+GLOBAL+"')") );

    array[item++] = new TestCase( SECTION,
                                  "var f = new Function(); f.toString = Object.prototype.toString; f.indexOf = String.prototype.indexOf; f.indexOf('[object Function]')",
                                   0,
                                   eval("var f = new Function(); f.toString = Object.prototype.toString; f.indexOf = String.prototype.indexOf; f.indexOf('[object Function]')") );

    array[item++] = new TestCase( SECTION,
                                  "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('true')",
                                  -1,
                                  eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('true')") );

    array[item++] = new TestCase( SECTION,
                                  "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 1)",
                                  -1,
                                  eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 1)") );

    array[item++] = new TestCase( SECTION,
                                  "var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 0)",
                                  0,
                                  eval("var b = new Boolean(); b.indexOf = String.prototype.indexOf; b.indexOf('false', 0)") );

    array[item++] = new TestCase( SECTION,
                                  "var n = new Number(1e21); n.indexOf = String.prototype.indexOf; n.indexOf('e')",
                                  1,
                                  eval("var n = new Number(1e21); n.indexOf = String.prototype.indexOf; n.indexOf('e')") );

    array[item++] = new TestCase( SECTION,
                                  "var n = new Number(-Infinity); n.indexOf = String.prototype.indexOf; n.indexOf('-')",
                                  0,
                                  eval("var n = new Number(-Infinity); n.indexOf = String.prototype.indexOf; n.indexOf('-')") );

    array[item++] = new TestCase( SECTION,
                                  "var n = new Number(0xFF); n.indexOf = String.prototype.indexOf; n.indexOf('5')",
                                  1,
                                  eval("var n = new Number(0xFF); n.indexOf = String.prototype.indexOf; n.indexOf('5')") );

    array[item++] = new TestCase( SECTION,
                                  "var m = Math; m.indexOf = String.prototype.indexOf; m.indexOf( 'Math' )",
                                  8,
                                  eval("var m = Math; m.indexOf = String.prototype.indexOf; m.indexOf( 'Math' )") );

    // new Date(0) has '31' or '01' at index 8 depending on whether tester is (GMT-) or (GMT+), respectively
    array[item++] = new TestCase( SECTION,
                                  "var d = new Date(0); d.indexOf = String.prototype.indexOf; d.getTimezoneOffset()>0 ? d.indexOf('31') : d.indexOf('01')",
                                  8,
                                  eval("var d = new Date(0); d.indexOf = String.prototype.indexOf; d.getTimezoneOffset()>0 ? d.indexOf('31') : d.indexOf('01')") );


    return array;
}

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {

            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+
                    testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed )
                                    ? ""
                                    : "wrong value "
        }
        stopTest();

    //  all tests must return a boolean value
        return ( testcases );
}
