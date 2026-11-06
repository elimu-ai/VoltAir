#!/usr/bin/env bash
# Place at: .github/scripts/dismiss-android-dialog.sh
#
# Polls the emulator UI and taps "OK" on the system dialog
#   "This app was built for an older version of Android and may not work properly..."
# Safe to run in the background while the app is launched; exits 0 even if the
# dialog never appears, so it can never fail the job.
#
# Usage: dismiss-android-dialog.sh [timeout_seconds]

TIMEOUT="${1:-180}"
END=$((SECONDS + TIMEOUT))

find_ok_button() {
  python3 - "$1" <<'PY'
import re, sys
import xml.etree.ElementTree as ET

try:
    root = ET.parse(sys.argv[1]).getroot()
except Exception:
    sys.exit(1)

nodes = list(root.iter("node"))
texts = " ".join(n.get("text", "") for n in nodes)
# Only act on the compat-warning dialog, not on any random "OK" button.
if "older version of Android" not in texts:
    sys.exit(1)

for n in nodes:
    if n.get("resource-id") == "android:id/button1" or n.get("text", "").strip().upper() == "OK":
        m = re.match(r"\[(\d+),(\d+)\]\[(\d+),(\d+)\]", n.get("bounds", ""))
        if m:
            x1, y1, x2, y2 = map(int, m.groups())
            print((x1 + x2) // 2, (y1 + y2) // 2)
            sys.exit(0)
sys.exit(1)
PY
}

echo "Waiting up to ${TIMEOUT}s for the 'older version of Android' dialog..."
while [ "$SECONDS" -lt "$END" ]; do
  if adb shell uiautomator dump /sdcard/window_dump.xml >/dev/null 2>&1 \
     && adb pull /sdcard/window_dump.xml /tmp/window_dump.xml >/dev/null 2>&1; then
    if coords=$(find_ok_button /tmp/window_dump.xml); then
      echo "Dialog found; tapping OK at: $coords"
      # shellcheck disable=SC2086
      adb shell input tap $coords
      exit 0
    fi
  fi
  sleep 1
done

echo "Dialog did not appear within ${TIMEOUT}s (nothing to dismiss)."
exit 0
