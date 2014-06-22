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
* Date:    03 June 2002
* SUMMARY: Function param or local var with same name as a function property
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=137000
* See http://bugzilla.mozilla.org/show_bug.cgi?id=138708
* See http://bugzilla.mozilla.org/show_bug.cgi?id=150032
* See http://bugzilla.mozilla.org/show_bug.cgi?id=150859
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 137000;
var summary = 'Function param or local var with same name as a function prop';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * Note use of 'x' both for the parameter to f,
 * and as a property name for |f| as an object
 */
function f(x)
{
}

status = inSection(1);
f.x = 12;
actual = f.x;
expect = 12;
addThis();



/*
 * A more elaborate example, using the call() method
 * to chain constructors from child to parent.
 *
 * The key point is the use of the same name 'p' for both
 * the parameter to the constructor, and as a property name
 */
function parentObject(p)
{
  this.p = 1;
}

function childObject()
{
  parentObject.call(this);
}
childObject.prototype = parentObject;

status = inSection(2);
var objParent = new parentObject();
actual = objParent.p;
expect = 1;
addThis();

status = inSection(3);
var objChild = new childObject();
actual = objChild.p;
expect = 1;
addThis();



/*
 * A similar set-up. Here the same name is being used for
 * the parameter to both the Base and Child constructors,
 */
function Base(id)
{
}

function Child(id)
{
  this.prop = id;
}
Child.prototype=Base;

status = inSection(4);
var c1 = new Child('child1');
actual = c1.prop;
expect = 'child1';
addThis();



/*
 * Use same identifier as a property name, too -
 */
function BaseX(id)
{
}

function ChildX(id)
{
  this.id = id;
}
ChildX.prototype=BaseX;

status = inSection(5);
c1 = new ChildX('child1');
actual = c1.id;
expect = 'child1';
addThis();



/*
 * From http://bugzilla.mozilla.org/show_bug.cgi?id=150032
 *
 * Here the same name is being used both for a local variable
 * declared in g(), and as a property name for |g| as an object
 */
function g()
{
  var propA = g.propA;
  var propB = g.propC;

  this.getVarA = function() {return propA;}
  this.getVarB = function() {return propB;}
}
g.propA = 'A';
g.propB = 'B';
g.propC = 'C';
var obj = new g();

status = inSection(6);
actual = obj.getVarA(); // this one was returning 'undefined'
expect = 'A';
addThis();

status = inSection(7);
actual = obj.getVarB(); // this one is easy; it never failed
expect = 'C';
addThis();



/*
 * By martin.honnen@t-online.de
 * From http://bugzilla.mozilla.org/show_bug.cgi?id=150859
 *
 * Here the same name is being used for a local var in F
 * and as a property name for |F| as an object
 *
 * Twist: the property is added via another function.
 */
function setFProperty(val)
{
  F.propA = val;
}

function F()
{
 var propA = 'Local variable in F';
}

status = inSection(8);
setFProperty('Hello');
actual = F.propA; // this was returning 'undefined'
expect = 'Hello';
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
