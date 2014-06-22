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
* Contributor(s): pschwartau@netscape.com
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
* Date:    11 Feb 2002
* SUMMARY: Testing functions having duplicate formal parameter names
*
* SpiderMonkey was crashing on each case below if the parameters had
* the same name. But duplicate parameter names are permitted by ECMA;
* see ECMA-262 3rd Edition Final Section 10.1.3
*
* NOTE: Rhino does not have toSource() and uneval(); they are non-ECMA
* extensions to the language. So we include a test for them at the beginning -
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing functions having duplicate formal parameter names';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var OBJ = new Object();
var OBJ_TYPE = OBJ.toString();

/*
 * Exit if the implementation doesn't support toSource() or uneval(),
 * since these are non-ECMA extensions to the language -
 */
try
{
  if (!OBJ.toSource || !uneval(OBJ))
    quit();
}
catch(e)
{
  quit();
}


/*
 * OK, now begin the test. Just checking that we don't crash on these -
 */
function f1(x,x,x,x)
{
  var ret = eval(arguments.toSource());
  return ret.toString();
}
status = inSection(1);
actual = f1(1,2,3,4);
expect = OBJ_TYPE;
addThis();


/*
 * Same thing, but preface |arguments| with the function name
 */
function f2(x,x,x,x)
{
  var ret = eval(f2.arguments.toSource());
  return ret.toString();
}
status = inSection(2);
actual = f2(1,2,3,4);
expect = OBJ_TYPE;
addThis();


function f3(x,x,x,x)
{
  var ret = eval(uneval(arguments));
  return ret.toString();
}
status = inSection(3);
actual = f3(1,2,3,4);
expect = OBJ_TYPE;
addThis();


/*
 * Same thing, but preface |arguments| with the function name
 */
function f4(x,x,x,x)
{
  var ret = eval(uneval(f4.arguments));
  return ret.toString();
}
status = inSection(4);
actual = f4(1,2,3,4);
expect = OBJ_TYPE;
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
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
