#!/usr/bin/env bash
set -euo pipefail

APK="${1:-VoltAir.apk}"
FAILED=0

AAPT="$(ls -d "$ANDROID_HOME"/build-tools/*/aapt | sort -V | tail -1)"
PKG="$("$AAPT" dump badging "$APK" | sed -n "s/^package: name='\([^']*\)'.*/\1/p")"
echo "Package: $PKG"

adb install -r "$APK"
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
