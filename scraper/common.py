"""Shared utilities for drikpanchang.com scraping.

Provides session management, CAPTCHA detection, and fetch helpers
used by both lunisolar and solar scrapers.
"""

import os
import signal
import time

import requests

# --- Geoname IDs for supported locations ---
GEONAME_IDS = {
    "delhi": "1261481",
    "nyc": "5128581",
}

# --- Cookies for Lahiri ayanamsa ---
COOKIES = {
    "drik-school-name": "amanta",
    "drik-geoname-id": "1261481",       # New Delhi (default)
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

# Domain/path our settings cookies are scoped to.
#
# These MUST match the domain and path the server uses in its own Set-Cookie
# headers.  A cookie jar keys entries on (domain, path, name), so setting our
# cookies with the default empty domain leaves TWO entries per name once the
# server echoes them back: ours at domain "" and the server's at
# "www.drikpanchang.com".  That makes `dict(session.cookies)` raise
# CookieConflictError, and — more dangerously — makes it ambiguous which value
# is sent on later requests.  If the server ever echoed a different ayanamsa or
# geoname, we would silently scrape the wrong calendar.  Scoping our cookies to
# the server's own domain means its Set-Cookie replaces ours in place.
COOKIE_DOMAIN = "www.drikpanchang.com"
COOKIE_PATH = "/"

# Normal pages are 150-250 KB; CAPTCHA pages are ~2 KB
MIN_VALID_SIZE = 50000

# Drikpanchang blocks per-IP after exactly 200 requests (see scraper/README.md;
# confirmed over four consecutive blocks at 200/199/200/200).  Stopping one
# short means a run never wastes requests discovering a block, and never has to
# retry a page that came back as a CAPTCHA.  Clearing the block needs a new
# outbound IP (VPN switch), then re-run to resume.
MAX_REQUESTS_PER_RUN = 199

# Rotate session every N requests.  NOTE: this does NOT prevent a block -- the
# limit is per-IP, and a fresh session with a new _DRIK_SESSION_ID is still
# refused.  Retained only as harmless hygiene; the real control is
# MAX_REQUESTS_PER_RUN above.
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


def get_cookies(location="delhi"):
    """Return cookies dict for a given location."""
    geoname_id = GEONAME_IDS.get(location)
    if not geoname_id:
        raise ValueError(f"Unknown location: {location!r} (known: {list(GEONAME_IDS)})")
    cookies = dict(COOKIES)
    cookies["drik-geoname-id"] = geoname_id
    return cookies


def new_session(location="delhi", arithmetic=None):
    """Create a new requests session with location-specific cookies and headers.

    Args:
        location: location key for the geoname cookie ("delhi" or "nyc")
        arithmetic: panchang arithmetic school, or None for the site default.
            "modern"         -> Bisuddhasiddhanta panjika (drik / modern astronomy)
            "suryasiddhanta" -> Suryasiddhanta panjika (traditional)
            This is the `drik-arithmetic` cookie, toggled on the site by the
            "Bisuddhasiddhanta / Suryasiddhanta Panjika" toolbar button.
            It changes the computed dates, not just labelling.

    Cookies are scoped to COOKIE_DOMAIN/COOKIE_PATH so that the server's own
    Set-Cookie headers replace them rather than creating duplicate jar entries
    (see the COOKIE_DOMAIN comment above).
    """
    s = requests.Session()
    s.headers.update(HEADERS)
    for name, value in get_cookies(location).items():
        s.cookies.set(name, value, domain=COOKIE_DOMAIN, path=COOKIE_PATH)
    if arithmetic:
        s.cookies.set("drik-arithmetic", arithmetic,
                      domain=COOKIE_DOMAIN, path=COOKIE_PATH)
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


def fetch_pages(targets, output_dir, url_fn, delay, label="page", location="delhi",
                arithmetic=None, max_requests=MAX_REQUESTS_PER_RUN):
    """Generic page fetcher with resume, CAPTCHA rotation, and shutdown support.

    Args:
        targets: list of (key, filename) tuples, e.g. [((2025,1), "2025-01.html"), ...]
        output_dir: directory to save files in
        url_fn: callable(key) -> URL string
        delay: seconds between requests
        label: display label for progress messages
        location: location key for cookies (default: "delhi")
        arithmetic: panchang arithmetic school, or None for the site default
            (see new_session).  Must be carried through EVERY session rotation,
            otherwise a rotation mid-run would silently revert to the default
            school and mix two calendars into one output directory.
        max_requests: stop cleanly after this many HTTP requests (default
            MAX_REQUESTS_PER_RUN).  Drikpanchang blocks per-IP at 200, so
            stopping at 199 means we never spend requests discovering a block
            we already know is coming.  Pass 0 to disable the cap.
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

    if max_requests:
        print(f"Request cap this run: {max_requests} "
              f"(per-IP block hits at 200; switch VPN and re-run to continue)\n")

    session = new_session(location, arithmetic)
    downloaded = 0
    failed = 0
    since_rotate = 0
    requests_made = 0
    hit_cap = False

    for key, fname in targets:
        if is_shutdown():
            print(f"\nShutdown: downloaded {downloaded}, failed {failed}")
            break

        output_path = os.path.join(output_dir, fname)
        if os.path.exists(output_path):
            continue

        if max_requests and requests_made >= max_requests:
            hit_cap = True
            break

        # Proactive session rotation to avoid CAPTCHA
        if since_rotate >= SESSION_ROTATE_INTERVAL:
            session = new_session(location, arithmetic)
            since_rotate = 0

        url = url_fn(key)
        remaining -= 1
        eta_min = remaining * delay / 60

        print(f"[{downloaded + failed + existing + 1}/{len(targets)}] "
              f"Fetching {fname} ... "
              f"(remaining: {remaining}, ETA: {eta_min:.0f}m)", end="", flush=True)

        result = fetch_url(url, output_path, session)
        requests_made += 1
        if result == "ok":
            size_kb = os.path.getsize(output_path) / 1024
            print(f"  OK ({size_kb:.0f} KB)")
            downloaded += 1
            since_rotate += 1
        elif result == "captcha":
            # With max_requests at 199 this should never fire.  Kept as a
            # backstop in case the site's threshold changes.
            print(f"\nCAPTCHA hit after {since_rotate} fetches. Rotating session...")
            session = new_session(location, arithmetic)
            since_rotate = 0
            result = fetch_url(url, output_path, session)
            requests_made += 1
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

    total_now = existing + downloaded
    print(f"\nDone: downloaded {downloaded}, failed {failed}, total existing {total_now}")

    if hit_cap:
        left = len(targets) - total_now
        print(f"\nRequest cap reached ({requests_made} requests, no CAPTCHA hit).")
        print(f"{left} {label}s still to fetch.")
        print("Switch VPN to a new IP, then re-run the same command to resume.")
