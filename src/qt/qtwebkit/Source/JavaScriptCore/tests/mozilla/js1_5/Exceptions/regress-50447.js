/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
*
* The contents of this file are subject to the Netscape Public
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
* Contributor(s): Rob Ginda rginda@netscape.com
*
* SUMMARY: New properties fileName, lineNumber have been added to Error objects
* in SpiderMonkey. These are non-ECMA extensions and do not exist in Rhino.
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=50447
*/
//-----------------------------------------------------------------------------
var bug = 50447;
var summary = 'Test (non-ECMA) Error object properties fileName, lineNumber';


//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  testRealError();
  test1();
  test2();
  test3();
  test4();

  exitFunc('test');
}


function testRealError()
{
    /* throw a real error, and see what it looks like */
    enterFunc ("testRealError");
    
    try 
    {
        blabla;
    }
    catch (e)
    {
        if (e.fileName.search (/-50447\.js$/i) == -1)
            reportFailure ("expected fileName to end with '-50447.js'");

        reportCompare (61, e.lineNumber,
                       "lineNumber property returned unexpected value.");
    }
    
    exitFunc ("testRealError");
}    


function test1()
{
    /* generate an error with msg, file, and lineno properties */
    enterFunc ("test1");

    var e = new InternalError ("msg", "file", 2);
    reportCompare ("(new InternalError(\"msg\", \"file\", 2))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("file", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (2, e.lineNumber,
                   "lineNumber property returned unexpected value.");
    
    exitFunc ("test1");
}


function test2()
{
    /* generate an error with only msg property */
    enterFunc ("test2");

    var e = new InternalError ("msg");
    reportCompare ("(new InternalError(\"msg\", \"\"))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (0, e.lineNumber,
                   "lineNumber property returned unexpected value.");
    
    exitFunc ("test2");
}


function test3()
{
    /* generate an error with only msg and lineNo properties */
    enterFunc ("test3");

    var e = new InternalError ("msg");
    e.lineNumber = 10;
    reportCompare ("(new InternalError(\"msg\", \"\", 10))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (10, e.lineNumber,
                   "lineNumber property returned unexpected value.");
    
    exitFunc ("test3");
}


function test4()
{
    /* generate an error with only msg and filename properties */
    enterFunc ("test4");

    var e = new InternalError ("msg", "file");
    reportCompare ("(new InternalError(\"msg\", \"file\"))",
                   e.toSource(),
                   "toSource() returned unexpected result.");
    reportCompare ("file", e.fileName,
                   "fileName property returned unexpected value.");
    reportCompare (0, e.lineNumber,
                   "lineNumber property returned unexpected value.");
    
    exitFunc ("test4");
}
