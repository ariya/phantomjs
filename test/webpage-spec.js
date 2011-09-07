describe("WebPage object", function() {
    var page = require('webpage').create();

    it("should be creatable", function() {
        expect(typeof page).toEqual('object');
        expect(page).toNotEqual(null);
    });

    expectHasProperty(page, 'clipRect');
    it("should have clipRect with height 0", function() {
        expect(page.clipRect.height).toEqual(0);
    });
    it("should have clipRect with left 0", function() {
        expect(page.clipRect.left).toEqual(0);
    });
    it("should have clipRect with top 0", function() {
        expect(page.clipRect.top).toEqual(0);
    });
    it("should have clipRect with width 0", function() {
        expect(page.clipRect.width).toEqual(0);
    });

    expectHasPropertyString(page, 'content');

    expectHasPropertyString(page, 'libraryPath');
    it("should have objectName as 'WebPage'", function() {
        expect(page.objectName).toEqual('WebPage');
    });

    expectHasProperty(page, 'paperSize');
    it("should have paperSize as an empty object", function() {
            expect(page.paperSize).toEqual({});
    });

    expectHasProperty(page, 'scrollPosition');
    it("should have scrollPosition with left 0", function() {
        expect(page.scrollPosition.left).toEqual(0);
    });
    it("should have scrollPosition with top 0", function() {
        expect(page.scrollPosition.top).toEqual(0);
    });

    expectHasProperty(page, 'settings');
    it("should have non-empty settings", function() {
        expect(page.settings).toNotEqual(null);
        expect(page.settings).toNotEqual({});
    });


    expectHasProperty(page, 'viewportSize');
    it("should have viewportSize with height 300", function() {
        expect(page.viewportSize.height).toEqual(300);
    });
    it("should have viewportSize with width 400", function() {
        expect(page.viewportSize.width).toEqual(400);
    });

    expectHasFunction(page, 'click');
    expectHasFunction(page, 'deleteLater');
    expectHasFunction(page, 'destroyed');
    expectHasFunction(page, 'evaluate');
    expectHasFunction(page, 'initialized');
    expectHasFunction(page, 'injectJs');
    expectHasFunction(page, 'javaScriptAlertSent');
    expectHasFunction(page, 'javaScriptConsoleMessageSent');
    expectHasFunction(page, 'loadFinished');
    expectHasFunction(page, 'loadStarted');
    expectHasFunction(page, 'mouseDown');
    expectHasFunction(page, 'mouseMoveTo');
    expectHasFunction(page, 'mouseUp');
    expectHasFunction(page, 'openUrl');
    expectHasFunction(page, 'release');
    expectHasFunction(page, 'render');
    expectHasFunction(page, 'resourceReceived');
    expectHasFunction(page, 'resourceRequested');
    expectHasFunction(page, 'uploadFile');
});
