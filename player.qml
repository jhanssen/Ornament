import QtQuick 1.0
import AudioDevice 1.0
import AudioPlayer 1.0
import MusicModel 1.0

Rectangle {
    SystemPalette { id: activePalette }

    function playFile(filename) {
        if (filename === "")
            return;
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
        if (cur !== -1 && cur + 1 < musicModel.trackCount())
            playFile(musicModel.filenameByPosition(cur + 1))
    }

    function playPrevious() {
        var cur = musicModel.positionFromFilename(audioPlayer.filename)
        if (cur > 0)
            playFile(musicModel.filenameByPosition(cur - 1))
    }

    width: 200
    height: 200

    color: "#f0f0f0"

    AudioDevice {
        id: audioDevice
    }

    AudioPlayer {
        id: audioPlayer

        onStateChanged: {
            if (state === AudioPlayer.Playing) {
                playButton.image = "icons/pause.svg"

                var cur = musicModel.positionFromFilename(audioPlayer.filename)
                if (cur !== -1)
                    list.currentIndex = cur;
            } else if (state === AudioPlayer.Done) {
                playNext()
            } else
                playButton.image = "icons/play.svg"
        }
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
    }

    Component {
        id: highlight
        Rectangle {
            color: "lightsteelblue"; radius: 3
            y: list.currentItem ? list.currentItem.y : 0
            Behavior on y {
                SpringAnimation {
                    spring: 3
                    damping: 0.2
                }
            }
        }
    }

    ListView {
        id: list
        clip: true

        anchors.left: buttons.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        model: musicModel
        highlight: highlight
        focus: true

        property int currentMusicId
        property int currentMouseButton

        onCurrentIndexChanged: {
            if (currentIndex === -1) {
                highlightItem.opacity = 0
                highlightItem.y = 0
            } else
                highlightItem.opacity = 1
        }

        onOpacityChanged: {
            if (opacity !== 0) {
                return
            }

            list.currentIndex = -1

            if (currentMouseButton === Qt.RightButton) {
                if (musicModel.currentAlbum !== 0)
                    musicModel.currentAlbum = 0
                else if (musicModel.currentArtist !== 0)
                    musicModel.currentArtist = 0

                fadeIn.start()

                return
            }

            if (musicModel.currentArtist === 0)
                musicModel.currentArtist = currentMusicId
            else if (musicModel.currentAlbum === 0)
                musicModel.currentAlbum = currentMusicId

            var cur = musicModel.positionFromFilename(audioPlayer.filename)
            if (cur !== -1)
                list.currentIndex = cur;

            fadeIn.start()
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton

            onClicked: {
                list.currentMouseButton = mouse.button
                fadeOut.start()
            }
        }

        SequentialAnimation {
            id: fadeOut
            NumberAnimation { target: list; property: "opacity"; from: 1; to: 0; duration: 200 }
        }

        SequentialAnimation {
            id: fadeIn
            NumberAnimation { target: list; property: "opacity"; from: 0; to: 1; duration: 200 }
        }

        delegate: Item {
            id: delegate
            width: delegate.ListView.view.width
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
                    width: 150

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onPressed: {
                            list.currentIndex = musicindex
                        }

                        onClicked: {
                            if (mouse.button === Qt.LeftButton && musicModel.currentArtist !== 0 && musicModel.currentAlbum !== 0) {
                                playFile(musicModel.filenameById(musicid))
                                return
                            }

                            list.currentMusicId = musicid
                            list.currentMouseButton = mouse.button
                            fadeOut.start()
                        }
                    }
                }
            }
        }
    }
}
