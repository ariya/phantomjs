#if 0
// ! [0]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const int width = 640;
    const int height = 480;

    QGraphicsScene scene;

    QGraphicsView view(&scene);
    view.setFrameShape(QFrame::NoFrame);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QGraphicsWebView webview;
    webview.resize(width, height);
    webview.load(QUrl("http://doc.qt.nokia.com/"));

    scene.addItem(&webview);

    view.resize(width, height);
    view.show();

    return app.exec();
}
// ! [0]


// ! [1]
webview.setResizesToContents(true);
// ! [1]

// ! [2]
class MobileWebView : public QGraphicsWidget {
    Q_OBJECT
public:
    MobileWebView(QGraphicsItem *parent = 0);
    ~MobileWebView();

    bool mousePress(const QPoint &value);
    void mouseMove(const QPoint &value);
    void mouseRelease(const QPoint &value);

private:
    QGraphicsWebView* webView;
};
// ! [2]

// ! [3]
webview.page()->setPreferredContentsSize(QSize(desiredWidth, desiredHeight));
// ! [3]

// ! [4]
QWebSettings::globalSettings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);
// ! [4]

// ! [5]
QWebSettings::globalSettings()->setAttribute(QWebSettings::FrameFlatteningEnable, true);
// ! [5]
#endif
