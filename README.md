# fex

`fex` is a small ncurses-based tool that writes the chosen directory to `~/.fexlastdir` on exit.
A shell function then `cd`s into that directory after `fex_exec` closes.

## Build

```sh
make
```

## Install (system)

```sh
sudo make install
```

## Shell integration

Arch-style integration is installed to:

- Bash: `/etc/profile.d/fex.sh`
- Zsh : `/etc/profile.d/fex.zsh`

Many terminal emulators start **non-login** interactive shells, which may not automatically load `/etc/profile.d/*`.
To opt in (recommended), run:

```sh
fex-setup
```

This will ask (y/n) and, if you agree, append a small guarded `source /etc/profile.d/fex.(sh|zsh)` block to your
`~/.bashrc` or `~/.zshrc`. You can remove it later with:

```sh
fex-setup --uninstall
```

## Notes for packaging (Arch Linux)

This repo includes an Arch PKGBUILD under `packaging/arch/`.
It installs to `/usr` and does not automatically modify user dotfiles.
Instead, it prints a post-install message recommending `fex-setup`.

You can also obtain the custom repo to add to your pacman by pasting this into your `/etc/pacman.conf`:

```
[fexrepo]
Server = https://ssh.lambdawiki.org/fexrepo/$arch
```

Feel free to download the public key at this [link](http://ssh.lambdawiki.org/fexrepo/fexrepo-publickey.asc) `SHA256: 0b69962a4d34e603c4bcd20fc8d81cfe2d4454f47c985900ce41f69fedb42673`

Add it to your keys with

```
sudo pacman-key --add fexrepo-publickey.asc
```
