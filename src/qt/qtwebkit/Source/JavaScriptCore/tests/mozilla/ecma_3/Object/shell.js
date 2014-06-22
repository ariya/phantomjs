/*
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
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com  
* Date: 14 Mar 2001
*
* SUMMARY: Utility functions for testing objects -
* 
* Suppose obj is an instance of a native type, e.g. Number. 
* Then obj.toString() invokes Number.prototype.toString().
* We would also like to access Object.prototype.toString().
* 
* The difference is this: suppose obj = new Number(7).
* Invoking Number.prototype.toString() on this just returns 7.
* Object.prototype.toString() on this returns '[object Number]'. 
*
* The getJSType() function below will return '[object Number]' for us.
* The getJSClass() function returns 'Number', the [[Class]] property of obj.
* See ECMA-262 Edition 3,  13-Oct-1999,  Section 8.6.2  
*/
//-------------------------------------------------------------------------------------------------
var cnNoObject = 'Unexpected Error!!! Parameter to this function must be an object';
var cnNoClass = 'Unexpected Error!!! Cannot find Class property';
var cnObjectToString = Object.prototype.toString;


// checks that it's safe to call findType()
function getJSType(obj)
{
  if (isObject(obj))
    return findType(obj);
  return cnNoObject;
}


// checks that it's safe to call findType()
function getJSClass(obj)
{
  if (isObject(obj))
    return findClass(findType(obj));
  return cnNoObject;
}


function findType(obj)
{
  return cnObjectToString.apply(obj);
}


// given '[object Number]',  return 'Number'
function findClass(sType)
{
  var re =  /^\[.*\s+(\w+)\s*\]$/;
  var a = sType.match(re);
  
  if (a && a[1])
    return a[1];
  return cnNoClass;
}


function isObject(obj)
{
  return obj instanceof Object;
}
