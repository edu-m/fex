# /etc/profile.d/fex.sh
# fex shell integration (bash-compatible)

fex() {
  /usr/bin/fex_exec "${1:-.}" || return
  if [ -f "$HOME/.fexlastdir" ]; then
    target="$(cat "$HOME/.fexlastdir")"
    if [ -n "$target" ] && [ -d "$target" ]; then
      cd "$target" || return
    fi
  fi
}
