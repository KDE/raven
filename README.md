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
* Promotional page: https://raven.espi.dev/

## Features
* Fetching emails with IMAP (SSL/STARTTLS)
* Viewing both plain text and HTML emails
* IMAP IDLE for power-efficient push notifications
* Incremental sync (only fetches new messages)
* OAuth2 for Gmail/Outlook/Yahoo login
* Conversation threading
* Attachment viewing and downloading
* Desktop notifications for new emails
* System tray integration
* Secure credential storage via Secret Service API

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

Email secrets are stored with in the system's secrets service (ex. KWallet).

## Architecture

The application consists of a standard Qt/QML frontend application [raven](/src/raven), and a Rust-based syncing daemon [ravend](/src/ravend).

Mail data is stored in a SQLite database in `~/.local/share/raven`.

### Daemon (`ravend`)

The daemon is a background service written in Rust that handles all email synchronization and IMAP operations.

It runs one worker thread per account, each maintaining a persistent IMAP connection. Workers use IMAP IDLE to wait for new mail efficiently, falling back to polling when IDLE is not supported.

The daemon is the single writer to the database - the frontend only reads from SQLite and requests operations (mark read, delete, etc.) via D-Bus. This ensures data consistency and allows the frontend to remain responsive while sync operations happen in the background.

## Background

This project was created as a stopgap solution until KDE PIM is ready for mobile. The daemon was rapidly developed with the help of an LLM.

The backend architecture is heavily inspired by [Mailspring-Sync](https://github.com/Foundry376/Mailspring-Sync).

## TODO
* SMTP support
* Other enterprise logins (Kerberos, GTLM, NSSAPI)
