#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$SCRIPT_DIR/PKGBUILD"
. /etc/makepkg.conf

cd "$SCRIPT_DIR/packaging/arch/"
cp "$SCRIPT_DIR/PKGBUILD" ./PKGBUILD
git archive --format=tar.gz --prefix="fex-${pkgver}/" HEAD > "fex-${pkgver}.tar.gz"
rm -f fexrepo.db* fexrepo.files* *.pkg.tar.zst
makepkg -sf --sign &&
repo-add -s -k "$GPGKEY" fexrepo.db.tar.gz *.pkg.tar.zst &&
scp *.sig fexrepo.db* fexrepo.files* *.pkg.tar.zst "$REPOHOSTING":/srv/http/fexrepo/x86_64/
