describe("Files and Directories API", function() {
    var TEST_DIR = "testdir",
        TEST_FILE = "testfile",
        START_CWD = fs.workingDirectory;

    it("should create a new temporary directory and change the Current Working Directory to it", function() {
        expect(fs.makeDirectory(TEST_DIR)).toBeTruthy();
        expect(fs.changeWorkingDirectory(TEST_DIR)).toBeTruthy();
    });

    it("should create a file in the Current Working Directory and check it's absolute path", function() {
        fs.write(TEST_FILE, TEST_FILE, "w");
        var suffix = fs.join("", TEST_DIR, TEST_FILE),
            abs = fs.absolute(".." + suffix),
            lastIndex = abs.lastIndexOf(suffix);
        expect(lastIndex).toNotEqual(-1);
        expect(lastIndex + suffix.length === abs.length);
    });

    it("should return to previous Current Working Directory and remove temporary directory", function() {
        expect(fs.changeWorkingDirectory(START_CWD)).toBeTruthy();
        fs.removeTree(TEST_DIR);
    });

    it("should copy Content of the '/test/' Directory in a temporary directory, compare with the original and then remove", function() {
        var phantomLibraryPathListingLength = fs.list(phantom.libraryPath).length;
        fs.copyTree(phantom.libraryPath, "/tmp/"+TEST_DIR);
        expect(phantomLibraryPathListingLength === fs.list("/tmp/"+TEST_DIR).length);
        fs.removeTree("/tmp/"+TEST_DIR);
    });

    // TODO: test the actual functionality once we can create symlink.
    it("should have readLink function", function() {
            expect(typeof fs.readLink).toEqual('function');
    });

    fs.removeTree(TEST_DIR);

    describe("fs.join(...)", function() {
        var parts, expected, actual;

        it("empty parts", function() {
            parts = [];
            expected = ".";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("one part (empty string)", function() {
            parts = [""];
            expected = ".";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("one part (array)", function() {
            parts = [[], null];
            expected = ".";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("empty string and one part", function() {
            parts = ["", "a"];
            expected = "/a";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("empty string and multiple parts", function() {
            parts = ["", "a", "b", "c"];
            expected = "/a/b/c";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("empty string and multiple parts with empty strings", function() {
            parts = ["", "a", "", "b", "", "c"];
            expected = "/a/b/c";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("multiple parts", function() {
            parts = ["a", "b", "c"];
            expected = "a/b/c";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });

        it("multiple parts with empty strings", function() {
            parts = ["a", "", "b", "", "c"];
            expected = "a/b/c";
            actual = fs.join.apply(null, parts);
            expect(actual).toEqual(expected);
        });
    });

    describe("fs.split(path)", function() {
        var path, expected, actual;

        it("should split absolute path with trailing separator", function() {
            path = fs.separator + "a" + fs.separator + "b" + fs.separator + "c" + fs.separator + "d" + fs.separator;
            actual = fs.split(path);
            expected = ["", "a", "b", "c", "d"];
            expect(actual).toEqual(expected);
        });

        it("should split absolute path without trailing separator", function() {
            path = fs.separator + "a" + fs.separator + "b" + fs.separator + "c" + fs.separator + "d";
            actual = fs.split(path);
            expected = ["", "a", "b", "c", "d"];
            expect(actual).toEqual(expected);
        });

        it("should split non-absolute path with trailing separator", function() {
            path = "a" + fs.separator + "b" + fs.separator + "c" + fs.separator + "d" + fs.separator;
            actual = fs.split(path);
            expected = ["a", "b", "c", "d"];
            expect(actual).toEqual(expected);
        });

        it("should split non-absolute path without trailing separator", function() {
            path = "a" + fs.separator + "b" + fs.separator + "c" + fs.separator + "d";
            actual = fs.split(path);
            expected = ["a", "b", "c", "d"];
            expect(actual).toEqual(expected);
        });

        it("should split path with consecutive separators", function() {
            path = "a" + fs.separator + fs.separator + fs.separator + "b" + fs.separator + "c" + fs.separator + fs.separator + "d" + fs.separator + fs.separator + fs.separator;
            expected = ["a", "b", "c", "d"];
            actual = fs.split(path);
            expect(actual).toEqual(expected);
        });
    });
});
