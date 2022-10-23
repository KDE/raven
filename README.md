<!--
- SPDX-FileCopyrightText: None
- SPDX-License-Identifier: CC0-1.0
-->

# Raven Mail <img src="logo.png" width="40" />

An email client for Plasma Mobile, based on Akonadi.

## Links
* Project page: https://invent.kde.org/plasma-mobile/raven
* Issues: https://invent.kde.org/plasma-mobile/raven/issues
* Development channel: https://matrix.to/#/#plasmamobile:matrix.org

## Dependencies
* Akonadi
* [PIM MailCommon](https://invent.kde.org/pim/mailcommon)
* Qt Quick + Controls
* Kirigami
* Kirigami Addons

## Compiling

```
mkdir build
cd build
cmake .. # add -DCMAKE_BUILD_TYPE=Release to compile for release
make
```

## TODO
* Draft and send mail
* Improved content display (HTML and images on by default)
* Welcome screen
* Share code with Kalendar
