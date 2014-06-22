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
    ECMA Section: 10.1.4.1 Entering An Execution Context
    ECMA says:
        * Global Code, Function Code
          Variable instantiation is performed using the global object as the
          variable object and using property attributes { DontDelete }.

        * Eval Code
          Variable instantiation is performed using the calling context's 
          variable object and using empty property attributes.
*/

test();

function test()
{
    enterFunc ("test");

    var y;
    eval("var x = 1");

    if (delete y)
        reportFailure ("Expected *NOT* to be able to delete y");

    if (typeof x == "undefined")
        reportFailure ("x did not remain defined after eval()");
    else if (x != 1)
        reportFailure ("x did not retain it's value after eval()");
    
    if (!delete x)
        reportFailure ("Expected to be able to delete x");

    exitFunc("test");        
}
