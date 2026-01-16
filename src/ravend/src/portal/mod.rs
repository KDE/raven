// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

use std::collections::HashMap;
use zbus::{proxy, Connection};
use zbus::zvariant::{OwnedValue, Value};

#[proxy(
    interface = "org.freedesktop.portal.Background",
    default_service = "org.freedesktop.portal.Desktop",
    default_path = "/org/freedesktop/portal/desktop"
)]
trait BackgroundPortal {
    fn request_background(
        &self,
        parent_window: &str,
        options: HashMap<&str, Value<'_>>,
    ) -> zbus::Result<zbus::zvariant::OwnedObjectPath>;
}

#[proxy(
    interface = "org.freedesktop.portal.Request",
    default_service = "org.freedesktop.portal.Desktop"
)]
trait PortalRequest {
    #[zbus(signal)]
    fn response(&self, response: u32, results: HashMap<String, OwnedValue>) -> zbus::Result<()>;
}

/// Returns (background_granted, autostart_granted) on success
pub async fn request_background_permission() -> Option<(bool, bool)> {
    use futures_util::StreamExt;

    let connection = Connection::session().await.ok()?;
    let portal = BackgroundPortalProxy::new(&connection).await.ok()?;

    let options = HashMap::from([
        ("reason", Value::from("Raven Mail needs to sync emails in the background")),
        ("autostart", Value::from(true)),
        ("commandline", Value::from(vec!["ravend".to_string()])),
        ("dbus-activatable", Value::from(true)),
    ]);

    let path = portal.request_background("", options).await.ok()?;
    let request = PortalRequestProxy::new(&connection, path).await.ok()?;
    let mut stream = request.receive_response().await.ok()?;

    let signal = tokio::time::timeout(std::time::Duration::from_secs(30), stream.next())
        .await
        .ok()??;

    let args = signal.args().ok()?;
    if args.response != 0 { return None; }

    let bg = args.results.get("background").and_then(|v| v.try_into().ok()).unwrap_or(false);
    let auto = args.results.get("autostart").and_then(|v| v.try_into().ok()).unwrap_or(false);
    Some((bg, auto))
}
