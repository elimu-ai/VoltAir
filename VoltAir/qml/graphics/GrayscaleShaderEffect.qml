/*
 * Copyright (C) 2014 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import QtQuick 2.2
import QtQuick.Effects
import VoltAir 1.0

/**
 * @ingroup QQuickItem
 * @brief A shader effect that renders an item in grayscale.
 */
Item {
    id: root

    /**
     * @brief Item to be rendered in grayscale.
     */
    property Item sourceItem
    /**
     * @brief type:real Proportion between the original colors and greyscale to output.
     *
     * @note @c 0.0 indicates no grayscaling.  @c 1.0 is completly monochrome.
     */
    property real ratio: 1.0

    // When no grayscaling is requested, bypass the effect entirely so the
    // source subtree (which may itself contain MultiEffects) renders
    // directly. Hiding the source unconditionally would force even unlocked
    // cards through a proxy, breaking nested mask effects.
    function updateVisibility() {
        if (!sourceItem) {
            return
        }
        sourceItem.visible = !(ratio > 0.01)
    }
    onSourceItemChanged: updateVisibility()
    onRatioChanged: updateVisibility()
    Component.onCompleted: updateVisibility()

    MultiEffect {
        anchors.fill: parent
        source: root.sourceItem
        saturation: -root.ratio
        visible: root.ratio > 0.01
    }
}
