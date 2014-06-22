/**
 *  File Name:          String/split-002.js
 *  ECMA Section:       15.6.4.9
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */

/*
 * Since regular expressions have been part of JavaScript since 1.2, there
 * are already tests for regular expressions in the js1_2/regexp folder.
 *
 * These new tests try to supplement the existing tests, and verify that
 * our implementation of RegExp conforms to the ECMA specification, but
 * does not try to be as exhaustive as in previous tests.
 *
 * The [,limit] argument to String.split is new, and not covered in any
 * existing tests.
 *
 * String.split cases are covered in ecma/String/15.5.4.8-*.js.
 * String.split where separator is a RegExp are in
 * js1_2/regexp/string_split.js
 *
 */

    var SECTION = "ecma_2/String/split-002.js";
    var VERSION = "ECMA_2";
    var TITLE   = "String.prototype.split( regexp, [,limit] )";

    startTest();

    // the separator is not supplied
    // separator is undefined
    // separator is an empty string

//    AddSplitCases( "splitme", "", "''", ["s", "p", "l", "i", "t", "m", "e"] );
//    AddSplitCases( "splitme", new RegExp(), "new RegExp()", ["s", "p", "l", "i", "t", "m", "e"] );

    // separator is an empty regexp
    // separator is not supplied

    CompareSplit( "hello", "ll" );

    CompareSplit( "hello", "l" );
    CompareSplit( "hello", "x" );
    CompareSplit( "hello", "h" );
    CompareSplit( "hello", "o" );
    CompareSplit( "hello", "hello" );
    CompareSplit( "hello", undefined );

    CompareSplit( "hello", "");
    CompareSplit( "hello", "hellothere" );

    CompareSplit( new String("hello" ) );


    Number.prototype.split = String.prototype.split;

    CompareSplit( new Number(100111122133144155), 1 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, 1 );

    CompareSplitWithLimit(new Number(100111122133144155), 1, 2 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, 0 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, 100 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, void 0 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, Math.pow(2,32)-1 );
    CompareSplitWithLimit(new Number(100111122133144155), 1, "boo" );
    CompareSplitWithLimit(new Number(100111122133144155), 1, -(Math.pow(2,32)-1) );
    CompareSplitWithLimit( "hello", "l", NaN );
    CompareSplitWithLimit( "hello", "l", 0 );
    CompareSplitWithLimit( "hello", "l", 1 );
    CompareSplitWithLimit( "hello", "l", 2 );
    CompareSplitWithLimit( "hello", "l", 3 );
    CompareSplitWithLimit( "hello", "l", 4 );


/*
    CompareSplitWithLimit( "hello", "ll", 0 );
    CompareSplitWithLimit( "hello", "ll", 1 );
    CompareSplitWithLimit( "hello", "ll", 2 );
    CompareSplit( "", " " );
    CompareSplit( "" );
*/

    // separartor is a regexp
    // separator regexp value global setting is set
    // string is an empty string
    // if separator is an empty string, split each by character

    // this is not a String object

    // limit is not a number
    // limit is undefined
    // limit is larger than 2^32-1
    // limit is a negative number

    test();

function CompareSplit( string, separator ) {
    split_1 = string.split( separator );
    split_2 = string_split( string, separator );

    AddTestCase(
        "( " + string +".split(" + separator + ") ).length" ,
        split_2.length,
        split_1.length );

    var limit = split_1.length > split_2.length ?
                    split_1.length : split_2.length;

    for ( var split_item = 0; split_item < limit; split_item++ ) {
        AddTestCase(
            string + ".split(" + separator + ")["+split_item+"]",
            split_2[split_item],
            split_1[split_item] );
    }
}

function CompareSplitWithLimit( string, separator, splitlimit ) {
    split_1 = string.split( separator, splitlimit );
    split_2 = string_split( string, separator, splitlimit );

    AddTestCase(
        "( " + string +".split(" + separator + ", " + splitlimit+") ).length" ,
        split_2.length,
        split_1.length );

    var limit = split_1.length > split_2.length ?
                    split_1.length : split_2.length;

    for ( var split_item = 0; split_item < limit; split_item++ ) {
        AddTestCase(
            string + ".split(" + separator  + ", " + splitlimit+")["+split_item+"]",
            split_2[split_item],
            split_1[split_item] );
    }
}

function string_split ( __this, separator, limit ) {
    var S = String(__this );					  // 1

    var A = new Array();                          // 2

    if ( limit == undefined ) {                   // 3
        lim = Math.pow(2, 31 ) -1;
    } else {
        lim = ToUint32( limit );
    }

	var s = S.length;                              // 4
    var p = 0;                                     // 5

    if  ( separator == undefined ) {              // 8
        A[0] = S;
        return A;
    }
	
    if ( separator.constructor == RegExp )         // 6
        R = separator;
	else
		R = separator.toString();

	if (lim == 0) return A;                       // 7

    if  ( separator == undefined ) {              // 8
        A[0] = S;
        return A;
    }
	
	if (s == 0) {		                          // 9
		z = SplitMatch(R, S, 0);
		if (z != false) return A;
        A[0] = S;
        return A;
	}

	var q = p;									  // 10
loop:
    while (true ) { 
		
		if ( q == s ) break;					  // 11
	
		z = SplitMatch(R, S, q);                  // 12

//print("Returned ", z);

		if (z != false) {							// 13
			e = z.endIndex;							// 14
			cap = z.captures;						// 14
			if (e != p) {							// 15
//print("S = ", S, ", p = ", p, ", q = ", q);
				T = S.slice(p, q);					// 16
//print("T = ", T);
				A[A.length] = T;					// 17
				if (A.length == lim) return A;		// 18
				p = e;								// 19
				i = 0;								// 20
				while (true) {						// 25
					if (i == cap.length) {              // 21
						q = p;                          // 10
						continue loop;
					}
					i = i + 1;							// 22
					A[A.length] = cap[i]				// 23
					if (A.length == lim) return A;		// 24
				} 
			}
		}

		q = q + 1;                               // 26
	}
	
	T = S.slice(p, q);
	A[A.length] = T;
	return A;
}

function SplitMatch(R, S, q)
{
	if (R.constructor == RegExp) {			// 1
		var reResult = R.match(S, q);		// 8
		if (reResult == undefined)
			return false;
		else {
			a = new Array(reResult.length - 1);
			for (var i = 1; i < reResult.length; i++)
				a[a.length] = reResult[i];
			return { endIndex : reResult.index + reResult[0].length, captures : cap };
		}
	}
	else {
		var r = R.length;					// 2
		s = S.length;						// 3
		if ((q + r) > s) return false;		// 4
		for (var i = 0; i < r; i++) {
//print("S.charAt(", q + i, ") = ", S.charAt(q + i), ", R.charAt(", i, ") = ", R.charAt(i));
			if (S.charAt(q + i) != R.charAt(i))			// 5
				return false;
		}
		cap = new Array();								// 6
		return { endIndex : q + r, captures : cap };	// 7
	}
}

function ToUint32( n ) {
    n = Number( n );
    var sign = ( n < 0 ) ? -1 : 1;

    if ( Math.abs( n ) == 0 
			|| Math.abs( n ) == Number.POSITIVE_INFINITY
			|| n != n) {
        return 0;
    }
    n = sign * Math.floor( Math.abs(n) )

    n = n % Math.pow(2,32);

    if ( n < 0 ){
        n += Math.pow(2,32);
    }

    return ( n );
}
