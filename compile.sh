#!/bin/zsh

gcc main.c xdg.c -lncurses -lm -ofex -Os -flto -ffunction-sections -fdata-sections -ffreestanding -fno-stack-protector -Wl,--build-id=none -fwrapv -fomit-frame-pointer -fno-asynchronous-unwind-tables -Wl,--gc-sections -fwhole-program -fno-math-errno -fno-exceptions -nodefaultlibs -lc -fsingle-precision-constant -fmerge-all-constants -fno-ident -fno-math-errno

strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag fex

upx --best fex
