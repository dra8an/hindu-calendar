# Hindu Calendar (Panchang)

A Hindu lunisolar calendar implementation in C that computes tithi, lunar month, and Hindu date for any Gregorian date. Matches [drikpanchang.com](https://www.drikpanchang.com) output using the Drik Siddhanta approach with Swiss Ephemeris and Lahiri ayanamsa.

## Build

```
make
```

## Usage

```
./hindu-calendar [options]
  -y YEAR      Gregorian year (default: current)
  -m MONTH     Gregorian month 1-12 (default: current)
  -d DAY       Specific day (if omitted, shows full month)
  -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)
  -u OFFSET    UTC offset in hours (default: 5.5)
  -h           Show this help
```

### Full month view

```
$ ./hindu-calendar -m 1 -y 2025

Hindu Calendar — 2025-01 (28.6139°N, 77.2090°E, UTC+5.5)

Date         Day   Sunrise    Tithi                        Hindu Date
----------   ---   --------   ---------------------------- ----------------------------
2025-01-01   Wed   07:15:05   Shukla Dwitiya       (S-2)   Pausha Shukla 2, Saka 1946
2025-01-02   Thu   07:15:19   Shukla Tritiya       (S-3)   Pausha Shukla 3, Saka 1946
...
2025-01-29   Wed   07:11:52   Krishna Amavasya      (K-15)   Pausha Krishna 15, Saka 1946
2025-01-30   Thu   07:11:22   Shukla Pratipada     (S-1)   Magha Shukla 1, Saka 1946
```

### Single day

```
$ ./hindu-calendar -d 18 -m 8 -y 2012

Date:       2012-08-18 (Saturday)
Sunrise:    05:53:08 IST
Tithi:      Shukla Pratipada (S-1)
Hindu Date: Adhika Bhadrapada Shukla 1, Saka 1934 (Vikram 2069)
```

## Tests

```
make test
```

Runs 157 tests covering astronomical calculations, tithi, month determination, and validation against drikpanchang.com reference data (2012-2025).

## Documentation

See [Docs/MASTER.md](Docs/MASTER.md) for a full index. Key files:

- [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md) — algorithms, tech stack, module structure
- [Docs/PROJECT-STATUS.md](Docs/PROJECT-STATUS.md) — what works, test coverage, known limitations
- [Docs/NEXT-STEPS.md](Docs/NEXT-STEPS.md) — roadmap (nakshatra, yoga, Purnimanta, etc.)
- [CHANGELOG.md](CHANGELOG.md) — version history
