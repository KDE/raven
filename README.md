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
* Fetching emails with IMAP (SSL/STARTTLS)
* Viewing both plain text and HTML emails
* Incremental sync and IMAP IDLE waiting support
* OAuth2 for Gmail/Outlook/Yahoo login
* Conversation threading
* Attachment viewing and downloading
* Email notifications
* System tray integration
* Account credential storage via SecretService

## Compiling

Along with the typical KDE application dependencies, Rust and [corrosion](https://github.com/corrosion-rs/corrosion) (for using Rust with CMake) need to be installed.

```
mkdir build
cd build
cmake .. # add -DCMAKE_BUILD_TYPE=Release to compile for release
make
```

## Locations

Data (mail db and downloaded attachments) is stored to `~/.local/share/raven`.

Accounts and configs are stored in `~/.config/raven`.

Email secrets are stored with in the system's secrets service (ex. KWallet).

## Architecture

The application consists of a standard Qt/QML frontend application [raven](/src/raven), and a Rust-based syncing daemon [ravend](/src/ravend).

Mail data is stored in a SQLite database in `~/.local/share/raven`.

#### Client app (`raven`)

The client is a typical Kirigami application, that queries the SQLite database and requests actions to the daemon over DBus.

The thread viewer page is a single QtWebEngine view for rendering HTML emails. It uses a single webview to save on resources when viewing long threads (as opposed to a webview per message). Communication with the rest of the app is done over QtWebChannel.

### Daemon (`ravend`)

The daemon is a background service written in Rust that handles all email synchronization and IMAP operations.

It runs one worker thread per account, each maintaining a persistent IMAP connection. Workers use IMAP IDLE (if supported) to listen to new mail for syncing, falling back to polling when IDLE is not supported.

The daemon is the single writer to the database - the frontend only reads from SQLite and requests operations (mark read, delete, etc.) via DBus.

## TODO
* Mailbox search
* Infinite scroll (fetch more from inbox as you scroll down)
* SMTP support
* Other enterprise logins (Kerberos, GTLM, NSSAPI)
