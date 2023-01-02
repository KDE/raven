<!--
- SPDX-FileCopyrightText: None
- SPDX-License-Identifier: CC0-1.0
-->

# Raven Mail <img src="logo.png" width="40" />

An email client for Plasma Mobile, based on MailCore2.

Inspired by [Mailspring-Sync](https://github.com/Foundry376/Mailspring-Sync).

## Links
* Project page: https://invent.kde.org/plasma-mobile/raven
* Issues: https://invent.kde.org/plasma-mobile/raven/issues
* Development channel: https://matrix.to/#/#plasmamobile:matrix.org

## Dependencies
* ctemplate
* tidy
* libetpan
* Qt Quick Controls
* Kirigami
* Kirigami Addons

## Compiling

```
mkdir build
cd build
cmake .. # add -DCMAKE_BUILD_TYPE=Release to compile for release
make
```

## Locations

Data is stored to `~/.local/share/raven`.

Configs are stored in `~/.config/raven`.

## TODO
* Use KWallet to store secrets
* OAuth2 support
* SMTP support
