describe("Files and Directories API", function() {
	var TEST_DIR = "testdir",
		TEST_FILE = "testfile",
		START_CWD = null;
	
	it("should create a new temporary directory and change the Current Working Directory to it", function() {
		START_CWD = fs.workingDirectory;
		expect(fs.makeDirectory(TEST_DIR)).toBeTruthy();
		expect(fs.changeWorkingDirectory(TEST_DIR)).toBeTruthy();
	});
	
	it("should create a file in the Current Working Directory and check it's absolute path", function() {
		fs.write(TEST_FILE, TEST_FILE, "w");
		var suffix = fs.separator + TEST_DIR + fs.separator +  TEST_FILE,
			abs = fs.absolute(".." + suffix),
			lastIndex = abs.lastIndexOf(suffix);
		expect(lastIndex).toNotEqual(-1);
		expect(lastIndex + suffix.length === abs.length);
	});
	
	it("should return to previous Current Working Directory and remove temporary directory", function() {
		expect(fs.changeWorkingDirectory(START_CWD)).toBeTruthy();
		fs.removeTree(TEST_DIR);
	});

	it("should copy Current Working Directory tree in a temporary directory, compare with the original and remove", function() {
		expect(fs.changeWorkingDirectory("../")).toBeTruthy();
		fs.copyTree(START_CWD, TEST_DIR);
		expect(fs.list(START_CWD).length).toEqual(fs.list(TEST_DIR).length);
		fs.removeTree(TEST_DIR);
		expect(fs.changeWorkingDirectory(START_CWD)).toBeTruthy();
	});
});