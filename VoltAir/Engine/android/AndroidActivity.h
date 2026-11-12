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

#ifndef ANDROIDACTIVITY_H
#define ANDROIDACTIVITY_H

#include <QObject>

#if defined(Q_OS_ANDROID)

#include <GameInput/JoystickAxisCodes.h>
#include <GameInput/KeyCodes.h>
#include <GameInput/TriggerAxisCodes.h>
#include <QEvent>
#include <QMap>
#include <QSet>
#include <jni.h>
#include "SelfDetachingJNIEnv.h"

class ControllerEvent;
class QKeyEvent;

/**
 * @ingroup Engine
 * @brief JNI interface for the main Android activity.
 *
 * This static class contains both native callbacks used by the Android activity and methods that
 * invoke their Java counterpart.
 * @note Users of this class should be aware of the different calling threads and contexts which
 * exist. Native callbacks are executing on the Android Ui (i.e. main activity looper thread), but
 * this thread is typically synchronized against the native Qt Ui thread and should not generally
 * pose thread-safety issues.
 * @note Native callbacks first two parameters are always the current JNI environment (a @c JNIEnv
 * pointer) and a reference (a @c jobject) to the invoking Android activity, unless the function has
 * no additional parameters in which case they can be removed. The convention assumed for this class
 * is to make either of these first two parameters nameless if they are unused in the callback (i.e.
 * the callback does not make calls back across the JNI boundary into Java).
 */
class AndroidActivity {
public:
    /**
     * @brief Returns the current JNI environment.
     * @note This method will cache the JVM reference so future calls do not need to provide it.
     * @param vm JavaVM to load the current environment from
     */
    static SelfDetachingJNIEnv getEnv(JavaVM* vm = nullptr);
    /**
     * @brief Returns the currently cached reference to the JVM.
     */
    static JavaVM* getJavaVM() { return sJavaVM; }

    /**
     * @brief Returns the currently cached reference to the Android activity.
     */
    static jobject getActivity() { return sActivity; }
    /**
     * @brief Returns a cached global reference to the Android activity class.
     * @note If currently unavailable, this method will load the activity class from @p env by
     * either looking through the @c CLASSPATH for the class or by retrieving the class of @p
     * activity if it is provided.
     * @param env Environment to load the activity class from
     * @param activity Optional activity object to use for class loading
     */
    static jclass getActivityClass(JNIEnv* env, jobject activity = nullptr);

    /**
     * @brief Returns whether or not the Android activity has been started previously, meaning all
     * future JNI calls should be safe to make.
     */
    static bool isStarted() { return sActivity; }

    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onCreate lifecycle callback.
     * @param jni Current JNI environment
     * @param activity Object reference to invoking Android activity
     */
    static void onCreate(JNIEnv* jni, jobject activity);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onStart lifecycle callback.
     */
    static void onStart();
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onResume lifecycle callback.
     */
    static void onResume();
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onPause lifecycle callback.
     */
    static void onPause();
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onStop lifecycle callback.
     */
    static void onStop();
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onDestroy lifecycle callback.
     * @param jni Current JNI environment
     * @param activity Object reference to invoking Android activity
     */
    static void onDestroy(JNIEnv* jni, jobject activity);

    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onControllerDisconnect callback indicating that a device
     * identified by @p deviceId has disconnected.
     * @param deviceId Id of hardware device which has become disconnected
     */
    static void onControllerDisconnect(JNIEnv*, jobject, int deviceId);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onGamepadMotionEvent callback for an Android @c MotionEvent that
     * originated from a gamepad device.
     * @param jni Current JNI environment
     * @param motionEvent Object reference to the Android @c MotionEvent
     * @returns @c true if the event was handled
     */
    static bool onGamepadMotionEvent(JNIEnv* jni, jobject, jobject motionEvent);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onGamepadKeyEvent callback for an Android @c KeyEvent that
     * originated from a gamepad device.
     * @param jni Current JNI environment
     * @param keyEvent Object reference to the Android @c KeyEvent
     * @returns @c true if the event was handled
     */
    static bool onGamepadKeyEvent(JNIEnv* jni, jobject, jobject keyEvent);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onTouchNavigationMotionEvent callback for an Android @c
     * MotionEvent that originated from a touch navigation device.
     * @param jni Current JNI environment
     * @param motionEvent Object reference to the Android @c MotionEvent
     * @returns @c true if the event was handled
     */
    static bool onTouchNavigationMotionEvent(JNIEnv* jni, jobject, jobject motionEvent);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onTouchNavigationKeyEvent callback for an Android @c KeyEvent that
     * originated from a touch navigation device.
     * @param jni Current JNI environment
     * @param keyEvent Object reference to the Android @c KeyEvent
     * @returns @c true if the event was handled
     */
    static bool onTouchNavigationKeyEvent(JNIEnv* jni, jobject, jobject keyEvent);
    /**
     * @ingroup JNINativeMethod
     * @brief Android activity @c onKeyboardKeyEvent callback for an Android @c KeyEvent that
     * originated from a keyboard device.
     * @param jni Current JNI environment
     * @param keyEvent Object reference to the Android @c KeyEvent
     * @returns @c true if the event was handled
     */
    static bool onKeyboardKeyEvent(JNIEnv* jni, jobject, jobject keyEvent);

