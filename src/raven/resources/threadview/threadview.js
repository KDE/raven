/*
 * SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Thread View JavaScript - handles WebChannel communication and rendering
 */

let bridge = null;
let currentMessages = [];

// Initialize WebChannel connection
function initWebChannel() {
    if (typeof qt === 'undefined' || typeof qt.webChannelTransport === 'undefined') {
        console.error('Qt WebChannel not available');
        showError('WebChannel not available');
        return;
    }

    new QWebChannel(qt.webChannelTransport, function(channel) {
        bridge = channel.objects.threadBridge;

        if (!bridge) {
            console.error('ThreadViewBridge not found in WebChannel');
            showError('Bridge not available');
            return;
        }

        // Listen for thread load
        bridge.threadLoaded.connect(renderThread);

        // Listen for theme changes
        bridge.themeChanged.connect(updateTheme);

        // Apply initial theme
        updateTheme();

        console.log('WebChannel initialized successfully');
    });
}

// Update theme CSS from Qt
function updateTheme() {
    if (!bridge) return;

    const themeCSS = bridge.themeCSS;

    // Update main page theme
    const themeStyle = document.getElementById('theme-style');
    if (themeStyle) {
        themeStyle.textContent = themeCSS;
    }
}

// Render the thread with all messages
function renderThread(messagesJson) {
    try {
        currentMessages = JSON.parse(messagesJson);
    } catch (e) {
        console.error('Failed to parse messages JSON:', e);
        showError('Failed to load messages');
        return;
    }

    const container = document.getElementById('messages');
    const subjectEl = document.getElementById('subject');

    if (!container) return;

    container.innerHTML = '';

    if (currentMessages.length === 0) {
        showEmpty();
        return;
    }

    // Set subject from first message
    if (subjectEl && currentMessages.length > 0) {
        subjectEl.textContent = currentMessages[0].subject || '';
    }

    // Render each message
    currentMessages.forEach((msg, index) => {
        const card = createMessageCard(msg, index);
        container.appendChild(card);
    });

    // Scroll to top
    window.scrollTo(0, 0);
}

// Escape HTML to prevent XSS
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text || '';
    return div.innerHTML;
}

// Create a message card element
function createMessageCard(msg, index) {
    const isLastMessage = index === currentMessages.length - 1;
    const card = document.createElement('div');
    card.className = isLastMessage ? 'message-card' : 'message-card collapsed';
    card.dataset.messageId = msg.id;

    // Build recipients HTML
    let recipientsHtml = '';
    if (msg.to) {
        recipientsHtml += `<div class="message-recipients-row">
            <span class="recipients-label">To:</span>
            <span class="recipients-value">${escapeHtml(msg.to)}</span>
        </div>`;
    }
    if (msg.cc) {
        recipientsHtml += `<div class="message-recipients-row">
            <span class="recipients-label">CC:</span>
            <span class="recipients-value">${escapeHtml(msg.cc)}</span>
        </div>`;
    }
    if (msg.bcc) {
        recipientsHtml += `<div class="message-recipients-row">
            <span class="recipients-label">BCC:</span>
            <span class="recipients-value">${escapeHtml(msg.bcc)}</span>
        </div>`;
    }

    // Build attachment bar HTML
    let attachmentBarHtml = '';
    if (msg.attachmentCount > 0) {
        const attachmentItemsHtml = msg.attachments
            .filter(att => !att.isInline)
            .map(att => `<div class="attachment-item" data-file-id="${escapeHtml(att.id)}">
                <div class="attachment-icon">&#128206;</div>
                <div class="attachment-info">
                    <span class="attachment-name">${escapeHtml(att.filename || 'Attachment')}</span>
                    <span class="attachment-size">${escapeHtml(att.formattedSize || '')}</span>
                </div>
                <div class="attachment-actions">
                    <button class="attachment-btn attachment-save-btn" title="Save attachment">&#128190;</button>
                </div>
            </div>`).join('');

        attachmentBarHtml = `<div class="attachment-bar">
            <div class="attachment-bar-header">Attachments (${msg.attachmentCount})</div>
            <div class="attachment-list">${attachmentItemsHtml}</div>
        </div>`;
    }

    card.innerHTML = `
        <div class="message-header">
            <div class="message-header-row">
                <span class="collapse-indicator">&#9654;</span>
                <span class="message-from">${escapeHtml(msg.from)}</span>
                <span class="message-date">${escapeHtml(msg.dateFormatted)}</span>
            </div>
            <div class="message-snippet">${escapeHtml(msg.snippet)}</div>
            <div class="message-recipients">${recipientsHtml}</div>
        </div>
        <div class="message-content">
            ${msg.isPlaintext
                ? `<pre class="message-plaintext">${escapeHtml(msg.content)}</pre>`
                : '<iframe class="message-content-frame" sandbox="allow-same-origin" scrolling="no"></iframe>'}
            ${attachmentBarHtml}
        </div>`;

    // Set iframe srcdoc for HTML content (can't be set via innerHTML)
    if (!msg.isPlaintext) {
        const iframe = card.querySelector('.message-content-frame');
        iframe.srcdoc = createIframeContent(msg.content || '');
        iframe.onload = function() {
            resizeIframe(iframe);
        };
    }

    // Set up header click handler
    const header = card.querySelector('.message-header');
    header.onclick = function(e) {
        if (e.target.tagName === 'A') return;
        const selection = window.getSelection();
        if (selection && selection.toString().length > 0) return;

        card.classList.toggle('collapsed');
        if (!card.classList.contains('collapsed')) {
            const iframe = card.querySelector('.message-content-frame');
            if (iframe) resizeIframe(iframe);
        }
    };

    // Set up attachment click handlers
    card.querySelectorAll('.attachment-item').forEach(item => {
        const fileId = item.dataset.fileId;
        item.onclick = function() { openAttachment(fileId); };
    });
    card.querySelectorAll('.attachment-save-btn').forEach(btn => {
        const fileId = btn.closest('.attachment-item').dataset.fileId;
        btn.onclick = function(e) {
            e.stopPropagation();
            saveAttachment(fileId);
        };
    });

    return card;
}

