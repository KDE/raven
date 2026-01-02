// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

//! XDG Desktop Portal integration for background/autostart permissions
//!
//! This module implements the org.freedesktop.portal.Background portal
//! to properly request permission to run in the background on modern
//! Linux desktops (especially important for Flatpak environments).

use log::{debug, error, info, warn};
use std::collections::HashMap;
use zbus::{proxy, Connection};
use zbus::zvariant::{OwnedValue, Value};

/// Portal response codes
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PortalResponse {
    /// User granted permission
    Success = 0,
    /// User cancelled/denied
    Cancelled = 1,
    /// Other error
    Other = 2,
}

impl From<u32> for PortalResponse {
    fn from(value: u32) -> Self {
        match value {
            0 => PortalResponse::Success,
            1 => PortalResponse::Cancelled,
            _ => PortalResponse::Other,
        }
    }
}

/// Result of a background portal request
#[derive(Debug)]
pub struct BackgroundResult {
    pub response: PortalResponse,
    pub background_granted: bool,
    pub autostart_granted: bool,
}

/// D-Bus proxy for the XDG Background Portal
#[proxy(
    interface = "org.freedesktop.portal.Background",
    default_service = "org.freedesktop.portal.Desktop",
    default_path = "/org/freedesktop/portal/desktop"
)]
trait BackgroundPortal {
    /// Request permission to run in the background
    fn request_background(
        &self,
        parent_window: &str,
        options: HashMap<&str, Value<'_>>,
    ) -> zbus::Result<zbus::zvariant::OwnedObjectPath>;
}

/// D-Bus proxy for portal Request objects
#[proxy(
    interface = "org.freedesktop.portal.Request",
    default_service = "org.freedesktop.portal.Desktop"
)]
trait PortalRequest {
    /// Signal emitted when the request completes
    #[zbus(signal)]
    fn response(&self, response: u32, results: HashMap<String, OwnedValue>) -> zbus::Result<()>;

    /// Close the request (cancel it)
    fn close(&self) -> zbus::Result<()>;
}

/// Request background and autostart permissions via the XDG portal
pub async fn request_background_permission() -> Option<BackgroundResult> {
    debug!("Checking XDG Background Portal availability...");

    let connection = match Connection::session().await {
        Ok(conn) => conn,
        Err(e) => {
            warn!("Failed to connect to session bus for portal: {}", e);
            return None;
        }
    };

    let proxy = match BackgroundPortalProxy::new(&connection).await {
        Ok(p) => p,
        Err(e) => {
            // Portal may not be available (e.g., on systems without portal support)
            debug!("Background portal not available: {}", e);
            return None;
        }
    };

    // Build options for the request
    let mut options: HashMap<&str, Value<'_>> = HashMap::new();

    // Reason shown to the user
    options.insert("reason", Value::from("Raven Mail needs to sync emails in the background"));

    // Request autostart permission
    options.insert("autostart", Value::from(true));

    // Command line for autostart (the daemon executable)
    let commandline: Vec<String> = vec!["ravend".to_string()];
    options.insert("commandline", Value::from(commandline));

    // Not D-Bus activatable
    options.insert("dbus-activatable", Value::from(false));

    info!("Requesting background and autostart permissions via XDG portal...");

    // Make the request
    let request_path = match proxy.request_background("", options).await {
        Ok(path) => path,
        Err(e) => {
            warn!("Failed to request background permission: {}", e);
            return None;
        }
    };

    debug!("Portal request path: {}", request_path);

    // Build the request proxy
    let request_proxy = match PortalRequestProxy::builder(&connection)
        .path(request_path.clone())
    {
        Ok(builder) => match builder.build().await {
            Ok(p) => p,
            Err(e) => {
                error!("Failed to build request proxy: {}", e);
                return None;
            }
        },
        Err(e) => {
            error!("Failed to create request proxy builder: {}", e);
            return None;
        }
    };

    // Wait for the response signal with a timeout
    let mut response_stream = match request_proxy.receive_response().await {
        Ok(stream) => stream,
        Err(e) => {
            error!("Failed to receive portal response stream: {}", e);
            return None;
        }
    };

    // Wait for response with timeout
    use futures_util::StreamExt;
    let timeout_duration = std::time::Duration::from_secs(30);
    let response_result = tokio::time::timeout(timeout_duration, response_stream.next()).await;

    match response_result {
        Ok(Some(signal)) => {
            let args = match signal.args() {
                Ok(a) => a,
                Err(e) => {
                    error!("Failed to parse portal response: {}", e);
                    return None;
                }
            };

            let response_code = PortalResponse::from(args.response);
            let results = args.results;

            // Parse the results - try to extract boolean values
            let background_granted = extract_bool(&results, "background").unwrap_or(false);
            let autostart_granted = extract_bool(&results, "autostart").unwrap_or(false);

            let result = BackgroundResult {
                response: response_code,
                background_granted,
                autostart_granted,
            };

            match response_code {
                PortalResponse::Success => {
                    info!(
                        "Portal permission granted - background: {}, autostart: {}",
                        background_granted, autostart_granted
                    );
                }
                PortalResponse::Cancelled => {
                    warn!("User denied background permission request");
                }
                PortalResponse::Other => {
                    warn!("Portal request failed with unknown response");
                }
            }

            Some(result)
        }
        Ok(None) => {
            warn!("Portal response stream ended without response");
            None
        }
        Err(_) => {
            warn!("Portal request timed out after {:?}", timeout_duration);
            None
        }
    }
}

/// Extract a boolean value from portal results
fn extract_bool(results: &HashMap<String, OwnedValue>, key: &str) -> Option<bool> {
    results.get(key).and_then(|v| {
        // Try to downcast to bool
        if let Ok(b) = <bool>::try_from(v) {
            Some(b)
        } else {
            None
        }
    })
}

/// Check if we're running in a sandboxed environment (Flatpak)
pub fn is_sandboxed() -> bool {
    std::path::Path::new("/.flatpak-info").exists()
}
