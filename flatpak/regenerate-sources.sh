#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later

set -e

export GIT_CLONE_ARGS="--depth 1 --single-branch"
export FLATPAK_DIR="$(readlink -f $(dirname $0))"
cd ${FLATPAK_DIR}

if [ ! -d flatpak-builder-tools ]; then
        git clone ${GIT_CLONE_ARGS} https://github.com/flatpak/flatpak-builder-tools
else
	git -C flatpak-builder-tools pull
fi

./flatpak-builder-tools/cargo/flatpak-cargo-generator.py -o generated-sources.json ../src/ravend/Cargo.lock
