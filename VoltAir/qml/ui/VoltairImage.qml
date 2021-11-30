import QtGraphicalEffects 1.0
import QtQuick 2.0
import VoltAir 1.0

/**
 * @ingroup QQuickItem
 * @brief Container for Image with default style settings and a drop shadow.
*/

Item {
    id: root

    property string sourceImage

    /**
     * @brief type:Text Reference to the internal @c Text element that is used by this VoltAirText.
     */
    //property alias textElement: text
    /**
     * @brief type:DropShadow Reference to the internal @c DropShadow element that is used by this VoltAirText.
     */
    property alias dropShadow: dropShadow

    FontLoader {
        id: voltAirFont
        source: Util.getPathToFont("AndikaLowerCase-Regular_5dp.ttf")
    }

    Image {
        id: image
        source: sourceImage

        anchors.fill: parent
        height: 0.8 * parent.height


        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        fillMode: Image.PreserveAspectFit
    }

    DropShadow {
        id: dropShadow

        anchors.fill: image
        source: image

        horizontalOffset: 1
        verticalOffset: 2
        radius: 6.0
        spread: 0.6
        samples: 16
        color: "white"
    }
}
