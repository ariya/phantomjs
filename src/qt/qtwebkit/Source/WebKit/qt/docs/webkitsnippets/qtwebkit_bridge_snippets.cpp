
void wrapInFunction()
{

    //! [0]
    // ...
    QWebFrame *frame = myWebPage->mainFrame();
    frame->addToJavaScriptWindowObject("someNameForMyObject", myObject);
    // ...
    //! [0]
#if 0
    //! [1]
    {
        width: ...,
        height: ...,
        toDataURL: function() { ... },
        assignToHTMLImageElement: function(element) { ... }
        toImageData: function() { ... }
    }
    //! [1]
#endif
    //! [2]
    class MyObject : QObject {
        Q_OBJECT
        Q_PROPERTY(QPixmap myPixmap READ getPixmap)

    public:
        QPixmap getPixmap() const;
    };

    /* ... */

    MyObject myObject;
    myWebPage.mainFrame()->addToJavaScriptWindowObject("myObject", &myObject);

    //! [2]
#if 0
    //! [3]
    <html>
        <head>
            <script>
                function loadImage()
                {
                    myObject.myPixmap.assignToHTMLImageElement(document.getElementById("imageElement"));
                    var context = document.getElementById("canvasElement").getContext("2d");
                    context.putImageData(myObject.myPixmap.toImageData());
                }
            </script>
        </head>
        <body onload="loadImage()">
            <img id="imageElement" width="300" height="200" />
            <canvas id="canvasElement" width="300" height="200" />
        </body>
    </html>
//! [3]
#endif
//! [4]
class MyObject : QObject {
        Q_OBJECT

    public Q_SLOTS:
        void doSomethingWithWebElement(const QWebElement&);
    };

    /* ... */

    MyObject myObject;
    myWebPage.mainFrame()->addToJavaScriptWindowObject("myObject", &myObject);

    //! [4]
#if 0
    //! [5]
    <html>
         <head>
             <script>
                 function runExample() {
                    myObject.doSomethingWithWebElement(document.getElementById("someElement"));
                 }
            </script>
         </head>
         <body onload="runExample()">
             <span id="someElement">Text</span>
         </body>
     </html>
    //! [5]
    //! [6]
    connect(function);
    //! [6]
    //! [7]
    function myInterestingScriptFunction() { ... }
    ...
    myQObject.somethingChanged.connect(myInterestingScriptFunction);
    //! [7]
    //! [8]
    myQObject.somethingChanged.connect(myOtherQObject.doSomething);
    //! [8]
    //! [9]
    myQObject.somethingChanged.disconnect(myInterestingFunction);
    myQObject.somethingChanged.disconnect(myOtherQObject.doSomething);
    //! [9]
    //! [10]
    myQObject.somethingChanged.connect(thisObject, function)
    //! [10]
    //! [11]
    var form = { x: 123 };
    var onClicked = function() { print(this.x); };
    myButton.clicked.connect(form, onClicked);
    //! [11]
    //! [12]
    myQObject.somethingChanged.disconnect(thisObject, function);
    //! [12]
    //! [13]
    connect(function);
    //! [13]
    //! [14]
    myQObject.somethingChanged.connect(thisObject, "functionName")
    //! [14]
    //! [15]
    var obj = { x: 123, fun: function() { print(this.x); } };
    myQObject.somethingChanged.connect(obj, "fun");
    //! [15]
    //! [16]
    connect(function);
    //! [16]
    //! [17]
    myQObject.somethingChanged.disconnect(thisObject, "functionName");
    //! [17]
    //! [18]
    try {
        myQObject.somethingChanged.connect(myQObject, "slotThatDoesntExist");
    } catch (e) {
        print(e);
    }
    //! [18]
    //! [19]
    myQObject.somethingChanged("hello");
    //! [19]
    //! [20]
    myQObject.myOverloadedSlot(10);   // will call the int overload
    myQObject.myOverloadedSlot("10"); // will call the QString overload
    //! [20]
    //! [21]
    myQObject['myOverloadedSlot(int)']("10");   // call int overload; the argument is converted to an int
    myQObject['myOverloadedSlot(QString)'](10); // call QString overload; the argument is converted to a string
    //! [21]
    //! [22]
    class MyObject : public QObject
    {
        Q_OBJECT

    public:
        Q_INVOKABLE void thisMethodIsInvokableInJavaScript();
        void thisMethodIsNotInvokableInJavaScript();

        ...
    };
    //! [22]
    //! [23]
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
    //! [23]
    //! [24]
    myQObject.enabled = true;

    ...

    myQObject.enabled = !myQObject.enabled;
    //! [24]
    //! [25]
    myDialog.okButton
    //! [25]
    //! [26]
    myDialog.okButton
    myDialog.okButton.objectName = "cancelButton";
    // from now on, myDialog.cancelButton references the button
    //! [26]
#endif
}

