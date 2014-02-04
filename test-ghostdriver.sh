#!/usr/bin/env bash

# Go to GhostDriver (Java) Tests
pushd ./test/ghostdriver-test/java

# Ensure Gradle Wrapper is executable
chmod +x ./gradlew

# Run tests
./gradlew test

# Return to starting directory
popd
