myInterface.logMessage ("Starting test");

myInterface.logMessage ("Testing properties:");

myInterface.jsobject = new Function ("arg1","arg2","return arg1 + arg2;");
myInterface.logMessage ("myInterface.jsobject =" + myInterface.jsobject);

var functionBody = 'return arg1*arg2;'

myInterface.setJSObject_(new Function ("arg1","arg2",functionBody));
myInterface.logMessage ("myInterface.jsobject =" + myInterface.jsobject);
myInterface.callJSObject__(5,6);
myInterface.callJSObject__(8,9);

myInterface.logMessage ("myInterface.setInt_(666) = " + myInterface.setInt_(666));
myInterface.logMessage ("myInterface.getInt() = " + myInterface.getInt());
myInterface.logMessage ("myInterface.getString().foo() = " + myInterface.getString().foo());
myInterface.logMessage ("myInterface.myInt = " + myInterface.myInt);
myInterface.logMessage ("setting myInterface.myInt = 777");
myInterface.myInt = 777;
myInterface.logMessage ("myInterface.myInt = " + myInterface.myInt);
myInterface.logMessage ("myInterface.getMySecondInterface().doubleValue = " + myInterface.getMySecondInterface().doubleValue);
myInterface.logMessage ("myInterface.getMySecondInterface() = " + myInterface.getMySecondInterface());

myInterface.logMessageWithPrefix ("msg", "prefix");

var strings = [ "one", "two", "three" ];

myInterface.logMessages (strings);