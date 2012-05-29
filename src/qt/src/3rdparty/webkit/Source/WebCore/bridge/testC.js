myInterface.logMessage ("Starting test");

myInterface.logMessage ("Testing properties:");
myInterface.logMessage ("  myInterface.doubleValue = " + myInterface.doubleValue);
myInterface.logMessage ("  myInterface.intValue = " + myInterface.intValue);
myInterface.logMessage ("  myInterface.stringValue = " + myInterface.stringValue);
myInterface.logMessage ("  myInterface.booleanValue = " + myInterface.booleanValue);
myInterface.logMessage ("  myInterface.nullValue = " + myInterface.nullValue);
myInterface.logMessage ("  myInterface.undefinedValue = " + myInterface.undefinedValue);

myInterface.logMessage ("Testing methods:");
myInterface.logMessage ("  myInterface.setDoubleValue(1234.1234) = " + myInterface.setDoubleValue(1234.1234));
myInterface.logMessage ("  myInterface.setIntValue(5678) = " + myInterface.setIntValue(5678));
myInterface.logMessage ("  myInterface.setStringValue(Goodbye) = " + myInterface.setStringValue('Goodbye'));
myInterface.logMessage ("  myInterface.setBooleanValue(false) = " + myInterface.setBooleanValue(false));

myInterface.logMessage ("Value of properties after calling setters:");
myInterface.logMessage ("  myInterface.getDoubleValue() = " + myInterface.getDoubleValue());
myInterface.logMessage ("  myInterface.getIntValue() = " + myInterface.getIntValue());
myInterface.logMessage ("  myInterface.getStringValue() = " + myInterface.getStringValue());
myInterface.logMessage ("  myInterface.getBooleanValue() = " + myInterface.getBooleanValue());
