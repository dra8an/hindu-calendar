#!/usr/bin/env python3
"""Surya Siddhanta solar longitude + sankranti finding, in floating point.

Ported from Reingold/Dershowitz calendar.l (hindu-true-position and friends),
but avoiding the exact-rational arithmetic that makes the Lisp version
~2 sankrantis/minute.

Precision trap and its fix
--------------------------
R/D define  creation = epoch - 1_955_880_000 * sidereal_year  (~ -7.1e11).
Computing (tee - creation) in doubles burns most of the 15-16 significant
digits, leaving ~1e-4 day (~9 second) resolution.

It cancels for the Sun, because its period IS sidereal_year:

    (tee - creation)/sidereal_year = (tee - epoch)/sidereal_year + 1_955_880_000

and the integer drops out under mod 1.  For the anomalistic year the offset is
not an integer, so we reduce that constant ONCE with exact Fractions and keep
only its fractional part.  Everything after that is small-magnitude floats.
"""

import math
from fractions import Fraction

# --- R/D constants (exact) ---------------------------------------------------
HINDU_EPOCH = -1132959                                   # RD of Kali Yuga start
SIDEREAL_YEAR = Fraction(365) + Fraction(279457, 1080000)
ANOMALISTIC_YEAR = Fraction(1577917828000, 4320000000 - 387)
CREATION_REVS = 1955880000                                # creation = epoch - this * sidereal_year

SOLAR_EPICYCLE = Fraction(14, 360)   # 'size'   in hindu-true-position
SOLAR_CHANGE = Fraction(1, 42)       # 'change' in hindu-true-position

# Fractional part of the constant offset for the anomalistic argument:
#   (tee - creation)/anom = (tee - epoch)/anom + CREATION_REVS*sidereal/anom
_ANOM_OFFSET = float((CREATION_REVS * SIDEREAL_YEAR / ANOMALISTIC_YEAR) % 1)

_SID = float(SIDEREAL_YEAR)
_ANOM = float(ANOMALISTIC_YEAR)
_SIZE = float(SOLAR_EPICYCLE)
_CHANGE = float(SOLAR_CHANGE)

# --- Hindu sine table --------------------------------------------------------
# Table is indexed by multiples of 225 arcminutes (3.75 degrees), 0..24.
_ARCMIN225 = 225.0 / 60.0   # = 3.75 degrees


def _sign(x):
    return (x > 0) - (x < 0)


_TABLE_CACHE = {}


def sine_table(n):
    """hindu-sine-table: simulates the classical table, in units of R=3438.

    R/D define this for ANY integer entry, not just the 0..24 first quadrant:
    the anomaly argument sweeps a full circle (entries 0..96), and sin() gives
    the correct negative values past entry 48.  Caching keeps it cheap.
    """
    v = _TABLE_CACHE.get(n)
    if v is None:
        exact = 3438.0 * math.sin(math.radians(n * _ARCMIN225))
        error = 0.215 * _sign(exact) * _sign(abs(exact) - 1716)
        v = round(exact + error) / 3438.0
        _TABLE_CACHE[n] = v
    return v


def hindu_sine(theta):
    """Linear interpolation in the Hindu sine table. theta in degrees."""
    entry = theta / _ARCMIN225
    frac = entry % 1
    return frac * sine_table(int(math.ceil(entry))) + \
        (1.0 - frac) * sine_table(int(math.floor(entry)))


def hindu_arcsin(amp):
    """Inverse of hindu_sine.  Search stays in the 0..24 quadrant."""
    if amp < 0:
        return -hindu_arcsin(-amp)
    pos = 0
    while pos < 24 and amp > sine_table(pos):
        pos += 1
    below = sine_table(pos - 1) if pos > 0 else 0.0
    span = sine_table(pos) - below
    if span == 0:
        return _ARCMIN225 * pos
    return _ARCMIN225 * (pos - 1 + (amp - below) / span)


# --- Positions ---------------------------------------------------------------
def mean_solar_position(tee):
    """360 * frac((tee - creation)/sidereal_year), computed without precision loss."""
    return 360.0 * (((tee - HINDU_EPOCH) / _SID) % 1.0)


def mean_anomalistic_position(tee):
    """360 * frac((tee - creation)/anomalistic_year), offset reduced exactly."""
    return 360.0 * ((((tee - HINDU_EPOCH) / _ANOM) + _ANOM_OFFSET) % 1.0)


def solar_longitude(tee):
    """Surya Siddhanta true solar longitude (sidereal/nirayana), degrees."""
    lam = mean_solar_position(tee)
    offset = hindu_sine(mean_anomalistic_position(tee))
    contraction = abs(offset) * _CHANGE * _SIZE
    equation = hindu_arcsin(offset * (_SIZE - contraction))
    return (lam - equation) % 360.0


