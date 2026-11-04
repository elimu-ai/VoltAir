#!/usr/bin/env bash
# Reads and updates the app version stored in AndroidManifest.xml.
#
# Development versions carry a "-SNAPSHOT" suffix on versionName (e.g.
# 1.0.5-SNAPSHOT) so builds from working branches are easy to tell apart from
# releases (1.0.5), which are the only versions that get a tag.
#
#   version.sh get
#       Print versionName exactly as it is in the manifest.
#   version.sh release
#       Drop the "-SNAPSHOT" suffix in place and print the release version.
#       Fails if the current version is not a snapshot.
#   version.sh bump <patch|minor|major>
#       Move to the next development version (X.Y.Z-SNAPSHOT) in place and
#       print it. Works from a snapshot or a release version, e.g. for patch:
#       1.0.5 -> 1.0.6-SNAPSHOT.
#
# versionCode is derived from the numeric part only, so 1.0.5 and
# 1.0.5-SNAPSHOT share a code (existing scheme, 1.0.4 -> 1000004):
#   versionCode = major * 1000000 + minor * 1000 + patch
#
# Override the manifest location with the MANIFEST environment variable.
set -euo pipefail

MANIFEST="${MANIFEST:-VoltAir/Engine/android/deploy/AndroidManifest.xml}"

die() { echo "::error::$*" >&2; exit 1; }

[ -f "$MANIFEST" ] || die "Manifest not found: $MANIFEST"

read_name() { sed -n 's/.*android:versionName="\([^"]*\)".*/\1/p' "$MANIFEST" | head -n 1; }
read_code() { sed -n 's/.*android:versionCode="\([^"]*\)".*/\1/p' "$MANIFEST" | head -n 1; }

code_for() { echo $(( $1 * 1000000 + $2 * 1000 + $3 )); }

write_version() { # $1 = versionName, $2 = versionCode
  sed -i \
    -e "s/android:versionName=\"[^\"]*\"/android:versionName=\"$1\"/" \
    -e "s/android:versionCode=\"[^\"]*\"/android:versionCode=\"$2\"/" \
    "$MANIFEST"
}

parse_and_validate() {
  NAME="$(read_name)"
  CODE="$(read_code)"
  [[ "$NAME" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)(-SNAPSHOT)?$ ]] \
    || die "versionName '$NAME' in $MANIFEST is not MAJOR.MINOR.PATCH or MAJOR.MINOR.PATCH-SNAPSHOT"
  MAJOR="${BASH_REMATCH[1]}"; MINOR="${BASH_REMATCH[2]}"; PATCH="${BASH_REMATCH[3]}"
  SUFFIX="${BASH_REMATCH[4]}"
  [ "$MINOR" -lt 1000 ] && [ "$PATCH" -lt 1000 ] \
    || die "minor/patch must be < 1000 for the versionCode scheme (got $NAME)"
  EXPECTED="$(code_for "$MAJOR" "$MINOR" "$PATCH")"
  [ "$CODE" = "$EXPECTED" ] \
    || die "versionCode '$CODE' does not match versionName '$NAME' (expected $EXPECTED)"
}

cmd="${1:-}"
case "$cmd" in
  get)
    parse_and_validate
    echo "$NAME"
    ;;
  release)
    parse_and_validate
    [ -n "$SUFFIX" ] \
      || die "versionName '$NAME' is not a -SNAPSHOT version, so there is nothing to release. Set the next development version first (e.g. $MAJOR.$MINOR.$((PATCH + 1))-SNAPSHOT)."
    RELEASE="$MAJOR.$MINOR.$PATCH"
    write_version "$RELEASE" "$CODE"
    echo "$RELEASE"
    ;;
  bump)
    part="${2:-patch}"
    parse_and_validate
    case "$part" in
      patch) PATCH=$((PATCH + 1)) ;;
      minor) MINOR=$((MINOR + 1)); PATCH=0 ;;
      major) MAJOR=$((MAJOR + 1)); MINOR=0; PATCH=0 ;;
      *) die "Unknown bump type '$part' (use patch, minor or major)" ;;
    esac
    NEW_NAME="$MAJOR.$MINOR.$PATCH-SNAPSHOT"
    write_version "$NEW_NAME" "$(code_for "$MAJOR" "$MINOR" "$PATCH")"
    echo "$NEW_NAME"
    ;;
  *)
    echo "Usage: $0 get | release | bump <patch|minor|major>" >&2
    exit 2
    ;;
esac
