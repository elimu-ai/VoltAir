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
 * @brief A shader effect that masks one texture with another.
 */
Item {
    id: root

    /**
     * @brief Item that is masked by #maskItem.
     */
    property Item sourceItem
    /**
     * @brief Item that masks #sourceItem.
     */
    property Item maskItem

    // Mirror the original hideSource:true behavior for both inputs: the
    // source and mask items are texture inputs only and must not render
    // directly, otherwise the raw mask color shows through.
    // The mask is a BorderImage, which MultiEffect cannot sample directly;
    // forcing a layer makes it usable as a texture source.
    function updateVisibility() {
        if (sourceItem) {
            sourceItem.visible = false
        }
        if (maskItem) {
            maskItem.visible = false
            maskItem.layer.enabled = true
        }
    }
    onSourceItemChanged: updateVisibility()
    onMaskItemChanged: updateVisibility()
    Component.onCompleted: updateVisibility()

    MultiEffect {
        anchors.fill: parent
        source: root.sourceItem
        maskEnabled: true
        maskSource: root.maskItem
    }
}
