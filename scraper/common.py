"""Shared utilities for drikpanchang.com scraping.

Provides session management, CAPTCHA detection, and fetch helpers
used by both lunisolar and solar scrapers.
"""

import os
import signal
import time

import requests

# --- Cookies for New Delhi + Lahiri ayanamsa ---
COOKIES = {
    "drik-school-name": "amanta",
    "drik-geoname-id": "1261481",       # New Delhi
    "drik-language": "en",
    "drik-time-format": "12hour",
    "drik-ayanamsha-type": "chitra-paksha",  # Lahiri
}

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36"
    ),
}

# Normal pages are 150-250 KB; CAPTCHA pages are ~2 KB
MIN_VALID_SIZE = 50000

# Rotate session every N requests to avoid CAPTCHA (triggers at ~200-400)
SESSION_ROTATE_INTERVAL = 10

# Graceful shutdown flag
_shutdown = False


def _signal_handler(signum, frame):
    global _shutdown
    _shutdown = True
    print("\nShutdown requested, finishing current download...")


def install_signal_handlers():
    """Install SIGINT/SIGTERM handlers for graceful shutdown."""
    signal.signal(signal.SIGINT, _signal_handler)
    signal.signal(signal.SIGTERM, _signal_handler)


def is_shutdown():
    """Check if shutdown has been requested."""
    return _shutdown


def new_session():
    """Create a new requests session with standard cookies and headers."""
    s = requests.Session()
    s.headers.update(HEADERS)
    s.cookies.update(COOKIES)
    return s


def fetch_url(url, output_path, session):
    """Fetch a URL and save to disk. Returns 'ok', 'captcha', or 'error'."""
    try:
        resp = session.get(url, timeout=60)
        resp.raise_for_status()
        content = resp.text
        if len(content) < MIN_VALID_SIZE:
            print(f"  CAPTCHA detected ({len(content)} bytes), stopping.")
            return "captcha"
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(content)
        return "ok"
    except requests.RequestException as e:
        print(f"  ERROR: {e}")
        return "error"


def fetch_pages(targets, output_dir, url_fn, delay, label="page"):
    """Generic page fetcher with resume, CAPTCHA rotation, and shutdown support.

    Args:
        targets: list of (key, filename) tuples, e.g. [((2025,1), "2025-01.html"), ...]
        output_dir: directory to save files in
        url_fn: callable(key) -> URL string
        delay: seconds between requests
        label: display label for progress messages
    """
    os.makedirs(output_dir, exist_ok=True)

    # Count already downloaded
    existing = sum(
        1 for _, fname in targets
        if os.path.exists(os.path.join(output_dir, fname))
    )
    remaining = len(targets) - existing
    print(f"Total {label}s: {len(targets)}, already downloaded: {existing}, remaining: {remaining}")

    if remaining == 0:
        print(f"All {label}s already downloaded.")
        return

    eta_hours = remaining * delay / 3600
    print(f"Estimated time: {eta_hours:.1f} hours at {delay}s delay\n")

    session = new_session()
    downloaded = 0
    failed = 0
    since_rotate = 0

    for key, fname in targets:
        if is_shutdown():
            print(f"\nShutdown: downloaded {downloaded}, failed {failed}")
            break

        output_path = os.path.join(output_dir, fname)
        if os.path.exists(output_path):
            continue

        # Proactive session rotation to avoid CAPTCHA
        if since_rotate >= SESSION_ROTATE_INTERVAL:
            session = new_session()
            since_rotate = 0

        url = url_fn(key)
        remaining -= 1
        eta_min = remaining * delay / 60

        print(f"[{downloaded + failed + existing + 1}/{len(targets)}] "
              f"Fetching {fname} ... "
              f"(remaining: {remaining}, ETA: {eta_min:.0f}m)", end="", flush=True)

        result = fetch_url(url, output_path, session)
        if result == "ok":
            size_kb = os.path.getsize(output_path) / 1024
            print(f"  OK ({size_kb:.0f} KB)")
            downloaded += 1
            since_rotate += 1
        elif result == "captcha":
            # Should be rare with proactive rotation, but handle it anyway
            print(f"\nCAPTCHA hit after {since_rotate} fetches. Rotating session...")
            session = new_session()
            since_rotate = 0
            result = fetch_url(url, output_path, session)
            if result == "ok":
                size_kb = os.path.getsize(output_path) / 1024
                print(f"  Retry OK ({size_kb:.0f} KB)")
                downloaded += 1
                since_rotate += 1
            else:
                print(f"\nStill blocked after session rotation. Stopping.")
                break
        else:
            failed += 1
            since_rotate += 1

        if remaining > 0 and not is_shutdown():
            time.sleep(delay)

    print(f"\nDone: downloaded {downloaded}, failed {failed}, total existing {existing + downloaded}")
