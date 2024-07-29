import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import JQImageItem 1.0

Window {
    id: window
    width: 720
    height: 720
    visible: true
    color: "#eef3fa"

    Component.onDestruction: {
        Helper.stop();
    }

    // 基于QQuickFramebufferObject
    JQImageItem {
        anchors.fill: parent

        Component.onCompleted: {
            Helper.setImageItem( this );
        }
    }

    // 基于QQuickPaintedItem
    // JQImageItem2 {
    //     anchors.fill: parent

    //     Component.onCompleted: {
    //         Helper.setImageItem2( this );
    //     }
    // }

    Text {
        x: 15
        y: 15
        font.pixelSize: 15
        color: "#ff00ff"
        text: {
            var result = "";

            result += window.width.toString();
            result += " x ";
            result += window.height.toString();

            return result;
        }
    }
}
