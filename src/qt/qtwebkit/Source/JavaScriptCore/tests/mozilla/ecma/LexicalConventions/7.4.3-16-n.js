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
    File Name:          lexical-023.js
    Corresponds To:     7.4.3-16-n.js
    ECMA Section:       7.4.3
    Description:
    The following words are used as keywords in proposed extensions and are
    therefore reserved to allow for the possibility of future adoption of
    those extensions.

    FutureReservedWord :: one of
    case    debugger    export      super
    catch   default     extends     switch
    class   do          finally     throw
    const   enum        import      try

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "lexical-023.js";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Future Reserved Words";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        try = true;
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "try = true" +
        " (threw " + exception +")",
        expect,
        result );

    test();


