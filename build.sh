#!/bin/zsh
# Builds Simpleton (VST3 + AU) and installs it into ~/Library/Audio/Plug-Ins.
set -euo pipefail
cd "$(dirname "$0")"
export PATH="$HOME/.local/bin:$PATH"

if [ ! -d libs/JUCE ]; then
    git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE libs/JUCE
fi

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

echo "Installed:"
echo "  $HOME/Library/Audio/Plug-Ins/VST3/Simpleton.vst3"
echo "  $HOME/Library/Audio/Plug-Ins/Components/Simpleton.component"
