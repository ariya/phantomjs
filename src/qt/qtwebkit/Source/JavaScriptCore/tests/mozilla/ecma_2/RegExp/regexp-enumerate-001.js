/**
    File Name:          regexp-enumerate-001.js
    ECMA V2 Section:
    Description:        Regression Test.

    If instance Native Object have properties that are enumerable,
    JavaScript enumerated through the properties twice. This only
    happened if objects had been instantiated, but their properties
    had not been enumerated.  ie, the object inherited properties
    from its prototype that are enumerated.

    In the core JavaScript, this is only a problem with RegExp
    objects, since the inherited properties of most core JavaScript
    objects are not enumerated.

    Author:             christine@netscape.com, pschwartau@netscape.com
    Date:               12 November 1997
    Modified:           14 July 2002
    Reason:             See http://bugzilla.mozilla.org/show_bug.cgi?id=155291
                        ECMA-262 Ed.3  Sections 15.10.7.1 through 15.10.7.5
                        RegExp properties should be DontEnum
*
*/
//    onerror = err;

    var SECTION = "regexp-enumerate-001";
    var VERSION = "ECMA_2";
    var TITLE   = "Regression Test for Enumerating Properties";

    var BUGNUMBER="339403";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    /*
     *  This test expects RegExp instances to have four enumerated properties:
     *  source, global, ignoreCase, and lastIndex
     *
     *  99.01.25:  now they also have a multiLine instance property.
     *
     */


    var r = new RegExp();

    var e = new Array();

    var t = new TestRegExp();

    for ( p in r ) { e[e.length] = { property:p, value:r[p] }; t.addProperty( p, r[p]) };

    testcases[testcases.length] = new TestCase( SECTION,
        "r = new RegExp(); e = new Array(); "+
        "for ( p in r ) { e[e.length] = { property:p, value:r[p] }; e.length",
        0,
        e.length );

    test();

function TestRegExp() {
    this.addProperty = addProperty;
}
function addProperty(name, value) {
       var pass = false;

       if ( eval("this."+name) != void 0 ) {
            pass = true;
       } else {
            eval( "this."+ name+" = "+ false );
       }

       testcases[testcases.length] = new TestCase( SECTION,
            "Property: " + name +" already enumerated?",
            false,
            pass );

        if ( testcases[ testcases.length-1].passed == false ) {
            testcases[testcases.length-1].reason = "property already enumerated";

        }

}
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
