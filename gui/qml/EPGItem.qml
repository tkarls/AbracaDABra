import QtQuick
import Qt5Compat.GraphicalEffects   // required for Qt < 6.5
// import QtQuick.Effects              // not available in Qt < 6.5


Item {
    id: progItem

    property double pointsPerSec: 0.0
    signal clicked
    property bool isSelected: false
    property int itemHeight: 50
    property int viewX: 0

    height: itemHeight
    clip: true
    width: mouseAreaId.containsMouse ? Math.max(textId.x + textId.width + 5, durationSec * pointsPerSec) : durationSec * pointsPerSec
    x:  startTimeSec * pointsPerSec
    z: mouseAreaId.containsMouse ? 1000 : index

    Rectangle {
        id: rect
        property bool textFits: (textId.x + textId.width + 5) <= width
        color: {
            if (endTimeSecSinceEpoch <= currentTimeSec) return "lightgrey";
            if (startTimeSecSinceEpoch >= currentTimeSec) return "white";
            return "lemonchiffon";
        }
        border.color: isSelected ? "black" : "darkgray"
        border.width: 1
        anchors.fill: parent
        x:  startTimeSec * pointsPerSec

        Rectangle {
            id: remainingTime
            visible: (startTimeSecSinceEpoch < currentTimeSec) && (endTimeSecSinceEpoch > currentTimeSec)
            color: "gold"            
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                topMargin: 1
                bottomMargin: 1
                leftMargin: 1
            }
            width: mouseAreaId.containsMouse
                   ? ((currentTimeSec - startTimeSecSinceEpoch) / durationSec) * parent.width - 1
                   : ((currentTimeSec - startTimeSecSinceEpoch) * pointsPerSec - 1)
        }
        Text {
            id: textId
            anchors {
                //left: parent.left
                //leftMargin: 5
                top: parent.top
                bottom: parent.bottom
            }
            x: (viewX > startTimeSec * pointsPerSec) && (viewX < (endTimeSec * pointsPerSec + 20))
                ? (viewX - startTimeSec * pointsPerSec + 5) : (parent.x + 5)
            text: name
            font.bold: isSelected
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
        }
        clip: true
        visible: true
    }

    Rectangle {
        id: shadowItem
        color: "black"
        anchors.left: rect.right
        anchors.top: rect.top
        anchors.bottom: rect.bottom
        width: 100
        visible: false
    }
    // Qt >= 6.5
    /*
    MultiEffect {
        source: shadowItem
        anchors.fill: shadowItem
        autoPaddingEnabled: true
        shadowBlur: 1.0
        shadowColor: 'black'
        shadowEnabled: true
        shadowOpacity: 0.5
        shadowHorizontalOffset: -2 // rect.shadowSize
        shadowVerticalOffset: 0
        visible: !rect.textFits
    }
    */
    DropShadow {
        id: shadowId
        anchors.fill: shadowItem
        source: shadowItem
        samples: 21
        color: "black"
        enabled: true
        opacity: 0.4
        horizontalOffset: -2
        verticalOffset: 0
        visible: !rect.textFits
    }
    MouseArea {
        id: mouseAreaId
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            progItem.clicked()
        }
    }
}