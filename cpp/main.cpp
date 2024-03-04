// Qt lib import
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

// JQLibrary lib import
#include <JQImageItem>

// Project lib import
#include "helper.h"

int main(int argc, char **argv)
{
    QGuiApplication app( argc, argv );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    QQuickWindow::setGraphicsApi( QSGRendererInterface::OpenGL );
#else
    QQuickWindow::setSceneGraphBackend( QSGRendererInterface::OpenGL );
#endif

    JQIMAGEITEM_REGISTERTYPE;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty( "Helper", new Helper );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    return app.exec();
}
