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
    File Name:          15.4.4.3-1.js
    ECMA Section:       15.4.4.3-1 Array.prototype.reverse()
    Description:

    The elements of the array are rearranged so as to reverse their order.
    This object is returned as the result of the call.

   1.   Call the [[Get]] method of this object with argument "length".
   2.   Call ToUint32(Result(1)).
   3.   Compute floor(Result(2)/2).
   4.   Let k be 0.
   5.   If k equals Result(3), return this object.
   6.   Compute Result(2)k1.
   7.   Call ToString(k).
   8.   ToString(Result(6)).
   9.   Call the [[Get]] method of this object with argument Result(7).
  10.   Call the [[Get]] method of this object with argument Result(8).
  11.   If this object has a property named by Result(8), go to step 12; but
        if this object has no property named by Result(8), then go to either
        step 12 or step 14, depending on the implementation.
  12.   Call the [[Put]] method of this object with arguments Result(7) and
        Result(10).
  13.   Go to step 15.
  14.   Call the [[Delete]] method on this object, providing Result(7) as the
        name of the property to delete.
  15.   If this object has a property named by Result(7), go to step 16; but if
        this object has no property named by Result(7), then go to either step 16
        or step 18, depending on the implementation.
  16.   Call the [[Put]] method of this object with arguments Result(8) and
        Result(9).
  17.   Go to step 19.
  18.   Call the [[Delete]] method on this object, providing Result(8) as the
        name of the property to delete.
  19.   Increase k by 1.
  20.   Go to step 5.

Note that the reverse function is intentionally generic; it does not require
that its this value be an Array object. Therefore it can be transferred to other
kinds of objects for use as a method. Whether the reverse function can be applied
successfully to a host object is implementation dependent.

    Note:   Array.prototype.reverse allows some flexibility in implementation
    regarding array indices that have not been populated. This test covers the
    cases in which unpopulated indices are not deleted, since the JavaScript
    implementation does not delete uninitialzed indices.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/
    var SECTION = "15.4.4.4-1";
    var VERSION = "ECMA_1";
    startTest();
    var BUGNUMBER="123724";

    writeHeaderToLog( SECTION + " Array.prototype.reverse()");

    var testcases = new Array();
    getTestCases();
    test();

function getTestCases() {
    var ARR_PROTOTYPE = Array.prototype;

    testcases[testcases.length] = new TestCase( SECTION, "Array.prototype.reverse.length",           0,      Array.prototype.reverse.length );
    testcases[testcases.length] = new TestCase( SECTION, "delete Array.prototype.reverse.length",    false,  delete Array.prototype.reverse.length );
    testcases[testcases.length] = new TestCase( SECTION, "delete Array.prototype.reverse.length; Array.prototype.reverse.length",    0, eval("delete Array.prototype.reverse.length; Array.prototype.reverse.length") );

    // length of array is 0
    testcases[testcases.length] = new TestCase(   SECTION,
                                    "var A = new Array();   A.reverse(); A.length",
                                    0,
                                    eval("var A = new Array();   A.reverse(); A.length") );

    // length of array is 1
    var A = new Array(true);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    "var A = new Array(true);   A.reverse(); A.length",
                                    R.length,
                                    eval("var A = new Array(true);   A.reverse(); A.length") );
    CheckItems( R, A );

    // length of array is 2
    var S = "var A = new Array( true,false )";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] =   new TestCase(
                                    SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );

    CheckItems(  R, A );

    // length of array is 3
    var S = "var A = new Array( true,false,null )";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );

    // length of array is 4
    var S = "var A = new Array( true,false,null,void 0 )";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );


    // some array indexes have not been set
    var S = "var A = new Array(); A[8] = 'hi', A[3] = 'yo'";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );


    var OBJECT_OBJECT = new Object();
    var FUNCTION_OBJECT = new Function( 'return this' );
    var BOOLEAN_OBJECT = new Boolean;
    var DATE_OBJECT = new Date(0);
    var STRING_OBJECT = new String('howdy');
    var NUMBER_OBJECT = new Number(Math.PI);
    var ARRAY_OBJECT= new Array(1000);

     var args = "null, void 0, Math.pow(2,32), 1.234e-32, OBJECT_OBJECT, BOOLEAN_OBJECT, FUNCTION_OBJECT, DATE_OBJECT, STRING_OBJECT,"+
                "ARRAY_OBJECT, NUMBER_OBJECT, Math, true, false, 123, '90210'";

    var S = "var A = new Array("+args+")";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );

    var limit = 1000;
    var args = "";
    for (var i = 0; i < limit; i++ ) {
        args += i +"";
        if ( i + 1 < limit ) {
            args += ",";
        }
    }

    var S = "var A = new Array("+args+")";
    eval(S);
    var R = Reverse(A);

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );

    var S = "var MYOBJECT = new Object_1( \"void 0, 1, null, 2, \'\'\" )";
    eval(S);
    var R = Reverse( A );

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.reverse(); A.length",
                                    R.length,
                                    eval( S + "; A.reverse(); A.length") );
    CheckItems( R, A );

    return ( testcases );
}
function CheckItems( R, A ) {
    for ( var i = 0; i < R.length; i++ ) {
        testcases[testcases.length] = new TestCase(
                                            SECTION,
                                            "A["+i+ "]",
                                            R[i],
                                            A[i] );
    }
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function Object_1( value ) {
    this.array = value.split(",");
    this.length = this.array.length;
    for ( var i = 0; i < this.length; i++ ) {
        this[i] = eval(this.array[i]);
    }
    this.join = Array.prototype.reverse;
    this.getClass = Object.prototype.toString;
}
function Reverse( array ) {
    var r2 = array.length;
    var k = 0;
    var r3 = Math.floor( r2/2 );
    if ( r3 == k ) {
        return array;
    }

    for ( k = 0;  k < r3; k++ ) {
        var r6 = r2 - k - 1;
//        var r7 = String( k );
        var r7 = k;
        var r8 = String( r6 );

        var r9 = array[r7];
        var r10 = array[r8];

        array[r7] = r10;
        array[r8] = r9;
    }

    return array;
}
function Iterate( array ) {
    for ( var i = 0; i < array.length; i++ ) {
//        print( i+": "+ array[String(i)] );
    }
}

function Object_1( value ) {
    this.array = value.split(",");
    this.length = this.array.length;
    for ( var i = 0; i < this.length; i++ ) {
        this[i] = this.array[i];
    }
    this.reverse = Array.prototype.reverse;
    this.getClass = Object.prototype.toString;
}