    /**
     * @brief Launch an intent to show the GPGS achievements activity screen.
     */
    static void showAchievementsRequested();
    /**
     * @brief Resets all GPGS achievement progress for the currently signed in user account.
     * @note This function is for debug / testing purposes only and will not work on GPGS accounts
     * no longer in staging.
     */
    static void resetAchievementsRequested();
    /**
     * @brief Returns the version string of the Android application.
     */
    static QString getVersionName();
    /**
     * @brief Returns the number of bytes allocated for the native heap.
     */
    static long getMemoryUsed();
    /**
     * @brief Returns the hardware deviceId of the touch screen input device, or -1 if none.
     * @note If multiple touch screen devices are present, this returns the id of the first one
     * discovered.
     */
    static int getTouchScreenDeviceId();
    /**
     * @brief Clears the Android SoundManager's BGM track at the specified priority level.
     * @note See SoundManager::clearBGMTrack for more details.
     * @param priority Priority level of BGM to clear
     */
    static void clearBGMTrack(int priority);
    /**
     * @brief Sets the Android SoundManager's BGM track at the specified priority level to @p track.
     * @note See SoundManager::setBGMTrack for more details.
     * @param priority Priority level of BGM to set
     * @param track Name of track to use for BGM
     */
    static void setBGMTrack(int priority, const QString& track);
    /**
     * @brief Returns whether or not the Android SoundManager's background music is muted.
     * @note See SoundManager::isBGMMuted for more details.
     */
    static bool isBGMMuted();
    /**
     * @brief Sets the Android SoundManager's muted state.
     * @param value @c true if muting BGM
     */
    static void setBGMMuted(bool value);
    /**
     * @brief Sets the Android SoundManager's paused state.
     * @note See SoundManager::setPaused for more details.
     * @param value @c true if pausing BGM
     */
    static void setBGMPaused(bool value);

    /**
     * @brief Android activity @c onStudentUpdateReceiver callback for a StudentUpdate bradcast message.
     * The letters are stored on SharedPreferences.
     * @param env Current JNI environment
     * @param obj
     * @param availableLetters String that stores the available letters for the student
     */
    static void onStudentUpdateReceiver(JNIEnv *env, jobject obj, jstring availableLetters);

    /**
     * @brief Android activity @c validLetters returns the letters sent by the broadcast message some time ago. If
     * no broadcast message was sent, then a default value is retrieved.
     * If a broadcast message was received before, then the returned letters are retrieve from SharedPreferences.
     * @return the letters
     */
    static QString validLetters();

    /**
     * @brief Reports a letter-sound learning event to the elimu.ai Analytics app, by invoking the
     * Android activity's @c reportLetterSoundLearningEvent method.
     * @param letter The letter that the student collected, e.g. "e"
     */
    static void reportLetterSoundLearningEvent(const QString& letter);

private:
    static bool onKeyEvent(JNIEnv* jni, jobject keyEvent, ControllerEvent* controllerEvent);
    static jclass getSoundManagerClass(JNIEnv* jni, jobject soundManager = nullptr);
    static jclass getMotionEventClass(JNIEnv* jni, jobject motionEvent = nullptr);
    static jclass getKeyEventClass(JNIEnv* jni, jobject keyEvent = nullptr);
    static jclass getInputDeviceClass(JNIEnv* jni, jobject inputDevice = nullptr);
    static jclass getMotionRangeClass(JNIEnv* jni, jobject motionRange = nullptr);
    static jclass getDebugClass(JNIEnv* jni);

    static int getMotionEventDeviceId(JNIEnv* jni, jobject motionEvent);
    static KeyCode getKeyCode(JNIEnv* jni, jobject keyEvent);
    static int getKeyAction(JNIEnv* jni, jobject keyEvent);
    static int getKeyEventDeviceId(JNIEnv* jni, jobject keyEvent);
    static jobject getInputDevice(JNIEnv* jni, jobject motionEvent);
    static jobject getMotionRange(JNIEnv* jni, jobject motionEvent, int axis);
    /**
     * Returns the axis value and the motion range values associated with the axis (if out params
     * are non-null).  If flatOut param is null, then the axis value returned is pre-flattened.
     */
    static float getAxisValue(JNIEnv* jni, jobject motionEvent, int axis, float* flatOut = nullptr,
            float* minOut = nullptr, float* rangeOut = nullptr);
    /**
     * Converts the axis value to be on a real scale from [-1, 1] for Joystick use.
     */
    static float getNormalizedJoystickAxisValue(JNIEnv* jni, jobject motionEvent, int axis);
    /**
     * Returns whether or not we should accept and consume Android events which have been translated
     * and possibly routed to VirtualControllers.
     */
    static bool shouldConsumeTranslatedEvents();
    static QKeyEvent* convertAndroidKeyEvent(int action, KeyCode keyCode);
    static jobject getSoundManager();

    static QMap<int, JoystickAxisCode> sGamepadJoystickAxisConversionMap;
    static QMap<int, TriggerAxisCode> sGamepadTriggerAxisConversionMap;
    static QMap<int, JoystickAxisCode> sTouchNavigationJoystickAxisConversionMap;
    static QMap<KeyCode, int> sKeycodeConversionMap;
    static QMap<int, QEvent::Type> sActionConversionMap;
    static QSet<KeyCode> sUnmappableKeycodes;

    static jobject sActivity;

    static jclass sActivityClass;
    static jclass sSoundManagerClass;
    static jclass sMotionEventClass;
    static jclass sKeyEventClass;
    static jclass sInputDeviceClass;
    static jclass sMotionRangeClass;
    static jclass sDebugClass;

    static JavaVM* sJavaVM;
};

#endif // Q_OS_ANDROID
#endif // ANDROIDACTIVITY_H
