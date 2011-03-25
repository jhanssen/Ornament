import QtQuick 1.0

Rectangle {
    id: buttonClass

    signal clicked

    property string image
    property string text

    width: 32
    height: 32

    color: parent.color
    border.color: "#cccccc"
    border.width: 2
    radius: 4

    anchors.margins: 9
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.right: parent.right

    MouseArea {
        anchors.fill: parent
        onClicked: buttonClass.clicked()
    }

    Image {
        anchors.fill: parent
        anchors.margins: 4
        source: buttonClass.image
        smooth: true
    }

    Text {
        anchors.fill: parent
        anchors.margins: 4
        text: buttonClass.text
        color: "#eeeeee"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.bold: true
    }
}
