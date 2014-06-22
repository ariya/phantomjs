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
* Date:    20 Feb 2002
* SUMMARY: Testing the comparison |undefined === null|
* See http://bugzilla.mozilla.org/show_bug.cgi?id=126722
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 126722;
var summary = 'Testing the comparison |undefined === null|';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
if (undefined === null)
  actual = true;
else
  actual = false;
expect = false;
addThis();



status = inSection(2);
switch(true)
{
  case (undefined === null) :
    actual = true;
    break;

  default:
    actual = false;
}
expect = false;
addThis();



status = inSection(3);
function f3(x)
{
  var res = false;

  switch(true)
  {
    case (x === null) :
      res = true;
      break;

    default:
      // do nothing
  }

  return res;
}

actual = f3(undefined);
expect = false;
addThis();



status = inSection(4);
function f4(arr)
{
  var elt = '';
  var res = false;

  for (i=0; i<arr.length; i++)
  {
    elt = arr[i];

    switch(true)
    {
      case (elt === null) :
        res = true;
        break;

      default:
        // do nothing
    }
  }

  return res;
}

var arr = Array('a', undefined);
actual = f4(arr);
expect = false;
addThis();



status = inSection(5);
function f5(arr)
{
  var len = arr.length;

  for(var i=0; (arr[i]===undefined) && (i<len); i++)
    ; //do nothing
  
  return i;
}

/*
 * An array of 5 undefined elements. Note:
 *
 * The return value of eval(a STATEMENT) is undefined.
 * A non-existent PROPERTY is undefined, not a ReferenceError.
 * No undefined element exists AFTER trailing comma at end.
 *
 */
var arrUndef = [ , undefined, eval('var x = 0'), this.NOT_A_PROPERTY, , ];
actual = f5(arrUndef);
expect = 5;
addThis();



status = inSection(6);
function f6(arr)
{
  var len = arr.length;

  for(var i=0; (arr[i]===null) && (i<len); i++)
    ; //do nothing

  return i;
}

/*
 * Use same array as above. This time we're comparing to |null|, so we expect 0
 */
actual = f6(arrUndef);
expect = 0;
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
