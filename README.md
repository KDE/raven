<!--
- SPDX-FileCopyrightText: None
- SPDX-License-Identifier: CC0-1.0
-->

# Raven Mail <img src="logo.png" width="40" />

A simple email client for Plasma Mobile.

It uses an independent Rust-based mail syncing daemon (not Akonadi/KDE PIM).

For a full Akonadi based mail client, see [KMail](https://invent.kde.org/pim/kmail).

## Links
* Project page: https://invent.kde.org/plasma-mobile/raven
* Issues: https://invent.kde.org/plasma-mobile/raven/issues
* Development channel: https://matrix.to/#/#plasmamobile:matrix.org
* Privacy policy: https://kde.org/privacypolicy-apps/

## Features
* Fetching emails with IMAP
* Viewing both plain text and HTML emails in an intuitive interface
* IMAP IDLE for reduced power consumption
* IMAP CONDSTORE/QRESYNC partial sync support for email providers that support it
* OAuth2 for GMail/Outlook/Yahoo login
* Notifications for new incoming email

## Compiling

Along with the typical KDE application dependencies, Rust and [corrosion](https://github.com/corrosion-rs/corrosion) (for using Rust with CMake) need to be installed.

```
mkdir build
cd build
cmake .. # add -DCMAKE_BUILD_TYPE=Release to compile for release
make
```

## Locations

Data is stored to `~/.local/share/raven`.

Accounts and configs are stored in `~/.config/raven`.

Email secrets are stored with [KWallet](https://invent.kde.org/frameworks/kwallet).

## Architecture

The application consists of a standard Qt frontend application [raven](/src/raven), and a Rust-based syncing daemon [ravend](/src/rs/ravend).

Mail data is stored in a sqlite database in `~/.local/share/raven`.

### Daemon (`ravend`)

The daemon periodically syncs the list of accounts

Only `ravend` modifies the database, with the frontend requesting any persistent operations over DBus.

## Background

This project was created as a stopgap solution until KDE PIM is ready for mobile. The daemon was rapidly developed with the help of an LLM.

The backend architecture is heavily inspired by [Mailspring-Sync](https://github.com/Foundry376/Mailspring-Sync).

## TODO
* SMTP support
* Other enterprise logins (ex. Kerberos?)
