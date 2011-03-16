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

    width: 200
    height: 200

    color: "#f0f0f0"

    AudioDevice {
        id: audioDevice
    }

    AudioPlayer {
        id: audioPlayer

        onStateChanged: {
            if (state == AudioPlayer.Playing)
                playButton.image = "icons/pause.svg"
            else if (state == AudioPlayer.Done) {
                // play next file
                console.log("play next?")
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

        Button { id: playButton; image: "icons/play.svg"; onClicked: { pauseOrPlayFile(musicModel.firstFilename()) } }
        Button { id: stopButton; image: "icons/stop.svg"; anchors.top: playButton.bottom; onClicked: { audioPlayer.stop() } }
        Button { id: prevButton; image: "icons/skip-backward.svg"; anchors.top: stopButton.bottom }
        Button { id: nextButton; image: "icons/skip-forward.svg"; anchors.top: prevButton.bottom }
    }

    ListView {
        id: list
        clip: true

        anchors.left: buttons.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        model: musicModel
        highlight: Rectangle { color: "lightsteelblue"; radius : 2 }
        focus: true

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton

            onClicked: {
                if (musicModel.currentAlbum !== 0)
                    musicModel.currentAlbum = 0
                else if (musicModel.currentArtist !== 0)
                    musicModel.currentArtist = 0
            }
        }

        delegate: Item {
            id: delegate
            width: delegate.ListView.view.width
            height: 30
            clip: true
            anchors.margins: 4

            Row {
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

                        onClicked: {
                            if (mouse.button === Qt.RightButton) {
                                if (musicModel.currentAlbum !== 0)
                                    musicModel.currentAlbum = 0
                                else if (musicModel.currentArtist !== 0)
                                    musicModel.currentArtist = 0
                                return
                            }

                            if (musicModel.currentArtist === 0)
                                musicModel.currentArtist = musicid
                            else if (musicModel.currentAlbum === 0)
                                musicModel.currentAlbum = musicid
                            else {
                                playFile(musicModel.filename(musicid))
                            }
                        }
                    }
                }
            }
        }
    }
}
