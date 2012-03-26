%modules = ( # path to module name map
    "QtWebKit" => "$basedir/WebCore",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtWebKit" => "../WebKit/qt/Api",
);
%classnames = (
);
%mastercontent = (
    "core" => "#include <QtCore/QtCore>\n",
    "gui" => "#include <QtGui/QtGui>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "script" => "#include <QtScript/QtScript>\n",
);
%modulepris = (
    "QtWebKit" => "$basedir/WebKit/qt/qt_webkit_version.pri",
);
@ignore_for_master_contents = ( "qwebscriptworld.h" );
