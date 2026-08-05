#!/usr/bin/env bash
# Pack a portable Linux "bin" tree that links against system Qt / Magick / etc.
# Does NOT bundle Qt or ImageMagick — use AppImage for a self-contained build.
#
# Usage:
#   pack-linux-bin.sh <install-root> <output-dir> [version] [arch]
#
# <install-root> is the DESTDIR/INSTALL_ROOT used with `make install`
# (e.g. AppDir with usr/bin/qtfm, or a plain prefix root).
#
# Produces:
#   <output-dir>/qtfm-<version>-linux-<arch>-bin.tar.xz
#   containing bin/, share/, README.txt
set -euo pipefail

INSTALL_ROOT="${1:?Usage: $0 <install-root> <output-dir> [version] [arch]}"
OUT_DIR="${2:?Usage: $0 <install-root> <output-dir> [version] [arch]}"
VERSION="${3:-6.3.0}"
ARCH="${4:-$(uname -m)}"

case "${ARCH}" in
  x86_64|amd64) ARCH=x86_64 ;;
  aarch64|arm64) ARCH=aarch64 ;;
esac

INSTALL_ROOT="$(cd "${INSTALL_ROOT}" && pwd)"
mkdir -p "${OUT_DIR}"
OUT_DIR="$(cd "${OUT_DIR}" && pwd)"

PKG_NAME="qtfm-${VERSION}-linux-${ARCH}-bin"
STAGE="${OUT_DIR}/${PKG_NAME}"
rm -rf "${STAGE}"
mkdir -p "${STAGE}/bin" "${STAGE}/share"

find_bin() {
  local name="$1"
  if [ -x "${INSTALL_ROOT}/usr/bin/${name}" ]; then
    echo "${INSTALL_ROOT}/usr/bin/${name}"
  elif [ -x "${INSTALL_ROOT}/bin/${name}" ]; then
    echo "${INSTALL_ROOT}/bin/${name}"
  elif [ -x "${INSTALL_ROOT}/usr/local/bin/${name}" ]; then
    echo "${INSTALL_ROOT}/usr/local/bin/${name}"
  else
    return 1
  fi
}

QTFM_BIN="$(find_bin qtfm)" || {
  echo "qtfm binary not found under ${INSTALL_ROOT}" >&2
  exit 1
}

cp -a "${QTFM_BIN}" "${STAGE}/bin/qtfm"
chmod 755 "${STAGE}/bin/qtfm"
if command -v strip >/dev/null 2>&1; then
  strip --strip-unneeded "${STAGE}/bin/qtfm" 2>/dev/null || true
fi

if TRAY_BIN="$(find_bin qtfm-tray)"; then
  cp -a "${TRAY_BIN}" "${STAGE}/bin/qtfm-tray"
  chmod 755 "${STAGE}/bin/qtfm-tray"
  if command -v strip >/dev/null 2>&1; then
    strip --strip-unneeded "${STAGE}/bin/qtfm-tray" 2>/dev/null || true
  fi
fi

