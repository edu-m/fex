source /etc/makepkg.conf

makepkg -sf --sign &&
repo-add -s -k "$GPGKEY" fexrepo.db.tar.gz *.pkg.tar.zst &&
scp *.sig fexrepo.db* fexrepo.files* *.pkg.tar.zst "$REPOHOSTING":/srv/http/fexrepo/x86_64/
