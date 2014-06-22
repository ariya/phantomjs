/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):  jim-patterson@ncf.ca, brendan@mozilla.org,
*                  pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    18 Jun 2002
* SUMMARY: Shouldn't crash when catch parameter is "hidden" by varX
* See http://bugzilla.mozilla.org/show_bug.cgi?id=146596
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 146596;
var summary = "Shouldn't crash when catch parameter is 'hidden' by varX";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * Just seeing we don't crash when executing this function -
 * This example provided by jim-patterson@ncf.ca
 * 
 * Brendan: "Jim, thanks for the testcase. But note that |var|
 * in a JS function makes a function-scoped variable --  JS lacks
 * block scope apart from for catch variables within catch blocks.
 *
 * Therefore the catch variable hides the function-local variable."
 */
function F()
{
  try
  {
    return "A simple exception";
  }
  catch(e)
  {
    var e = "Another exception";
  }

  return 'XYZ';
}

status = inSection(1);
actual = F();
expect = "A simple exception";
addThis();



/*
 * Sanity check by Brendan: "This should output
 *
 *             24
 *             42
 *          undefined
 *
 * and throw no uncaught exception."
 *
 */
function f(obj)
{
  var res = [];

  try
  {
    throw 42;
  }
  catch(e)
  {
    with(obj)
    {
      var e;
      res[0] = e; // |with| binds tighter than |catch|; s/b |obj.e|
    }

    res[1] = e;   // |catch| binds tighter than function scope; s/b 42
  }

  res[2] = e;     // |var e| has function scope; s/b visible but contain |undefined|
  return res;
}

status = inSection(2);
actual = f({e:24});
expect = [24, 42, undefined];
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual.toString();
  expectedvalues[UBound] = expect.toString();
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
