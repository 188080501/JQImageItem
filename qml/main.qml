import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import JQImageItem 1.0

Window {
    id: window
    width: 720
    height: 720
    visible: true
    color: "#000000"

    JQImageItem {
        id: imageItem
        anchors.fill: parent
        visible: false
    }

    JQImageItem2 {
        id: imageItem2
        anchors.fill: parent
        visible: false
    }

    MouseArea {
        anchors.fill: parent

        function update() {
            imageItem.visible  = !imageItem.visible;
            imageItem2.visible = !imageItem.visible;

            if ( imageItem.visible )
            {
                Helper.setImageItem( imageItem );
                Helper.setImageItem2( null );
            }
            else
            {
                Helper.setImageItem( null );
                Helper.setImageItem2( imageItem2 );
            }
        }

        onClicked: {
            update();
        }

        Component.onCompleted: {
            update();
        }
    }

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

            if ( imageItem.visible )
            {
                result += "\nJQImageItem";
            }
            else if ( imageItem2.visible )
            {
                result += "\nJQImageItem2";
            }

            return result;
        }
    }
}