def zodiac(tee):
    """Rashi 1..12 at moment tee."""
    return 1 + int(solar_longitude(tee) // 30)


# --- Lunar (needed for the tithi-based critical-time rule) -------------------
SIDEREAL_MONTH = Fraction(27) + Fraction(4644439, 14438334)
ANOMALISTIC_MONTH = Fraction(1577917828, 57753336 - 488199)
LUNAR_EPICYCLE = Fraction(32, 360)
LUNAR_CHANGE = Fraction(1, 96)

_SID_M = float(SIDEREAL_MONTH)
_ANOM_M = float(ANOMALISTIC_MONTH)
_SIZE_M = float(LUNAR_EPICYCLE)
_CHANGE_M = float(LUNAR_CHANGE)

# Same precision trick as the Sun: reduce the huge creation offset exactly once.
_LUNAR_OFFSET = float((CREATION_REVS * SIDEREAL_YEAR / SIDEREAL_MONTH) % 1)
_LUNAR_ANOM_OFFSET = float((CREATION_REVS * SIDEREAL_YEAR / ANOMALISTIC_MONTH) % 1)


def lunar_longitude(tee):
    """Surya Siddhanta true lunar longitude (sidereal), degrees."""
    lam = 360.0 * ((((tee - HINDU_EPOCH) / _SID_M) + _LUNAR_OFFSET) % 1.0)
    anom = 360.0 * ((((tee - HINDU_EPOCH) / _ANOM_M) + _LUNAR_ANOM_OFFSET) % 1.0)
    offset = hindu_sine(anom)
    contraction = abs(offset) * _CHANGE_M * _SIZE_M
    equation = hindu_arcsin(offset * (_SIZE_M - contraction))
    return (lam - equation) % 360.0


def lunar_phase(tee):
    """Moon - Sun elongation, degrees."""
    return (lunar_longitude(tee) - solar_longitude(tee)) % 360.0


def tithi_at(tee):
    """Tithi 1..30 at moment tee."""
    return 1 + int(lunar_phase(tee) // 12)


def tithi_end_after(tee):
    """Moment the tithi current at `tee` ends (next 12-degree boundary).

    Mirrors the boundary search in src/tithi.c: bisect on the elongation
    crossing the next multiple of 12 degrees.
    """
    phase = lunar_phase(tee)
    target = 12.0 * (math.floor(phase / 12.0) + 1)
    if target >= 360.0:
        target = 0.0

    def diff(t):
        d = (lunar_phase(t) - target) % 360.0
        return d - 360.0 if d > 180.0 else d

    # The moon gains ~12 degrees on the sun in ~1 day; bracket generously.
    lo = tee
    hi = tee + 2.0
    while diff(hi) < 0 and hi - tee < 5.0:
        hi += 0.5
    for _ in range(60):
        mid = (lo + hi) / 2.0
        if diff(mid) < 0:
            lo = mid
        else:
            hi = mid
        if hi - lo < 1e-9:
            break
    return (lo + hi) / 2.0


# --- Sankranti finding -------------------------------------------------------
def _ang_diff(a, b):
    d = (a - b) % 360.0
    return d - 360.0 if d > 180.0 else d


def sankranti_at_or_after(target_deg, tee):
    """Moment at or after tee when solar_longitude reaches target_deg.

    Bisection, mirroring the approach used in src/solar.c.
    """
    # Estimate: how far to travel at mean rate.
    gap = (target_deg - solar_longitude(tee)) % 360.0
    tau = tee + _SID * gap / 360.0
    lo, hi = max(tee, tau - 5.0), tau + 5.0

    # Ensure the bracket straddles the crossing.
    if _ang_diff(solar_longitude(lo), target_deg) > 0:
        lo = max(tee, lo - 5.0)

    for _ in range(60):
        mid = (lo + hi) / 2.0
        if _ang_diff(solar_longitude(mid), target_deg) < 0:
            lo = mid
        else:
            hi = mid
        if hi - lo < 1e-9:      # ~0.1 ms
            break
    return (lo + hi) / 2.0


def all_sankrantis(start_rd, end_rd):
    """Yield (rashi, moment) for every 30-degree crossing in [start_rd, end_rd]."""
    tee = start_rd
    while True:
        lon = solar_longitude(tee)
        target = (30.0 * (math.floor(lon / 30.0) + 1)) % 360.0
        moment = sankranti_at_or_after(target, tee)
        if moment > end_rd:
            return
        yield (1 + int(round(target / 30.0)) % 12, moment)
        tee = moment + 1.0
