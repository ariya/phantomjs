ErrorHelper = {
    foo: function foo() {
        this.bar()
    },

    bar: function bar() {
        referenceError
    }
};
