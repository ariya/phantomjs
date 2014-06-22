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

test();

function test()
{
    enterFunc ("test");

    var EXCEPTION_DATA = "String exception";
    var e = "foo", x = "foo";
    var caught = false;

    printStatus ("Catchguard 'Common Scope' test.");
    
    try 
    {    
        throw EXCEPTION_DATA;   
    }
    catch (e if ((x = 1) && false))
    {
        reportFailure ("Catch block (e if ((x = 1) && false) should not " +
                       "have executed.");
    }
    catch (e if (x == 1))
    {   
        caught = true;
    }
    catch (e)
    {   
        reportFailure ("Same scope should be used across all catchguards.");
    }

    if (!caught)
        reportFailure ("Execption was never caught.");
    
    if (e != "foo")
        reportFailure ("Exception data modified inside catch() scope should " +
                       "not be visible in the function scope (e ='" +
                       e + "'.)");

    if (x != 1)
        reportFailure ("Data modified in 'catchguard expression' should " +
                       "be visible in the function scope (x = '" +
                       x + "'.)");

    exitFunc ("test");
}
