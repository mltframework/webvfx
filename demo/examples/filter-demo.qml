import QtQuick 2.0

Rectangle {
    width: 576;
    height: 432
    color: "lightgray"

    Component.onCompleted: {
        webvfx.imageTypeMap = { "sourceImage" : webvfx.SourceImageType };
        webvfx.readyRender(true);
    }

    Image {
        id: image
        width: parent.width
        height: parent.height
        clip: true
    }
    Text {
        id: timeText
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 24
        font.bold: true
    }
    Connections {
        target: webvfx
        onRenderRequested: {
    	    image.source = webvfx.getImageUrl("sourceImage");
            image.rotation = time * 360;
	        timeText.text = "rotating " + Math.round(image.rotation);
            timeText.rotation = -time * 360;
	    }
    }
}
