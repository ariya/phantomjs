#!/usr/bin/env bash

# Go to GhostDriver (Java) Tests
pushd ./test/ghostdriver-test/java

# Ensure Gradle Wrapper is executable
chmod +x ./gradlew

# Run tests
./gradlew test -q
# Grab exit status
TEST_EXIT_STATUS=$?

# Return to starting directory
popd

exit $TEST_EXIT_STATUS
