# Copyright (c) 2025 Eduardo Meli
# Released under GNU GPL 3.0 license. Refer to LICENSE or visit <http://www.gnu.org/licenses/>
#!/bin/bash

sudo cp ./fex_exec /usr/local/bin/fex_exec

user_shell=$(basename "$SHELL")

if [[ "$user_shell" == "bash" ]]; then
    config_file="$HOME/.bashrc"
elif [[ "$user_shell" == "zsh" ]]; then
    config_file="$HOME/.zshrc"
else
    echo "Unsupported shell: $user_shell"
    exit 1
fi

read -r -d '' func_def <<'EOF'

# Added by fex setup
fex() {
    /usr/local/bin/fex_exec
    if [ -f ~/.fexlastdir ]; then
        cd "$(cat ~/.fexlastdir)"
    fi
}
EOF

if grep -q "fex()" "$config_file"; then
    echo "fex function already exists in $config_file"
else
    echo "Appending fex function to $config_file"
    echo "$func_def" >> "$config_file"
fi

echo "Setup complete. Restart your terminal or run 'source $config_file' to use fex."

