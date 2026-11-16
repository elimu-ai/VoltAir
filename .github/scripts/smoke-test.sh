#!/usr/bin/env bash
set -euo pipefail

APK="${1:-VoltAir.apk}"
FAILED=0

AAPT="$(ls -d "$ANDROID_HOME"/build-tools/*/aapt | sort -V | tail -1)"
PKG="$("$AAPT" dump badging "$APK" | sed -n "s/^package: name='\([^']*\)'.*/\1/p")"
echo "Package: $PKG"

# The CI emulator is x86_64 and advertises arm64-v8a via NDK translation, so
# a fat APK installs its arm64 slice there — but Qt's arm64 code crashes under
# translation (F DEBUG in qtMainLoopThread), while armeabi-v7a runs fine.
# Pin the translated ABI when ADB_INSTALL_ABI is set (CI); unset installs the
# device-preferred ABI (e.g. arm64 on real 64-bit hardware).
ADB_INSTALL_ABI="${ADB_INSTALL_ABI:-}"
if [ -n "$ADB_INSTALL_ABI" ]; then
  adb install -g -r --abi "$ADB_INSTALL_ABI" "$APK"
else
  adb install -g -r "$APK"
fi
adb logcat -c
adb shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1

# Give the game 30s to start, and check it stays alive the whole time.
for i in $(seq 1 10); do
  sleep 3
  if ! adb shell pidof "$PKG" > /dev/null; then
    echo "::error::$PKG is not running after $((i * 3))s (crashed or exited)"
    FAILED=1
    break
  fi
done

adb exec-out screencap -p > launch.png || true
adb logcat -d > logcat.txt || true

# The crash buffer holds Java exceptions and native signals. Filter by
# package so unrelated system crashes on the emulator don't fail the job.
if adb logcat -d -b crash | grep -F "$PKG"; then
  echo "::error::Crash reported for $PKG"
  FAILED=1
fi

exit "$FAILED"
