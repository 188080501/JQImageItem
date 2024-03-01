import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import JQImageItem 1.0

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    color: "#000000"

    JQImageItem {
        id: imageItem
        anchors.fill: parent

        Component.onCompleted: {
            Helper.setImageItem( this );
        }
    }

    JQImageItem2 {
        id: imageItem2
        anchors.fill: parent
        visible: false
    }

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        checkable: true
        checked: true
        text: ( checked ) ? ( "Item" ) :( "Item2" )

        onClicked: {
            if ( checked )
            {
                Helper.setImageItem( imageItem );
                Helper.setImageItem2( null );

                imageItem.visible  = true;
                imageItem2.visible = false;
            }
            else
            {
                Helper.setImageItem( null );
                Helper.setImageItem2( imageItem2 );

                imageItem.visible  = false;
                imageItem2.visible = true;
            }
        }
    }
}
