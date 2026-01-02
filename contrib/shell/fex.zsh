# /etc/profile.d/fex.zsh
# fex shell integration (zsh)

fex() {
  /usr/bin/fex_exec "${1:-.}" || return
  if [[ -f "$HOME/.fexlastdir" ]]; then
    local target
    target="$(cat "$HOME/.fexlastdir")"
    if [[ -n "$target" && -d "$target" ]]; then
      cd "$target" || return
    fi
  fi
}
