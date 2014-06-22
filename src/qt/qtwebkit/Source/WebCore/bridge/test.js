myInterface.logMessage ("Starting test");

myInterface.logMessage ("Testing properties:");
myInterface.logMessage ("myInterface.doubleValue = " + myInterface.doubleValue);
myInterface.logMessage ("myInterface.intValue = " + myInterface.intValue);
myInterface.logMessage ("myInterface.stringValue = " + myInterface.stringValue);
myInterface.logMessage ("myInterface.booleanValue = " + myInterface.booleanValue);
myInterface.logMessage ("myInterface.nullValue = " + myInterface.nullValue);
myInterface.logMessage ("myInterface.undefinedValue = " + myInterface.undefinedValue);

myInterface.logMessage ("myInterface.setInt_(666) = " + myInterface.setInt_(666));
myInterface.logMessage ("myInterface.getInt() = " + myInterface.getInt());
myInterface.logMessage ("myInterface.getString() = " + myInterface.getString());
myInterface.logMessage ("myInterface.myInt = " + myInterface.myInt);
myInterface.logMessage ("setting myInterface.myInt = 777");
myInterface.myInt = 777;
myInterface.logMessage ("myInterface.myInt = " + myInterface.myInt);
myInterface.logMessage ("myInterface.getMySecondInterface().doubleValue = " + myInterface.getMySecondInterface().doubleValue);
myInterface.logMessage ("myInterface.getMySecondInterface() = " + myInterface.getMySecondInterface());
