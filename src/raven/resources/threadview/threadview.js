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

// Create a message card element
function createMessageCard(msg, index) {
    const card = document.createElement('div');
    card.className = 'message-card collapsed';
    card.dataset.messageId = msg.id;

    // Header (clickable to expand/collapse)
    const header = createMessageHeader(msg);
    header.onclick = function(e) {
        // Don't toggle if clicking a link
        if (e.target.tagName === 'A') return;

        // Don't toggle if there's a text selection (user is selecting text)
        const selection = window.getSelection();
        if (selection && selection.toString().length > 0) {
            return;
        }

        card.classList.toggle('collapsed');
        // Resize iframe when expanding
        if (!card.classList.contains('collapsed')) {
            const iframe = card.querySelector('.message-content-frame');
            if (iframe) {
                resizeIframe(iframe);
            }
        }
    };
    card.appendChild(header);

    // Content wrapper
    const content = document.createElement('div');
    content.className = 'message-content';
    content.appendChild(createMessageContent(msg));
    card.appendChild(content);

    // Attachment bar (only if there are non-inline attachments)
    if (msg.attachmentCount > 0) {
        content.appendChild(createAttachmentBar(msg));
    }

    // Auto-expand last message in thread
    if (index === currentMessages.length - 1) {
        card.classList.remove('collapsed');
    }

    return card;
}

// Create message header element
function createMessageHeader(msg) {
    const header = document.createElement('div');
    header.className = 'message-header';

    // Main row with from and date
    const mainRow = document.createElement('div');
    mainRow.className = 'message-header-row';

    const collapseIndicator = document.createElement('span');
    collapseIndicator.className = 'collapse-indicator';
    collapseIndicator.innerHTML = '&#9654;'; // Right arrow
    mainRow.appendChild(collapseIndicator);

    const from = document.createElement('span');
    from.className = 'message-from';
    from.textContent = msg.from || '';
    mainRow.appendChild(from);

    const date = document.createElement('span');
    date.className = 'message-date';
    date.textContent = msg.dateFormatted || '';
    mainRow.appendChild(date);

    header.appendChild(mainRow);

    // Snippet (shown when collapsed)
    const snippet = document.createElement('div');
    snippet.className = 'message-snippet';
    snippet.textContent = msg.snippet || '';
    header.appendChild(snippet);

    // Recipients section
    const recipients = document.createElement('div');
    recipients.className = 'message-recipients';

    // To
    if (msg.to) {
        recipients.appendChild(createRecipientsRow('To:', msg.to));
    }

    // CC
    if (msg.cc) {
        recipients.appendChild(createRecipientsRow('CC:', msg.cc));
    }

    // BCC
    if (msg.bcc) {
        recipients.appendChild(createRecipientsRow('BCC:', msg.bcc));
    }

    header.appendChild(recipients);

    return header;
}

// Create a recipients row
function createRecipientsRow(label, value) {
    const row = document.createElement('div');
    row.className = 'message-recipients-row';

    const labelEl = document.createElement('span');
    labelEl.className = 'recipients-label';
    labelEl.textContent = label;
    row.appendChild(labelEl);

    const valueEl = document.createElement('span');
    valueEl.className = 'recipients-value';
    valueEl.textContent = value;
    row.appendChild(valueEl);

    return row;
}

// Create message content (iframe for HTML, pre for plaintext)
function createMessageContent(msg) {
    if (msg.isPlaintext) {
        const pre = document.createElement('pre');
        pre.className = 'message-plaintext';
        pre.textContent = msg.content || '';
        return pre;
    }

    // HTML content in sandboxed iframe
    const iframe = document.createElement('iframe');
    iframe.className = 'message-content-frame';
    iframe.sandbox = 'allow-same-origin'; // No scripts allowed
    iframe.scrolling = 'no'; // Disable scrollbars
    iframe.srcdoc = createIframeContent(msg.content || '');

    // Resize iframe to fit content after load
    iframe.onload = function() {
        resizeIframe(iframe);
    };

    return iframe;
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

// Create attachment bar element
function createAttachmentBar(msg) {
    const bar = document.createElement('div');
    bar.className = 'attachment-bar';

    const header = document.createElement('div');
    header.className = 'attachment-bar-header';
    header.textContent = 'Attachments (' + msg.attachmentCount + ')';
    bar.appendChild(header);

    const list = document.createElement('div');
    list.className = 'attachment-list';

    msg.attachments.forEach(att => {
        // Only show non-inline attachments
        if (!att.isInline) {
            list.appendChild(createAttachmentItem(att));
        }
    });

    bar.appendChild(list);
    return bar;
}

// Create attachment item element
function createAttachmentItem(att) {
    const item = document.createElement('div');
    item.className = 'attachment-item';
    item.onclick = function() {
        openAttachment(att.id);
    };

    // Icon placeholder (would need icon font or SVG)
    const icon = document.createElement('div');
    icon.className = 'attachment-icon';
    icon.innerHTML = '&#128206;'; // Paperclip emoji as fallback
    item.appendChild(icon);

    // Info
    const info = document.createElement('div');
    info.className = 'attachment-info';

    const name = document.createElement('span');
    name.className = 'attachment-name';
    name.textContent = att.filename || 'Attachment';
    info.appendChild(name);

    const size = document.createElement('span');
    size.className = 'attachment-size';
    size.textContent = att.formattedSize || '';
    info.appendChild(size);

    item.appendChild(info);

    // Actions
    const actions = document.createElement('div');
    actions.className = 'attachment-actions';

    const saveBtn = document.createElement('button');
    saveBtn.className = 'attachment-btn';
    saveBtn.title = 'Save attachment';
    saveBtn.innerHTML = '&#128190;'; // Floppy disk emoji
    saveBtn.onclick = function(e) {
        e.stopPropagation();
        saveAttachment(att.id);
    };
    actions.appendChild(saveBtn);

    item.appendChild(actions);

    return item;
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