// Create iframe content with styles
function createIframeContent(html) {
    // Get current theme CSS
    const themeCSS = bridge ? bridge.themeCSS : '';

    return `<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        html, body {
            margin: 0;
            padding: 16px 16px;
            overflow-x: auto;
            overflow-y: hidden;
        }
        img {
            max-width: 100%;
            height: auto;
        }
        pre, code {
            white-space: pre-wrap;
            word-wrap: break-word;
        }
    </style>
</head>
<body>${html}</body>
</html>`;
}

// Resize iframe to fit content
function resizeIframe(iframe) {
    try {
        const doc = iframe.contentDocument || iframe.contentWindow.document;
        if (doc && doc.body) {
            // Temporarily set overflow visible to get accurate measurements
            doc.body.style.overflowY = 'visible';
            doc.documentElement.style.overflowY = 'visible';

            // Get content dimensions using multiple methods to ensure accuracy
            const bodyHeight = doc.body.scrollHeight;
            const docHeight = doc.documentElement.scrollHeight;
            const bodyOffset = doc.body.offsetHeight;
            const docOffset = doc.documentElement.offsetHeight;

            // Use the maximum of all measurements and add buffer for safety
            const height = Math.max(bodyHeight, docHeight, bodyOffset, docOffset);
            iframe.style.height = Math.max(height + 10, 100) + 'px';

            // Restore overflow-y to hidden (overflow-x stays auto for horizontal scroll)
            doc.body.style.overflowY = 'hidden';
            doc.documentElement.style.overflowY = 'hidden';

            // Intercept link clicks
            interceptLinks(doc);

            // Re-measure after images load
            const images = doc.querySelectorAll('img');
            images.forEach(img => {
                if (!img.complete) {
                    img.onload = () => resizeIframe(iframe);
                }
            });
        }
    } catch (e) {
        console.error('Failed to resize iframe:', e);
    }
}

// Show link tooltip at bottom of screen
function showLinkTooltip(url) {
    const tooltip = document.getElementById('link-tooltip');
    if (tooltip && url) {
        tooltip.textContent = url;
        tooltip.classList.add('visible');
    }
}

// Hide link tooltip
function hideLinkTooltip() {
    const tooltip = document.getElementById('link-tooltip');
    if (tooltip) {
        tooltip.classList.remove('visible');
    }
}

// Intercept link clicks to open in external browser
function interceptLinks(doc) {
    const links = doc.querySelectorAll('a[href]');
    links.forEach(link => {
        link.onclick = function(e) {
            e.preventDefault();
            const href = link.getAttribute('href');
            if (href && bridge) {
                bridge.openExternalUrl(href);
            }
        };

        // Show tooltip on hover
        link.onmouseenter = function() {
            const href = link.getAttribute('href');
            if (href) {
                showLinkTooltip(href);
            }
        };

        link.onmouseleave = function() {
            hideLinkTooltip();
        };
    });
}

// Open attachment
function openAttachment(fileId) {
    if (bridge) {
        bridge.openAttachment(fileId);
    }
}

// Save attachment
function saveAttachment(fileId) {
    if (bridge) {
        bridge.saveAttachment(fileId);
    }
}

// Show loading state
function showLoading() {
    const container = document.getElementById('messages');
    if (container) {
        container.innerHTML = '<div class="loading">Loading...</div>';
    }
}

// Show empty state
function showEmpty() {
    const container = document.getElementById('messages');
    if (container) {
        container.innerHTML = '<div class="empty-state">No messages in this thread</div>';
    }
}

// Show error state
function showError(message) {
    const container = document.getElementById('messages');
    if (container) {
        container.innerHTML = '<div class="empty-state">Error: ' + (message || 'Unknown error') + '</div>';
    }
}

// Initialize on DOM ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initWebChannel);
} else {
    initWebChannel();
}
