import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 400
        height: 300

        experimental.preferences.javascriptEnabled: true
        experimental.preferences.localStorageEnabled: true
        experimental.preferences.pluginsEnabled: true

        TestWebView {
            id: webView2
            width: 400
            height: 300
        }

        SignalSpy {
            id: titleSpy
            target: webView
            signalName: "titleChanged"
        }

        SignalSpy {
            id: standardFontFamilySpy
            target: webView.experimental.preferences
            signalName: "standardFontFamilyChanged"
        }

        SignalSpy {
            id: fixedFontFamilySpy
            target: webView.experimental.preferences
            signalName: "fixedFontFamilyChanged"
        }

        SignalSpy {
            id: serifFontFamilySpy
            target: webView.experimental.preferences
            signalName: "serifFontFamilyChanged"
        }

        SignalSpy {
            id: sansSerifFontFamilySpy
            target: webView.experimental.preferences
            signalName: "sansSerifFontFamilyChanged"
        }

        SignalSpy {
            id: cursiveFontFamilySpy
            target: webView.experimental.preferences
            signalName: "cursiveFontFamilyChanged"
        }

        SignalSpy {
            id: fantasyFontFamilySpy
            target: webView.experimental.preferences
            signalName: "fantasyFontFamilyChanged"
        }

        SignalSpy {
            id: minimumFontSizeSpy
            target: webView.experimental.preferences
            signalName: "minimumFontSizeChanged"
        }

        SignalSpy {
            id: defaultFontSizeSpy
            target: webView.experimental.preferences
            signalName: "defaultFontSizeChanged"
        }

        SignalSpy {
            id: defaultFixedFontSizeSpy
            target: webView.experimental.preferences
            signalName: "defaultFixedFontSizeChanged"
        }

        TestCase {
            name: "WebViewPreferences"

            property bool shouldSetupFonts: true
            property string defaultStandardFontFamily
            property string defaultFixedFontFamily
            property string defaultSerifFontFamily
            property string defaultSansSerifFontFamily
            property string defaultCursiveFontFamily
            property string defaultFantasyFontFamily
            property int defaultMinimumFontSize
            property int defaultFontSize
            property int defaultFixedFontSize

            function init() {
                if (shouldSetupFonts) {
                    // Setup initial values (may be different per platform).
                    shouldSetupFonts = false
                    defaultStandardFontFamily = webView.experimental.preferences.standardFontFamily
                    defaultFixedFontFamily = webView.experimental.preferences.fixedFontFamily
                    defaultSerifFontFamily = webView.experimental.preferences.serifFontFamily
                    defaultSansSerifFontFamily = webView.experimental.preferences.sansSerifFontFamily
                    defaultCursiveFontFamily = webView.experimental.preferences.cursiveFontFamily
                    defaultFantasyFontFamily = webView.experimental.preferences.fantasyFontFamily
                    defaultMinimumFontSize = webView.experimental.preferences.minimumFontSize
                    defaultFontSize = webView.experimental.preferences.defaultFontSize
                    defaultFixedFontSize = webView.experimental.preferences.defaultFixedFontSize
                }
                else {
                    // Restore default values before starting a new test case.
                    webView.experimental.preferences.standardFontFamily = defaultStandardFontFamily
                    webView.experimental.preferences.fixedFontFamily = defaultFixedFontFamily
                    webView.experimental.preferences.serifFontFamily = defaultSerifFontFamily
                    webView.experimental.preferences.sansSerifFontFamily = defaultSansSerifFontFamily
                    webView.experimental.preferences.cursiveFontFamily = defaultCursiveFontFamily
                    webView.experimental.preferences.fantasyFontFamily = defaultFantasyFontFamily
                    webView.experimental.preferences.minimumFontSize = defaultMinimumFontSize
                    webView.experimental.preferences.defaultFontSize = defaultFontSize
                    webView.experimental.preferences.defaultFixedFontSize = defaultFixedFontSize

                    if (webView.url != '' && webView.url != 'about:blank') {
                        webView.url = 'about:blank'
                        verify(webView.waitForLoadSucceeded())
                    }

                    standardFontFamilySpy.clear()
                    fixedFontFamilySpy.clear()
                    serifFontFamilySpy.clear()
                    sansSerifFontFamilySpy.clear()
                    cursiveFontFamilySpy.clear()
                    fantasyFontFamilySpy.clear()
                    minimumFontSizeSpy.clear()
                    defaultFontSizeSpy.clear()
                    defaultFixedFontSizeSpy.clear()
                }

                webView.experimental.preferences.javascriptEnabled = true
                webView.experimental.preferences.localStorageEnabled = true
                webView.experimental.preferences.pluginsEnabled = true
                titleSpy.clear()
            }

            function test_javascriptEnabled() {
                webView.experimental.preferences.javascriptEnabled = true
                var testUrl = Qt.resolvedUrl("../common/javascript.html")
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "New Title")
            }

            function test_javascriptDisabled() {
                webView.experimental.preferences.javascriptEnabled = false
                var testUrl = Qt.resolvedUrl("../common/javascript.html")
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
            }

            function test_localStorageDisabled() {
                webView.experimental.preferences.localStorageEnabled = false
                var testUrl = Qt.resolvedUrl("../common/localStorage.html")
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
            }

            function test_localStorageEnabled() {
                webView.experimental.preferences.localStorageEnabled = true
                var testUrl = Qt.resolvedUrl("../common/localStorage.html")
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "New Title")
            }

            function test_preferencesAffectCurrentViewOnly() {
                webView.experimental.preferences.javascriptEnabled = true
                webView2.experimental.preferences.javascriptEnabled = true
                var testUrl = Qt.resolvedUrl("../common/javascript.html")
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                webView2.url = testUrl
                verify(webView2.waitForLoadSucceeded())
                compare(webView.title, "New Title")
                compare(webView2.title, "New Title")
                webView.experimental.preferences.javascriptEnabled = false
                webView.url = testUrl
                verify(webView.waitForLoadSucceeded())
                webView2.url = testUrl
                verify(webView2.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                compare(webView2.title, "New Title")
            }

            function unquote(text) {
                return text[0] === "'" ? text.slice(1, -1) : text
            }

            function test_standardFontFamilyChanged() {
                var url = Qt.resolvedUrl("../common/font-preferences.html?standard#font-family")
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(unquote(webView.title), defaultStandardFontFamily)

                webView.experimental.preferences.standardFontFamily = "foobar"
                standardFontFamilySpy.wait()
                compare(standardFontFamilySpy.count, 1)
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, "foobar")
            }

            function test_fontSizeChanged() {
                var url = Qt.resolvedUrl("../common/font-preferences.html?standard#font-size")
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")                
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, defaultFontSize.toString() + "px")

                webView.experimental.preferences.defaultFontSize = defaultFontSize + 1
                defaultFontSizeSpy.wait()
                compare(defaultFontSizeSpy.count, 1)
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, (defaultFontSize + 1).toString() + "px")
            }

            function test_fixedFontSizeChanged() {
                var url = Qt.resolvedUrl("../common/font-preferences.html?fixed#font-size")
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, defaultFixedFontSize.toString() + "px")

                webView.experimental.preferences.defaultFixedFontSize = defaultFixedFontSize + 1
                defaultFixedFontSizeSpy.wait()
                compare(defaultFixedFontSizeSpy.count, 1)
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, (defaultFixedFontSize + 1).toString() + "px")

                webView.url = Qt.resolvedUrl("../common/font-preferences.html?standard#font-size")
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, defaultFontSize.toString() + "px")
            }

            function test_minimumFontSizeChanged() {
                verify(defaultMinimumFontSize < defaultFontSize)
                var url = Qt.resolvedUrl("../common/font-preferences.html?minimum#font-size")
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                var smallerFontSize = webView.title
                smallerFontSize = smallerFontSize.substring(0, smallerFontSize.length - 2)
                smallerFontSize = parseInt(smallerFontSize)
                verify(smallerFontSize < defaultFontSize)

                webView.experimental.preferences.minimumFontSize = defaultFontSize
                minimumFontSizeSpy.wait()
                compare(minimumFontSizeSpy.count, 1)
                webView.url = url
                verify(webView.waitForLoadSucceeded())
                compare(webView.title, "Original Title")
                titleSpy.clear()

                titleSpy.wait()
                compare(webView.title, defaultFontSize.toString() + "px")
            }

            function test_defaultFontsChanged() {
                // As there's currently no way to test through JS if a generic font was indeed changed
                // we keep this test for really basic coverage.

                webView.experimental.preferences.standardFontFamily = "foobar0"
                standardFontFamilySpy.wait()
                webView.experimental.preferences.fixedFontFamily = "foobar1"
                fixedFontFamilySpy.wait()
                webView.experimental.preferences.serifFontFamily = "foobar2"
                serifFontFamilySpy.wait()
                webView.experimental.preferences.sansSerifFontFamily = "foobar3"
                sansSerifFontFamilySpy.wait()
                webView.experimental.preferences.cursiveFontFamily = "foobar4"
                cursiveFontFamilySpy.wait()
                webView.experimental.preferences.fantasyFontFamily = "foobar5"
                fantasyFontFamilySpy.wait()

                compare(standardFontFamilySpy.count, 1)
                compare(fixedFontFamilySpy.count, 1)
                compare(serifFontFamilySpy.count, 1)
                compare(sansSerifFontFamilySpy.count, 1)
                compare(cursiveFontFamilySpy.count, 1)
                compare(fantasyFontFamilySpy.count, 1)

                compare(webView.experimental.preferences.standardFontFamily, "foobar0")
                compare(webView.experimental.preferences.fixedFontFamily, "foobar1")
                compare(webView.experimental.preferences.serifFontFamily, "foobar2")
                compare(webView.experimental.preferences.sansSerifFontFamily, "foobar3")
                compare(webView.experimental.preferences.cursiveFontFamily, "foobar4")
                compare(webView.experimental.preferences.fantasyFontFamily, "foobar5")
            }


        }
    }
}
