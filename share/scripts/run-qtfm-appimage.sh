#!/bin/sh
# Run QtFM AppImage when FUSE mount fails (common on some Arch/Manjaro setups).
set -e
APPIMAGE="${1:?Usage: run-qtfm-appimage.sh /path/to/qtfm-*.AppImage}"
shift
export APPIMAGE_EXTRACT_AND_RUN=1
# Match fm/src/main.cpp: prefer X11 on Wayland so DnD works with Thunar/Electron.
if [ -z "${QT_QPA_PLATFORM:-}" ] && [ -z "${QTFM_NATIVE_WAYLAND:-}" ]; then
  if [ -n "${WAYLAND_DISPLAY:-}" ] || [ "${XDG_SESSION_TYPE:-}" = "wayland" ]; then
    export QT_QPA_PLATFORM=xcb
  fi
fi
exec "$APPIMAGE" "$@"
