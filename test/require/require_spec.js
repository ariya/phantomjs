describe("require()", function() {
    it("loads 'webpage' native module", function() {
        should.exist(require('webpage').create);
    });

    it("loads 'fs' native module", function() {
        should.exist(require('fs').separator);
    });

    it("loads 'webserver' native module", function() {
        should.exist(require('webserver').create);
    });

    it("loads 'cookiejar' native module", function() {
        should.exist(require('cookiejar').create);
    });

    it("loads 'system' native module", function() {
        require('system').platform.should.equal('phantomjs');
    });

    it("doesn't expose CoffeeScript", function() {
        should.not.exist(window.CoffeeScript);
    });

    it("loads JSON modules", function() {
        require('./json_dummy').message.should.equal('hello');
    });

    it("loads modules with specified extension", function() {
        require('./dummy.js').should.equal('require/dummy');
    });

    it("caches modules", function() {
        require('./empty').hello = 'hola';
        require('./empty').hello.should.equal('hola');
    });

    it("supports cycles (circular dependencies)", function() {
        var a = require('./a');
        var b = require('./b');
        a.b.should.equal(b);
        b.a.should.equal(a);
    });

    it("has cache object attached containing cached modules", function() {
        var exposed = require('dummy_exposed');
        should.exist(require.cache);
        require.cache[module.filename].should.equal(module);
        require.cache[exposed.filename].should.equal(exposed);
    });

    it("throws an error with appropriate message when module not found", function() {
        (function() {
            require('dummy_missing');
        }).should.Throw("Cannot find module 'dummy_missing'");
    });

    xit("maintains proper .stack when module not found", function() {
        try {
            require('./not_found').requireNonExistent();
        } catch (e) {
            e.stack.should.match(/\n *at .*not_found\.js:2\n/);
        }
    });

    xit("maintains proper .stack when an error is thrown in module's exports", function() {
        try {
            require('./thrower').fn();
        } catch (e) {
            e.stack.should.match(/^Error: fn\n *at .*thrower\.js:2/);
        }
    });

    describe("stub()", function() {
        it("stubs modules in given context", function() {
            require('./stubber').stubbed.should.equal('stubbed module');
        });

        it("stubs modules in child context", function() {
            require('./stubber').child.stubbed.should.equal('stubbed module');
        });

        it("doesn't stub in parent context", function() {
            (function() {
                require('stubbed');
            }).should.Throw("Cannot find module 'stubbed'");
        });

        describe("when invoked with a factory function", function() {
            var count = 0;
            require.stub('lazily_stubbed', function() {
                ++count;
                return 'lazily stubbed module';
            });

            it("initializes the module lazily", function() {
                require('lazily_stubbed').should.equal('lazily stubbed module');
            });

            it("doesn't reinitialize the module each time it's required", function() {
                require('lazily_stubbed');
                count.should.equal(1);
            });
        });
    });

    describe("when the path is relative", function() {
        it("loads modules from the same directory", function() {
            require('./dummy').should.equal('require/dummy');
        });

        it("loads modules from the parent directory", function() {
            require('../dummy').should.equal('spec/dummy');
        });

        it("loads modules from a child directory", function() {
            require('./dir/dummy').should.equal('dir/dummy');
        });

        it("loads modules from a deeper directory", function() {
            require('./dir/subdir/dummy').should.equal('subdir/dummy');
        });

        it("loads modules when path has intertwined '..'", function() {
            require('./dir/../dummy').should.equal('require/dummy');
        });

        it("loads modules when path has intertwined '.'", function() {
            require('./dir/./dummy').should.equal('dir/dummy');
        });
    });

    describe("when loading from node_modules", function() {
        it("first tries to load from ./node_modules", function() {
            require('dummy_file').should.equal('require/node_modules/dummy_file');
        });

        it("loads from ../node_modules", function() {
            require('dummy_file2').should.equal('spec/node_modules/dummy_file2');
        });

        it("loads from further up the directory tree", function() {
            require('./dir/subdir/loader').dummyFile2.should.equal('spec/node_modules/dummy_file2');
        });

        describe("when module is a directory", function() {
            it("first tries to load the path from package.json", function() {
                require('dummy_module').should.equal('require/node_modules/dummy_module');
            });

            it("loads index.js if package.json not found", function() {
                require('dummy_module2').should.equal('require/node_modules/dummy_module2');
            });
        });
    });

    describe("when path is absolute", function() {
        it("loads modules from the absolute path", function() {
            require(fs.absolute('dummy')).should.equal('spec/dummy');
        });
    });

    describe("with require.paths", function() {
        describe("when require.paths.push(relative)", function() {
            it("add relative path to paths", function() {
                require.paths.push('./dir/subdir');
            });

            it("loads 'loader' module in dir/subdir", function() {
                require('loader').dummyFile2.should.equal('spec/node_modules/dummy_file2');
            });

            it("loads 'loader' module in dir/subdir2 relative to require.paths", function() {
                require('../subdir2/loader').should.equal('require/subdir2/loader');
            });

            it("loads 'dummy' module from the path that takes precedence", function() {
                require('../dummy').should.equal('spec/dummy');
            });

            it("doesn't load 'loader' module in dir/subdir after require.paths.pop()", function() {
                (function() {
                    require.paths.pop();
                    require('loader');
                }).should.Throw("Cannot find module 'loader'");
            });
        });

        describe("when require.paths.push(absolute)", function() {
            it("adds absolute path to paths", function() {
                require.paths.push(fs.absolute('require/dir/subdir'));
            });

            it("loads 'loader' module in dir/subdir", function() {
                require('loader').dummyFile2.should.equal('spec/node_modules/dummy_file2');
            });

            it("loads 'loader' module in dir/subdir2 relative to require.paths", function() {
                require('../subdir2/loader').should.equal('require/subdir2/loader');
            });

            it("loads 'dummy' module from the path that takes precedence", function() {
                require('../dummy').should.equal('spec/dummy');
            });

            it("doesn't load 'loader' module in dir/subdir after require.paths.pop()", function() {
                (function() {
                    require.paths.pop();
                    require('loader');
                }).should.Throw("Cannot find module 'loader'");
            });
        });
    });
});
