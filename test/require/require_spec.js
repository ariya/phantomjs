describe("require()", function() {
    it("loads native PhantomJS modules", function() {
        should.exist(require('webpage').create);
        should.exist(require('fs').separator);
        if (phantom.version.major >= 1 && phantom.version.minor >= 4) {
            should.exist(require('webserver').create);
        }
        if (phantom.version.major >= 1 && phantom.version.minor >= 5) {
            require('system').platform.should.equal('phantomjs');
        }
    });

    it("loads CoffeeScript modules", function() {
        require('./coffee_dummy').should.equal('require/coffee_dummy');
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

    it("maintains proper .stack when an error is thrown in module's exports", function() {
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
});
