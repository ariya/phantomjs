deps = {
  "trunk/third_party/gyp":
      "http://gyp.googlecode.com/svn/trunk@1564",

  "trunk/third_party/googletest":
      "http://googletest.googlecode.com/svn/trunk@573", #release 1.6.0

  "trunk/third_party/googlemock":
      "http://googlemock.googlecode.com/svn/trunk@387", #release 1.6.0
}

hooks = [
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "trunk/build/gyp_angle"],
  },
]
