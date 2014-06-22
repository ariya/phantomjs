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
 * Contributor(s): 
 * Rob Ginda rginda@netscape.com
 */

/**
    ECMA Section: 10.1.3: Variable Instantiation
    FunctionDeclarations are processed before VariableDeclarations, and 
    VariableDeclarations don't replace existing values with undefined
*/

test();

function f()
{
    var x;

    return typeof x;

    function x()
    {
        return 7;    
    }
}

function test()
{
    enterFunc ("test");

    printStatus ("ECMA Section: 10.1.3: Variable Instantiation.");
    printBugNumber (17290);

    reportCompare ("function", f(), "Declaration precedence test");

    exitFunc("test");        
}
