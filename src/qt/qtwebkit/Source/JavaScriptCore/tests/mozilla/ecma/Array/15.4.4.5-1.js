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
    File Name:          15.4.4.5.js
    ECMA Section:       Array.prototype.sort(comparefn)
    Description:

    This test file tests cases in which the compare function is not supplied.

    The elements of this array are sorted. The sort is not necessarily stable.
    If comparefn is provided, it should be a function that accepts two arguments
    x and y and returns a negative value if x < y, zero if x = y, or a positive
    value if x > y.

   1.   Call the [[Get]] method of this object with argument "length".
   2.   Call ToUint32(Result(1)).
        1.  Perform an implementation-dependent sequence of calls to the
        [[Get]] , [[Put]], and [[Delete]] methods of this object and
        toSortCompare (described below), where the first argument for each call
            to [[Get]], [[Put]] , or [[Delete]] is a nonnegative integer less
            than Result(2) and where the arguments for calls to SortCompare are
            results of previous calls to the [[Get]] method. After this sequence
            is complete, this object must have the following two properties.
            (1) There must be some mathematical permutation of the nonnegative
            integers less than Result(2), such that for every nonnegative integer
            j less than Result(2), if property old[j] existed, then new[(j)] is
            exactly the same value as old[j],. but if property old[j] did not exist,
            then new[(j)] either does not exist or exists with value undefined.
            (2) If comparefn is not supplied or is a consistent comparison
            function for the elements of this array, then for all nonnegative
            integers j and k, each less than Result(2), if old[j] compares less
            than old[k] (see SortCompare below), then (j) < (k). Here we use the
            notation old[j] to refer to the hypothetical result of calling the [
            [Get]] method of this object with argument j before this step is
            executed, and the notation new[j] to refer to the hypothetical result
            of calling the [[Get]] method of this object with argument j after this
            step has been completely executed. A function is a consistent
            comparison function for a set of values if (a) for any two of those
            values (possibly the same value) considered as an ordered pair, it
            always returns the same value when given that pair of values as its
            two arguments, and the result of applying ToNumber to this value is
            not NaN; (b) when considered as a relation, where the pair (x, y) is
            considered to be in the relation if and only if applying the function
            to x and y and then applying ToNumber to the result produces a
            negative value, this relation is a partial order; and (c) when
            considered as a different relation, where the pair (x, y) is considered
            to be in the relation if and only if applying the function to x and y
            and then applying ToNumber to the result produces a zero value (of either
            sign), this relation is an equivalence relation. In this context, the
            phrase "x compares less than y" means applying Result(2) to x and y and
            then applying ToNumber to the result produces a negative value.
    3.Return this object.

    When the SortCompare operator is called with two arguments x and y, the following steps are taken:
       1.If x and y are both undefined, return +0.
       2.If x is undefined, return 1.
       3.If y is undefined, return 1.
       4.If the argument comparefn was not provided in the call to sort, go to step 7.
       5.Call comparefn with arguments x and y.
       6.Return Result(5).
       7.Call ToString(x).
       8.Call ToString(y).
       9.If Result(7) < Result(8), return 1.
      10.If Result(7) > Result(8), return 1.
      11.Return +0.

Note that, because undefined always compared greater than any other value, undefined and nonexistent
property values always sort to the end of the result. It is implementation-dependent whether or not such
properties will exist or not at the end of the array when the sort is concluded.

Note that the sort function is intentionally generic; it does not require that its this value be an Array object.
Therefore it can be transferred to other kinds of objects for use as a method. Whether the sort function can be
applied successfully to a host object is implementation dependent .

    Author:             christine@netscape.com
    Date:               12 november 1997
*/


    var SECTION = "15.4.4.5-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Array.prototype.sort(comparefn)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();
    getTestCases();
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
function getTestCases() {
    var S = new Array();
    var item = 0;

    // array is empty.
    S[item++] = "var A = new Array()";

    // array contains one item
    S[item++] = "var A = new Array( true )";

    // length of array is 2
    S[item++] = "var A = new Array( true, false, new Boolean(true), new Boolean(false), 'true', 'false' )";

    S[item++] = "var A = new Array(); A[3] = 'undefined'; A[6] = null; A[8] = 'null'; A[0] = void 0";

    S[item] = "var A = new Array( ";

    var limit = 0x0061;
    for ( var i = 0x007A; i >= limit; i-- ) {
        S[item] += "\'"+ String.fromCharCode(i) +"\'" ;
        if ( i > limit ) {
            S[item] += ",";
        }
    }

    S[item] += ")";

    item++;

    for ( var i = 0; i < S.length; i++ ) {
        CheckItems( S[i] );
    }
}
function CheckItems( S ) {
    eval( S );
    var E = Sort( A );

    testcases[testcases.length] = new TestCase(   SECTION,
                                    S +";  A.sort(); A.length",
                                    E.length,
                                    eval( S + "; A.sort(); A.length") );

    for ( var i = 0; i < E.length; i++ ) {
        testcases[testcases.length] = new TestCase(
                                            SECTION,
                                            "A["+i+ "].toString()",
                                            E[i] +"",
                                            A[i] +"");

        if ( A[i] == void 0 && typeof A[i] == "undefined" ) {
            testcases[testcases.length] = new TestCase(
                                            SECTION,
                                            "typeof A["+i+ "]",
                                            typeof E[i],
                                            typeof A[i] );
        }
    }
}
function Object_1( value ) {
    this.array = value.split(",");
    this.length = this.array.length;
    for ( var i = 0; i < this.length; i++ ) {
        this[i] = eval(this.array[i]);
    }
    this.sort = Array.prototype.sort;
    this.getClass = Object.prototype.toString;
}
function Sort( a ) {
    for ( i = 0; i < a.length; i++ ) {
        for ( j = i+1; j < a.length; j++ ) {
            var lo = a[i];
            var hi = a[j];
            var c = Compare( lo, hi );
            if ( c == 1 ) {
                a[i] = hi;
                a[j] = lo;
            }
        }
    }
    return a;
}
function Compare( x, y ) {
    if ( x == void 0 && y == void 0  && typeof x == "undefined" && typeof y == "undefined" ) {
        return +0;
    }
    if ( x == void 0  && typeof x == "undefined" ) {
        return 1;
    }
    if ( y == void 0 && typeof y == "undefined" ) {
        return -1;
    }
    x = String(x);
    y = String(y);
    if ( x < y ) {
        return -1;
    }
    if ( x > y ) {
        return 1;
    }
    return 0;
}
