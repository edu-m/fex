# Copyright (c) 2025 Eduardo Meli
# Released under GNU GPL 3.0 license. Refer to LICENSE or visit <http://www.gnu.org/licenses/>
#!/bin/bash

gcc main.c xdg.c trie.c -lncurses -lpanel -lm -ofex_exec -Wall -Wextra -pedantic -Os -flto -ffunction-sections -fdata-sections -ffreestanding -fno-stack-protector -Wl,--build-id=none -fwrapv -fomit-frame-pointer -fno-asynchronous-unwind-tables -Wl,--gc-sections -fwhole-program -fno-math-errno -fno-exceptions -nodefaultlibs -lc -fsingle-precision-constant -fmerge-all-constants -fno-ident -fno-math-errno

strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag fex_exec

upx --best fex_exec
