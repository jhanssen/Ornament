/*
    Ornament - A cross plaform audio player
    Copyright (C) 2011  Jan Erik Hanssen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
