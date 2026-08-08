#!/bin/sh
# Run QtFM AppImage when FUSE mount fails (common on some Arch/Manjaro setups).
set -e
APPIMAGE="${1:?Usage: run-qtfm-appimage.sh /path/to/qtfm-*.AppImage}"
shift
export APPIMAGE_EXTRACT_AND_RUN=1
# Optional: QTFM_FORCE_X11=1 for XWayland targets (Thunar). Default stays native.
if [ -z "${QT_QPA_PLATFORM:-}" ] && [ -n "${QTFM_FORCE_X11:-}" ]; then
  export QT_QPA_PLATFORM=xcb
fi
exec "$APPIMAGE" "$@"
