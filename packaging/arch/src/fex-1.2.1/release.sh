source /etc/makepkg.conf
source ./PKGBUILD
git archive --format=tar.gz --prefix=fex-${pkgver}/ HEAD > fex-1.2.1.tar.gz
makepkg -sf --sign &&
repo-add -s -k "$GPGKEY" fexrepo.db.tar.gz *.pkg.tar.zst &&
scp *.sig fexrepo.db* fexrepo.files* *.pkg.tar.zst "$REPOHOSTING":/srv/http/fexrepo/x86_64/