# Prefer PREFIX=/usr layout from make install
copy_share_tree() {
  local src="$1"
  [ -d "${src}" ] || return 0
  mkdir -p "${STAGE}/share"
  # shellcheck disable=SC2045
  for item in "${src}"/*; do
    [ -e "${item}" ] || continue
    cp -a "${item}" "${STAGE}/share/"
  done
}

if [ -d "${INSTALL_ROOT}/usr/share" ]; then
  copy_share_tree "${INSTALL_ROOT}/usr/share"
elif [ -d "${INSTALL_ROOT}/share" ]; then
  copy_share_tree "${INSTALL_ROOT}/share"
elif [ -d "${INSTALL_ROOT}/usr/local/share" ]; then
  copy_share_tree "${INSTALL_ROOT}/usr/local/share"
fi

# Shared libQtFM only if release used CONFIG+=sharedlib
if [ -d "${INSTALL_ROOT}/usr/lib" ] || [ -d "${INSTALL_ROOT}/usr/lib64" ] \
   || [ -d "${INSTALL_ROOT}/lib" ] || [ -d "${INSTALL_ROOT}/lib64" ]; then
  for libroot in \
      "${INSTALL_ROOT}/usr/lib64" \
      "${INSTALL_ROOT}/usr/lib" \
      "${INSTALL_ROOT}/lib64" \
      "${INSTALL_ROOT}/lib"
  do
    [ -d "${libroot}" ] || continue
    while IFS= read -r -d '' so; do
      mkdir -p "${STAGE}/lib"
      cp -a "${so}" "${STAGE}/lib/"
      # Also copy soname links next to the real file
      base="$(dirname "${so}")"
      name="$(basename "${so}")"
      for link in "${base}"/libQtFM.so*; do
        [ -e "${link}" ] || continue
        cp -a "${link}" "${STAGE}/lib/" 2>/dev/null || true
      done
      unset name base
    done < <(find "${libroot}" -maxdepth 2 -name 'libQtFM.so*' -print0 2>/dev/null || true)
  done
fi

# Refuse to ship bundled Qt (this package is system-linked only).
if [ -d "${STAGE}/usr" ] || ls "${STAGE}"/lib/libQt5*.so* >/dev/null 2>&1; then
  echo "Refusing to pack bundled Qt libraries into system bin package." >&2
  exit 1
fi

# Document dynamic deps (system packages).
{
  echo "QtFM ${VERSION} — Linux bin (system libraries)"
  echo
  echo "This archive does NOT include Qt, ImageMagick, FFmpeg, or Poppler."
  echo "Link against libraries already installed on your distribution."
  echo
  echo "Layout:"
  echo "  bin/qtfm              main application"
  if [ -x "${STAGE}/bin/qtfm-tray" ]; then
    echo "  bin/qtfm-tray         tray helper"
  fi
  echo "  share/…               icons, desktop file, translations, mime icons"
  echo
  echo "Run without installing:"
  echo "  ./bin/qtfm"
  echo "  (resolves share/ next to bin/ via ../share/qtfm/…)"
  echo
  echo "Install system-wide (optional):"
  echo "  sudo cp -a bin/qtfm /usr/local/bin/"
  if [ -x "${STAGE}/bin/qtfm-tray" ]; then
    echo "  sudo cp -a bin/qtfm-tray /usr/local/bin/"
  fi
  echo "  sudo cp -a share/* /usr/local/share/"
  echo
  echo "Runtime packages (names vary by distro):"
  echo
  echo "  Debian / Ubuntu:"
  echo "    libqt5widgets5 libqt5gui5 libqt5core5a libqt5dbus5 libqt5concurrent5"
  echo "    libqt5svg5"
  echo "    libmagick++-6.q16-8   # or matching Magick++ for your release"
  echo "    ffmpeg poppler-utils   # thumbnails (optional but recommended)"
  echo "    adwaita-icon-theme hicolor-icon-theme"
  echo
  echo "  Arch / Manjaro:"
  echo "    qt5-base qt5-svg"
  echo "    imagemagick"
  echo "    ffmpeg poppler"
  echo "    adwaita-icon-theme hicolor-icon-theme"
  echo
  echo "  Fedora:"
  echo "    qt5-qtbase qt5-qtsvg"
  echo "    ImageMagick-c++"
  echo "    ffmpeg poppler-utils"
  echo "    adwaita-icon-theme hicolor-icon-theme"
  echo
  echo "Linked libraries (from build host ldd):"
  echo
  if command -v ldd >/dev/null 2>&1; then
    ldd "${STAGE}/bin/qtfm" | sed 's/^/  /' || true
  else
    echo "  (ldd not available)"
  fi
  echo
  echo "Build note: release uses static libQtFM by default, so only system"
  echo "Qt / Magick / libc appear in ldd — no libQtFM.so required."
} > "${STAGE}/README.txt"

# Sanity checks
test -x "${STAGE}/bin/qtfm"
test -d "${STAGE}/share/qtfm/mimes" || test -d "${STAGE}/share/icons" || {
  echo "warning: share/qtfm/mimes missing — icons may rely on embedded resources only" >&2
}

TAR="${OUT_DIR}/${PKG_NAME}.tar.xz"
rm -f "${TAR}"
(
  cd "${OUT_DIR}"
  tar -cJf "${PKG_NAME}.tar.xz" "${PKG_NAME}"
)

echo "Created ${TAR}"
ls -lah "${TAR}"
file "${STAGE}/bin/qtfm" || true
ldd "${STAGE}/bin/qtfm" | head -20 || true
