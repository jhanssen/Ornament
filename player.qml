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
import AudioDevice 1.0
import AudioPlayer 1.0
import MediaModel 1.0
import MusicModel 1.0

Rectangle {
    id: topLevel

    property string artistName: musicModel.artistnameFromFilename(audioPlayer.filename)
    property string albumName: musicModel.albumnameFromFilename(audioPlayer.filename)
    property string songTitle: musicModel.tracknameFromFilename(audioPlayer.filename)

    SystemPalette { id: activePalette }

    function msToString(ms) {
        var s = ms / 1000
        var m = s / 60
        s -= Math.floor(m) * 60
        var h = Math.floor(m / 60)
        m -= h * 60
        if (s < 10)
            s = "0" + Math.floor(s)
        else
            s = Math.floor(s)
        if (h == 0)
            return Math.floor(m) + "." + s
        if (m < 10)
            m = "0" + Math.floor(m)
        else
            m = Math.floor(m)
        return h + ":" + m + "." + s
    }

    function playFile(filename) {
        if (filename === "")
            return
        audioDevice.device = audioDevice.devices[0]
        audioPlayer.audioDevice = audioDevice
        audioPlayer.filename = filename
        audioPlayer.play()

        var duration = musicModel.durationFromFilename(filename)
        if (duration === 0)
            durationText.text = ""
        else
            durationText.text = msToString(duration)
    }

    function pauseOrPlayFile(filename) {
        if (audioPlayer.state === AudioPlayer.Playing)
            audioPlayer.pause()
        else if (audioPlayer.state === AudioPlayer.Paused)
            audioPlayer.play()
        else // AudioPlayer.Stopped or AudioPlayer.Done
            playFile(filename)
    }

    function playNext() {
        var cur = musicModel.positionFromFilename(audioPlayer.filename)
        if (cur !== -1 && cur + 1 < musicModel.trackCount()) {
            playFile(musicModel.filenameByPosition(cur + 1))
            return true
        }
        audioPlayer.filename = ""
        return false
    }

    function playPrevious() {
        var cur = musicModel.positionFromFilename(audioPlayer.filename)
        if (cur > 0)
            playFile(musicModel.filenameByPosition(cur - 1))
    }

    function toggleMediaList() {
        if (buttons.state === "media") {
            list.state = "music"
            buttons.state = "music"
        } else {
            list.state = "media"
            buttons.state = "media"
        }
    }

    function addMediaItem() {
        mediaModel.addRow()
    }

    function removeMediaItem() {
        if (list.currentIndex !== -1)
            mediaModel.removeRow(list.currentIndex)
    }

    function refreshMediaList() {
        mediaModel.refreshMedia()
    }

    width: 350
    height: 300

    color: "#f0f0f0"

    AudioDevice {
        id: audioDevice
    }

    AudioPlayer {
        id: audioPlayer
        windowTitle: musicModel.tracknameFromFilename(audioPlayer.filename)

        onStateChanged: {
            topLevel.songTitle = ""

            if (state === AudioPlayer.Playing) {
                topLevel.state = "playing"

                var cur = musicModel.positionFromFilename(audioPlayer.filename)
                if (cur !== -1)
                    list.currentIndex = cur
            } else if (state === AudioPlayer.Done) {
                if (!playNext())
                    topLevel.state = "stopped"
            } else if (state === AudioPlayer.Stopped) {
                topLevel.state = "stopped"
            } else if (state === AudioPlayer.Paused) {
                topLevel.state = "paused"
            }

        }

        onPositionChanged: {
            positionText.text = msToString(position)
        }
    }

    MediaModel {
        id: mediaModel
    }

    MusicModel {
        id: musicModel
    }

    Rectangle {
        id: buttons
        clip: true

        color: "#444444"

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 50

        Button { id: playButton; image: "icons/play.svg"; onClicked: { pauseOrPlayFile(musicModel.filenameByPosition(0)) } }
        Button { id: stopButton; image: "icons/stop.svg"; anchors.top: playButton.bottom; onClicked: { audioPlayer.stop() } }
        Button { id: prevButton; image: "icons/skip-backward.svg"; anchors.top: stopButton.bottom; onClicked: { playPrevious() } }
        Button { id: nextButton; image: "icons/skip-forward.svg"; anchors.top: prevButton.bottom; onClicked: { playNext() } }
        Button { id: mediaButton; text: "m"; anchors.top: nextButton.bottom; onClicked: { toggleMediaList() } }
        Button { id: plusButton; text: "+"; anchors.top: parent.top; opacity: 0; onClicked: { addMediaItem() } }
        Button { id: minusButton; text: "-"; anchors.top: plusButton.bottom; opacity: 0; onClicked: { removeMediaItem() } }
        Button { id: refreshButton; text: "r"; anchors.top: minusButton.bottom; opacity: 0; onClicked: { refreshMediaList() } }
        Text { id: positionText; anchors.margins: 9; anchors.top: mediaButton.bottom; anchors.left: parent.left; anchors.right: parent.right; horizontalAlignment: Text.AlignHCenter; color: "#eeeeee" }
        Text { id: durationText; anchors.margins: 9; anchors.top: positionText.bottom; anchors.left: parent.left; anchors.right: parent.right; horizontalAlignment: Text.AlignHCenter; color: "#eeeeee" }

        Component.onCompleted: { state = "music" }

        states: [
            State {
                name: "music"
            },
            State {
                name: "media"
                AnchorChanges { target: mediaButton; anchors.top: refreshButton.bottom }
            }
        ]

        transitions: [
            Transition {
                from: "music"; to: "media"
                ParallelAnimation {
                    PropertyAnimation { target: playButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: stopButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: prevButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: nextButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: plusButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: minusButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: refreshButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    AnchorAnimation { duration: 200 }
                }
            },
            Transition {
                from: "media"; to: "music"
                ParallelAnimation {
                    PropertyAnimation { target: playButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: stopButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: prevButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: nextButton; property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { target: plusButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: minusButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    PropertyAnimation { target: refreshButton; property: "opacity"; from: 1; to: 0; duration: 200 }
                    AnchorAnimation { duration: 200 }
                }
            }
        ]
    }

    Component {
        id: highlight
        Rectangle {
            color: "lightsteelblue"; radius: 3
            y: list.currentItem ? list.currentItem.y : 0
            width: list.width
            Behavior on y {
                SpringAnimation {
                    spring: 3
                    damping: 0.2
                }
            }
        }
    }

    Rectangle {
        id: artworkContainer
        clip: true
        opacity: 0

        anchors.left: buttons.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        property string updateSource

        function updateArtwork() {
            updateSource = "image://artwork/" + audioPlayer.filename
            artworkChange.start()
        }

        Component.onCompleted: {
            audioPlayer.artworkAvailable.connect(updateArtwork)
        }

        Image {
            id: artwork
            fillMode: Image.PreserveAspectCrop
            anchors.fill: parent
        }

        SequentialAnimation {
            id: artworkChange
            PropertyAnimation { target: artworkContainer; property: "opacity"; from: artworkContainer.opacity; to : 0; duration: 200 }
            ScriptAction { script: { artwork.source = artworkContainer.updateSource } }
            PropertyAnimation { target: artworkContainer; property: "opacity"; from: 0; to : 1; duration: 200 }
        }
    }

    Rectangle {
        id: listWrapper
        color: "white"
        opacity: 0.7

        anchors.left: buttons.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        property int currentMusicId
        property int currentMouseButton

        ListView {
            id: list
            clip: true

            anchors.fill: parent

            model: musicModel
            highlight: highlight
            focus: true

            states: [
                State { name: "music" },
                State { name: "media" }
            ]

            transitions: [
                Transition {
                    from: "music"; to: "media"
                    SequentialAnimation {
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0.7; to: 0; duration: 200 }
                        ScriptAction { script: { list.model = mediaModel; list.delegate = mediaDelegate } }
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0; to: 0.7; duration: 200 }
                    }
                },
                Transition {
                    from: "media"; to: "music"
                    SequentialAnimation {
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0.7; to: 0; duration: 200 }
                        ScriptAction { script: { list.delegate = musicDelegate; list.model = musicModel } }
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0; to: 0.7; duration: 200 }
                    }
                }
            ]

            Component.onCompleted: { state = "music" }

            onCurrentIndexChanged: {
                if (currentIndex === -1) {
                    highlightItem.opacity = 0
                    highlightItem.y = 0
                } else
                    highlightItem.opacity = 1
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                hoverEnabled: true

                onPositionChanged: {
                    if (mouse.y < 10)
                        statusWindow.state = "shown"
                    else
                        statusWindow.state = "hidden"
                }

                onExited: {
                    statusWindow.state = "hidden"
                }

                onClicked: {
                    if (list.state === "media")
                        return

                    listWrapper.currentMouseButton = mouse.button
                    fadeList.start()
                }
            }

            SequentialAnimation {
                id: fadeList
                NumberAnimation { target: listWrapper; property: "opacity"; from: 0.7; to: 0; duration: 200 }
                ScriptAction {
                    script: {
                        list.currentIndex = -1

                        if (listWrapper.currentMouseButton === Qt.RightButton) {
                            if (musicModel.currentAlbumId !== -1)
                                musicModel.currentAlbumId = -1
                            else if (musicModel.currentArtistId !== -1)
                                musicModel.currentArtistId = -1

                            return
                        }

                        if (musicModel.currentArtistId === -1)
                            musicModel.currentArtistId = listWrapper.currentMusicId
                        else if (musicModel.currentAlbumId === -1)
                            musicModel.currentAlbumId = listWrapper.currentMusicId

                        var cur = musicModel.positionFromFilename(audioPlayer.filename)
                        if (cur !== -1)
                            list.currentIndex = cur
                    }
                }
                NumberAnimation { target: listWrapper; property: "opacity"; from: 0; to: 0.7; duration: 200 }
            }

            delegate: musicDelegate
        }

        Component {
            id: musicDelegate

            Item {
                width: list.width
                height: 30
                clip: true
                anchors.margins: 4

                Row {
                    id: row
                    anchors.margins: 4
                    anchors.fill: parent
                    spacing: 4

                    Text {
                        id: value

                        text: musicitem
                        width: parent.width
                        height: parent.height

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            onPressed: {
                                list.currentIndex = musicindex
                            }

                            onClicked: {
                                if (mouse.button === Qt.LeftButton) {
                                    // -1 is no artist or album selected, 0 is all tracks for the album/artist selected
                                    if ((musicModel.currentArtistId !== -1 && musicModel.currentAlbumId !== -1)
                                            || (musicModel.currentArtistId === 0 && musicModel.currentAlbumId === -1)) {
                                        playFile(musicModel.filenameById(musicid))
                                        return
                                    }
                                }

                                listWrapper.currentMusicId = musicid
                                listWrapper.currentMouseButton = mouse.button
                                fadeList.start()
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: mediaDelegate

            Item {
                width: list.width
                height: 30
                clip: true
                anchors.margins: 4

                Row {
                    anchors.margins: 4
                    anchors.fill: parent
                    spacing: 4

                    Text {
                        text: mediaitem
                        width: parent.width
                        height: parent.height
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onPressed: {
                            list.currentIndex = mediaindex
                        }

                        onClicked: {
                            mediaModel.setPathInRow(mediaindex)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: statusWindow

        Text {
            id: statusArtistText

            anchors.left: parent.left
            anchors.right: parent.right
            height: 20

            clip: true

            horizontalAlignment: Text.AlignHCenter
            color: "#eeeeee"

            text: topLevel.artistName
        }
        Text {
            id: statusAlbumText

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: statusArtistText.bottom
            height: 20

            clip: true

            horizontalAlignment: Text.AlignHCenter
            color: "#eeeeee"

            text: topLevel.albumName
        }
        anchors.bottom: listWrapper.top
        anchors.left: listWrapper.left
        anchors.right: listWrapper.right
        height: 40

        color: "#444444"

        Component.onCompleted: { state = "hidden" }

        states: [
            State {
                name: "shown"
                AnchorChanges { target: statusWindow; anchors.bottom: undefined; anchors.top: listWrapper.top }
            },
            State {
                name: "hidden"
                AnchorChanges { target: statusWindow; anchors.top: undefined; anchors.bottom: listWrapper.top }
            }
        ]
        transitions: Transition {
            AnchorAnimation { duration: 200 }
        }
    }

    states: [
        State {
            name: "playing"
            PropertyChanges { target: playButton; image: "icons/pause.svg" }
            PropertyChanges { target: artworkContainer; opacity: 1 }
        },
        State {
            name: "paused"
            PropertyChanges { target: playButton; image: "icons/play.svg" }
        },
        State {
            name: "stopped"
            PropertyChanges { target: playButton; image: "icons/play.svg" }
            PropertyChanges { target: artworkContainer; opacity: 0 }
        }
    ]
    transitions: Transition {
        PropertyAnimation { target: artworkContainer; property: "opacity"; duration: 200 }
    }
}
