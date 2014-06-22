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
* Portions created by the Initial Developer are Copyright (C) 2003
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): igor@icesoft.no, pschwartau@netscape.com
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
* Date:    06 February 2003
* SUMMARY: Using |instanceof| to check if function is called as a constructor
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=192105
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 192105;
var summary = 'Using |instanceof| to check if f() is called as constructor';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * This function is the heart of the test. It sets the result
 * variable |actual|, which we will compare against |expect|.
 *
 * Note |actual| will be set to |true| or |false| according
 * to whether or not this function is called as a constructor;
 * i.e. whether it is called via the |new| keyword or not -
 */
function f()
{
  actual = (this instanceof f);
}


/*
 * Call f as a constructor from global scope
 */
status = inSection(1);
new f(); // sets |actual|
expect = true;
addThis();

/*
 * Now, not as a constructor
 */
status = inSection(2);
f(); // sets |actual|
expect = false;
addThis();


/*
 * Call f as a constructor from function scope
 */
function F()
{
  new f();
}
status = inSection(3);
F(); // sets |actual|
expect = true;
addThis();

/*
 * Now, not as a constructor
 */
function G()
{
  f();
}
status = inSection(4);
G(); // sets |actual|
expect = false;
addThis();


/*
 * Now make F() and G() methods of an object
 */
var obj = {F:F, G:G};
status = inSection(5);
obj.F(); // sets |actual|
expect = true;
addThis();

status = inSection(6);
obj.G(); // sets |actual|
expect = false;
addThis();


/*
 * Now call F() and G() from yet other functions, and use eval()
 */
function A()
{
  eval('F();');
}
status = inSection(7);
A(); // sets |actual|
expect = true;
addThis();


function B()
{
  eval('G();');
}
status = inSection(8);
B(); // sets |actual|
expect = false;
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
