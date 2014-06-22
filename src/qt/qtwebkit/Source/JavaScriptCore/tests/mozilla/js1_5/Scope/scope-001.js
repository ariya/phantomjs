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
*
*
* The idea behind bug 53268 is as follows. The item 'five' below is defined
* as const, hence is a read-only property of the global object. So if we set
* obj.__proto__  = this,  'five' should become a read-only propery of obj.
*
* If we then change obj.__proto__  to null, obj.five should initially be
* undefined. We should be able to define obj.five to whatever we want,
* and be able to access this value as obj.five.
*
* Bug 53268 was filed because obj.five could not be set or accessed after
* obj.__proto__  had been set to the global object and then to null.
*/
//-----------------------------------------------------------------------------
var bug = '53268';
var status = 'Testing scope after changing obj.__proto__';
var expect= '';
var actual = '';
var obj = {};
const five = 5;


//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function test()
{
   enterFunc ("test");
   printBugNumber (bug);
   printStatus (status);


   status= 'Step 1:  setting obj.__proto__ = global object';
   obj.__proto__ = this;

   actual = obj.five;
   expect=5;
   reportCompare (expect, actual, status);
 
   obj.five=1;
   actual = obj.five;
   expect=5;
   reportCompare (expect, actual, status);



   status= 'Step 2:  setting obj.__proto__ = null';
   obj.__proto__ = null;

   actual = obj.five;
   expect=undefined;
   reportCompare (expect, actual, status);

   obj.five=2;
   actual = obj.five;
   expect=2;
   reportCompare (expect, actual, status);
 

  
   status= 'Step 3:  setting obj.__proto__  to global object again';
   obj.__proto__ = this;

   actual = obj.five;
   expect=2;  //<--- (FROM STEP 2 ABOVE)
   reportCompare (expect, actual, status);
 
   obj.five=3;
   actual = obj.five;
   expect=3;
   reportCompare (expect, actual, status);



   status= 'Step 4:  setting obj.__proto__   to  null again';
   obj.__proto__ = null;

   actual = obj.five;
   expect=3;  //<--- (FROM STEP 3 ABOVE)
   reportCompare (expect, actual, status);

   obj.five=4;
   actual = obj.five;
   expect=4;
   reportCompare (expect, actual, status);


   exitFunc ("test");
}
