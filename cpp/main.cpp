// Qt lib import
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// JQLibrary lib import
#include <JQImageItem>

// Project lib import
#include "helper.h"

int main(int argc, char **argv)
{
    QGuiApplication app( argc, argv );

    JQIMAGEITEM_REGISTERTYPE;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty( "Helper", new Helper );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    return app.exec();
}
