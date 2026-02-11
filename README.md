# Hindu Calendar (Panchang)

A Hindu lunisolar and solar calendar implementation in C. Computes tithi, lunar month, and Hindu date (lunisolar panchang), plus regional solar calendars for Tamil, Bengali, Odia, and Malayalam traditions. Matches [drikpanchang.com](https://www.drikpanchang.com) output using the Drik Siddhanta approach with Swiss Ephemeris and Lahiri ayanamsa.

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
  -s TYPE      Solar calendar: tamil, bengali, odia, malayalam
               (if omitted, shows lunisolar panchang)
  -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)
  -u OFFSET    UTC offset in hours (default: 5.5)
  -h           Show this help
```

### Lunisolar panchang (full month)

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

### Solar calendar (Tamil)

```
$ ./hindu-calendar -s tamil -y 2025 -m 4

Tamil Solar Calendar — Panguni 1946 (Saka)
Gregorian 2025-04

Date         Day   Solar Date
----------   ---   --------------------
2025-04-01   Tue   Panguni 18, 1946
...
2025-04-13   Sun   Panguni 30, 1946
2025-04-14   Mon   Chithirai 1, 1947     <- Puthandu (Tamil New Year)
2025-04-15   Tue   Chithirai 2, 1947
...
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

Runs 51,735 assertions across 9 test suites: unit tests for astronomical calculations, tithi, month determination, and solar calendars; 186 lunisolar dates validated against drikpanchang.com (1900-2050, including 132 adhika/kshaya edge cases); 327 solar calendar month-start dates validated against drikpanchang.com/prokerala.com across all four regional variants; and regression tests covering 1,104 sampled lunisolar days, all 4,269 adhika/kshaya tithi edge cases, and 7,244 solar month boundaries (1900-2050).

## Documentation

See [Docs/MASTER.md](Docs/MASTER.md) for a full index. Key files:

- [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md) — algorithms, tech stack, module structure
- [Docs/PROJECT-STATUS.md](Docs/PROJECT-STATUS.md) — what works, test coverage, known limitations
- [Docs/NEXT-STEPS.md](Docs/NEXT-STEPS.md) — roadmap (nakshatra, yoga, Purnimanta, etc.)
- [Docs/SOLAR_PLAN.md](Docs/SOLAR_PLAN.md) — solar calendar design document
- [CHANGELOG.md](CHANGELOG.md) — version history
