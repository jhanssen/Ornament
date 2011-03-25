import QtQuick 1.0
import AudioDevice 1.0
import AudioPlayer 1.0
import MediaModel 1.0
import MusicModel 1.0

Rectangle {
    id: topLevel

    property string songTitle: ""
    property string windowTitle: ""

    SystemPalette { id: activePalette }

    function playFile(filename) {
        if (filename === "")
            return
        audioDevice.device = audioDevice.devices[0]
        audioPlayer.audioDevice = audioDevice
        audioPlayer.filename = filename
        audioPlayer.play()
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

    width: 350
    height: 300

    color: "#f0f0f0"

    AudioDevice {
        id: audioDevice
    }

    AudioPlayer {
        id: audioPlayer

        onStateChanged: {
            topLevel.songTitle = ""

            if (state === AudioPlayer.Playing) {
                if (artworkContainer.opacity < 1)
                    artworkFadeIn.start()

                playButton.image = "icons/pause.svg"

                var cur = musicModel.positionFromFilename(audioPlayer.filename)
                if (cur !== -1)
                    list.currentIndex = cur

                // ### this is not the best way of getting the current track name I'm sure
                topLevel.songTitle = musicModel.tracknameFromFilename(audioPlayer.filename)
            } else if (state === AudioPlayer.Done) {
                if (!playNext() && artworkContainer.opacity > 0) {
                    artworkFadeOut.start()
                    playButton.image = "icons/play.svg"
                }
            } else {
                if (state === AudioPlayer.Stopped && artworkContainer.opacity > 0)
                    artworkFadeOut.start()
                playButton.image = "icons/play.svg"
            }

            var title = topLevel.windowTitle
            if (topLevel.songTitle.length > 0) {
                if (title.length > 0)
                    title += " "
                title += "(P: " + topLevel.songTitle + ")"
            }
            audioPlayer.windowTitle = title
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

        Component.onCompleted: { state = "music" }

        states: [
            State {
                name: "music"
            },
            State {
                name: "media"
                AnchorChanges { target: mediaButton; anchors.top: minusButton.bottom }
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

        PropertyAnimation { id: artworkFadeOut; target: artworkContainer; property: "opacity"; from: 1; to: 0; duration: 200 }
        PropertyAnimation { id: artworkFadeIn; target: artworkContainer; property: "opacity"; from: 0; to: 1; duration: 200 }
        SequentialAnimation {
            id: artworkChange
            PropertyAnimation { target: artworkContainer; property: "opacity"; from: 1; to : 0; duration: 200 }
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
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 1; to: 0; duration: 200 }
                        ScriptAction { script: { list.model = mediaModel; list.delegate = mediaDelegate } }
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0; to: 1; duration: 200 }
                    }
                },
                Transition {
                    from: "media"; to: "music"
                    SequentialAnimation {
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 1; to: 0; duration: 200 }
                        ScriptAction { script: { list.delegate = musicDelegate; list.model = musicModel } }
                        PropertyAnimation { target: listWrapper; property: "opacity"; from: 0; to: 1; duration: 200 }
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

                        var title

                        if (listWrapper.currentMouseButton === Qt.RightButton) {
                            // ### a lot of the code for Qt.RightButton here is duplicated below. Needs a fix!

                            if (musicModel.currentAlbumId !== -1)
                                musicModel.currentAlbumId = -1
                            else if (musicModel.currentArtistId !== -1)
                                musicModel.currentArtistId = -1

                            if (musicModel.currentArtistId > 0) {
                                if (musicModel.currentAlbumId > 0)
                                    topLevel.windowTitle = musicModel.currentArtist.name + " - " + musicModel.currentAlbum.name
                                else
                                    topLevel.windowTitle = musicModel.currentArtist.name
                            } else
                                topLevel.windowTitle = ""

                            title = topLevel.windowTitle
                            if (topLevel.songTitle.length > 0) {
                                if (title.length > 0)
                                    title += " "
                                title += "(P: " + topLevel.songTitle + ")"
                            }
                            audioPlayer.windowTitle = title

                            return
                        }

                        if (musicModel.currentArtistId === -1)
                            musicModel.currentArtistId = listWrapper.currentMusicId
                        else if (musicModel.currentAlbumId === -1)
                            musicModel.currentAlbumId = listWrapper.currentMusicId

                        if (musicModel.currentArtistId > 0) {
                            if (musicModel.currentAlbumId > 0)
                                topLevel.windowTitle = musicModel.currentArtist.name + " - " + musicModel.currentAlbum.name
                            else
                                topLevel.windowTitle = musicModel.currentArtist.name
                        } else
                            topLevel.windowTitle = ""

                        title = topLevel.windowTitle
                        if (topLevel.songTitle.length > 0) {
                            if (title.length > 0)
                                title += " "
                            title += "(P: " + topLevel.songTitle + ")"
                        }
                        audioPlayer.windowTitle = title

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
}
