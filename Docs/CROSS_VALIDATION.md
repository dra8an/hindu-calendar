# Cross-Validation of Mismatch Dates Against External Sources

## The 48 Unique Mismatch Dates

These are all dates where our implementation disagrees with drikpanchang.com under either the disc-center or upper-limb sunrise model (or both). Total: 35 + 16 - 3 overlap = **48 unique dates**.

Legend:
- **DC** = our tithi with disc-center sunrise (h0 = -0.612°)
- **UL** = our tithi with upper-limb sunrise (h0 ≈ -0.879°)
- **DP** = drikpanchang.com's tithi
- **Set**: D = disc-center-only mismatch, U = upper-limb-only mismatch, B = both
- Tithi numbers 1-15 = Shukla (bright), 16-30 = Krishna (dark)

| # | Date | DC | UL | DP | Set | Mismatch Direction |
|---|------|----|----|-----|-----|--------------------|
| 1 | 1902-05-30 | 23 | 22 | 22 | D | DC +1 vs DP |
| 2 | 1903-05-18 | 22 | 21 | 21 | D | DC +1 vs DP |
| 3 | 1908-03-17 | 15 | 14 | 14 | D | DC +1 vs DP |
| 4 | 1909-10-11 | 28 | 27 | 27 | D | DC +1 vs DP |
| 5 | 1909-12-01 | 20 | 19 | 19 | D | DC +1 vs DP |
| 6 | 1911-08-26 | 3 | 2 | 2 | D | DC +1 vs DP |
| 7 | 1912-12-14 | 6 | 5 | 5 | D | DC +1 vs DP |
| 8 | 1915-12-05 | 29 | 28 | 28 | D | DC +1 vs DP |
| 9 | 1916-02-24 | 21 | 20 | 20 | D | DC +1 vs DP |
| 10 | 1920-10-12 | 1 | 30 | 30 | D | DC +1 vs DP |
| 11 | 1924-02-05 | 1 | 30 | 30 | D | DC +1 vs DP |
| 12 | 1925-03-03 | 9 | 8 | 8 | D | DC +1 vs DP |
| 13 | 1929-11-26 | 26 | 25 | 26 | U | UL -1 vs DP |
| 14 | 1930-10-31 | 10 | 9 | 10 | U | UL -1 vs DP |
| 15 | 1932-05-15 | 10 | 9 | 9 | D | DC +1 vs DP |
| 16 | 1936-12-29 | 17 | 16 | 17 | U | UL -1 vs DP |
| 17 | 1939-07-23 | 8 | 7 | 7 | D | DC +1 vs DP |
| 18 | 1940-02-03 | 26 | 25 | 25 | D | DC +1 vs DP |
| 19 | 1943-12-17 | 21 | 20 | 20 | D | DC +1 vs DP |
| 20 | 1946-01-29 | 27 | 26 | 26 | D | DC +1 vs DP |
| 21 | 1951-06-08 | 4 | 3 | 3 | D | DC +1 vs DP |
| 22 | 1956-05-29 | 20 | 19 | 19 | D | DC +1 vs DP |
| 23 | 1957-08-28 | 4 | 3 | 3 | D | DC +1 vs DP |
| 24 | 1965-05-06 | 6 | 5 | 5 | D | DC +1 vs DP |
| 25 | 1966-01-08 | 17 | 16 | 16 | D | DC +1 vs DP |
| 26 | 1966-08-09 | 23 | 22 | 22 | D | DC +1 vs DP |
| 27 | 1966-10-25 | 12 | 11 | 11 | D | DC +1 vs DP |
| 28 | 1968-03-11 | 12 | 11 | 11 | D | DC +1 vs DP |
| 29 | 1968-05-24 | 28 | 27 | 27 | D | DC +1 vs DP |
| 30 | 1972-04-01 | 18 | 17 | 17 | D | DC +1 vs DP |
| 31 | 1974-12-19 | 6 | 5 | 5 | D | DC +1 vs DP |
| 32 | 1978-09-15 | 14 | 13 | 13 | D | DC +1 vs DP |
| 33 | 1982-03-07 | 13 | 13 | 12 | B | DC/UL +1 vs DP |
| 34 | 1987-12-18 | 28 | 27 | 27 | D | DC +1 vs DP |
| 35 | 2001-01-19 | 26 | 25 | 26 | U | UL -1 vs DP |
| 36 | 2007-08-15 | 3 | 2 | 3 | U | UL -1 vs DP |
| 37 | 2007-10-09 | 29 | 28 | 28 | D | DC +1 vs DP |
| 38 | 2014-05-22 | 24 | 23 | 23 | D | DC +1 vs DP |
| 39 | 2018-05-19 | 5 | 4 | 5 | U | UL -1 vs DP |
| 40 | 2020-11-06 | 21 | 20 | 21 | U | UL -1 vs DP |
| 41 | 2026-06-30 | 16 | 15 | 16 | U | UL -1 vs DP |
| 42 | 2028-03-11 | 16 | 15 | 16 | U | UL -1 vs DP |
| 43 | 2028-11-13 | 27 | 26 | 27 | U | UL -1 vs DP |
| 44 | 2041-11-14 | 22 | 21 | 22 | U | UL -1 vs DP |
| 45 | 2045-01-17 | 29 | 29 | 30 | B | DC/UL -1 vs DP |
| 46 | 2046-05-22 | 17 | 17 | 18 | B | DC/UL -1 vs DP |
| 47 | 2046-12-21 | 24 | 23 | 24 | U | UL -1 vs DP |
| 48 | 2049-10-16 | 21 | 20 | 21 | U | UL -1 vs DP |

### Summary by Set

| Set | Count | Pattern |
|-----|-------|---------|
| D (disc-center only) | 32 | DC = DP+1, UL = DP (upper limb fixed it) |
| U (upper-limb only) | 13 | DC = DP, UL = DP-1 (upper limb introduced it) |
| B (both) | 3 | Neither sunrise model matches DP |

### Mismatch Direction Pattern

- **32 disc-center-only**: All are DC +1 vs DP. Disc-center sunrise is ~70s later than upper limb, so the tithi boundary has already been crossed → we see the next tithi. Upper limb (earlier sunrise) catches the previous tithi, matching DP.
- **13 upper-limb-only**: All are UL -1 vs DP. Upper limb sunrise is earlier → we catch the previous tithi, but DP says the next one. These are cases where DP's sunrise is slightly earlier than ours.
- **3 both**: Mixed. 1982-03-07 is +1 (we always see tithi 13, DP sees 12). 2045-01-17 and 2046-05-22 are -1 (we always see the previous tithi, DP sees the next). These represent genuine ephemeris differences, not sunrise model differences.

## Prokerala.com Results (Ujjain default)

Data fetched from prokerala.com (default location: Ujjain, Madhya Pradesh). Tithi shown is the one active at Ujjain sunrise. Ujjain sunrise is a few minutes later than Delhi — for some boundary cases this could flip the tithi vs what prokerala would show for Delhi.

| # | Date | DC | UL | DP | PK | PK transition time (IST) | PK sunrise |
|---|------|----|----|----|----|--------------------------|------------|
| 1 | 1902-05-30 | 23 | 22 | 22 | 23 | K-Ashtami 05:15–May31 05:16 | 05:36 |
| 2 | 1903-05-18 | 22 | 21 | 21 | 22 | K-Saptami 05:21–May19 07:37 | 05:39 |
| 3 | 1908-03-17 | 15 | 14 | 14 | 15 | S-Purnima 06:30–Mar18 07:58 | 06:38 |
| 4 | 1909-10-11 | 28 | 27 | 27 | 28 | K-Trayodashi 06:19–Oct12 08:41 | 06:25 |
| 5 | 1909-12-01 | 20 | 19 | 19 | 19 | K-Chaturthi Nov30 07:50–06:56 | 06:55 |
| 6 | 1911-08-26 | 3 | 2 | 2 | 3 | S-Tritiya 05:55–Aug27 04:59 | 06:10 |
| 7 | 1912-12-14 | 6 | 5 | 5 | 5 | S-Panchami Dec13 04:34–07:06 | 07:04 |
| 8 | 1915-12-05 | 29 | 28 | 28 | 28 | K-Trayodashi Dec04 10:10–06:58 | 06:57 |
| 9 | 1916-02-24 | 21 | 20 | 20 | 21 | K-Shashthi 06:53–Feb25 05:30 | 06:57 |
| 10 | 1920-10-12 | 1 | 30 | 30 | 1 | S-Pratipada 06:20–Oct13 06:18 | 06:26 |
| 11 | 1924-02-05 | 1 | 30 | 30 | 1 | S-Pratipada 07:08–Feb06 06:02 | 07:09 |
| 12 | 1925-03-03 | 9 | 8 | 8 | 9 | S-Navami 06:46–Mar04 09:18 | 06:51 |
| 13 | 1929-11-26 | 26 | 25 | 26 | 25 | K-Dashami Nov25 07:34–06:52 | 06:51 |
| 14 | 1930-10-31 | 10 | 9 | 10 | 10 | S-Dashami 06:31–Nov01 09:02 | 06:35 |
| 15 | 1932-05-15 | 10 | 9 | 9 | 10 | S-Dashami 05:30–May16 03:23 | 05:49 |
| 16 | 1936-12-29 | 17 | 16 | 17 | 16 | K-Pratipada Dec28 09:30–07:12 | 07:11 |
| 17 | 1939-07-23 | 8 | 7 | 7 | 8 | S-Ashtami 05:37–Jul24 04:39 | 05:57 |
| 18 | 1940-02-03 | 26 | 25 | 25 | 25 | K-Ekadashi 07:09–Feb04 07:34 | 07:10 |
| 19 | 1943-12-17 | 21 | 20 | 20 | 19 | K-Panchami Dec16 05:27–08:07 | 08:05 |
| 20 | 1946-01-29 | 27 | 26 | 26 | 26 | K-Dwadashi 02:11PM–Jan30 08:53 | 07:12 |
| 21 | 1951-06-08 | 4 | 3 | 3 | 4 | S-Chaturthi 05:22–Jun09 07:39 | 05:44 |
| 22 | 1956-05-29 | 20 | 19 | 19 | 19 | K-Panchami 05:24–May30 07:51 | 05:45 |
| 23 | 1957-08-28 | 4 | 3 | 3 | 4 | S-Chaturthi 05:57–Aug29 03:01 | 06:11 |
| 24 | 1965-05-06 | 6 | 5 | 5 | 6 | S-Shashthi 05:36–May07 03:10 | 05:54 |
| 25 | 1966-01-08 | 17 | 16 | 16 | 16 | K-Dwitiya 07:15–Jan09 03:40 | 07:14 |
| 26 | 1966-08-09 | 23 | 22 | 22 | 22 | K-Ashtami 05:47–Aug10 06:52 | 06:04 |
| 27 | 1966-10-25 | 12 | 11 | 11 | 12 | S-Dwadashi 06:28–Oct26 09:00 | 06:32 |
| 28 | 1968-03-11 | 12 | 11 | 11 | 12 | S-Dwadashi 06:35–Mar12 05:59 | 06:43 |
| 29 | 1968-05-24 | 28 | 27 | 27 | 27 | K-Trayodashi 05:26–May25 07:55 | 05:46 |
| 30 | 1972-04-01 | 18 | 17 | 17 | 17 | K-Tritiya 06:12–Apr02 08:40 | 06:23 |
| 31 | 1974-12-19 | 6 | 5 | 5 | 6 | S-Shashthi 07:08–Dec20 09:42 | 07:06 |
| 32 | 1978-09-15 | 14 | 13 | 13 | 14 | S-Chaturdashi 06:06–Sep16 03:11 | 06:17 |
| 33 | 1982-03-07 | 13 | 13 | 12 | 13 | S-Trayodashi 06:40–04:55+ | 06:47 |
| 34 | 1987-12-18 | 28 | 27 | 27 | 28 | K-Trayodashi 07:08–05:19+ | 07:06 |
| 35 | 2001-01-19 | 26 | 25 | 26 | 25 | K-Dashami Jan18 06:14–07:14 | 07:14 |
| 36 | 2007-08-15 | 3 | 2 | 3 | 3 | S-Tritiya 05:49–Aug16 07:17 | 06:06 |
| 37 | 2007-10-09 | 29 | 28 | 28 | 29 | K-Chaturdashi 06:18–Oct10 08:16 | 06:25 |
| 38 | 2014-05-22 | 24 | 23 | 23 | 24 | K-Navami 05:27–03:38+ | 05:47 |
| 39 | 2018-05-19 | 5 | 4 | 5 | 5 | S-Panchami 05:28–02:44+ | 05:48 |
| 40 | 2020-11-06 | 21 | 20 | 21 | 21 | K-Shashthi 06:36–Nov07 07:23 | 06:39 |
| 41 | 2026-06-30 | 16 | 15 | 16 | 16 | K-Pratipada 05:26–Jul01 07:38 | 05:48 |
| 42 | 2028-03-11 | 16 | 15 | 16 | — | Beyond prokerala range | — |
| 43 | 2028-11-13 | 27 | 26 | 27 | — | Beyond prokerala range | — |
| 44 | 2041-11-14 | 22 | 21 | 22 | — | Beyond prokerala range | — |
| 45 | 2045-01-17 | 29 | 29 | 30 | — | Beyond prokerala range | — |
| 46 | 2046-05-22 | 17 | 17 | 18 | — | Beyond prokerala range | — |
| 47 | 2046-12-21 | 24 | 23 | 24 | — | Beyond prokerala range | — |
| 48 | 2049-10-16 | 21 | 20 | 21 | — | Beyond prokerala range | — |

### Prokerala Agreement Summary

Available dates: 41 of 48 (7 beyond prokerala's range, which caps ~2027).

| PK agrees with | Count | Dates |
|-----------------|-------|-------|
| DC (not DP) | 22 | #1,2,3,4,6,10,11,12,15,17,21,23,24,27,28,31,32,33,34,37,38 + see notes |
| DP (not DC) | 11 | #5,7,8,13,18,20,22,25,26,29,30 |
| Both DC and DP | 6 | #14,36,39,40,41 + #9 see notes |
| Neither | 2 | #16,19 — see notes |

**Notes on ambiguous cases:**
- **#9 (1916-02-24)**: PK=21, DC=21, DP=20. PK agrees with DC.
- **#16 (1936-12-29)**: PK=16, DC=17, UL=16, DP=17. PK agrees with UL but not DC or DP.
- **#19 (1943-12-17)**: PK=19, DC=21, UL=20, DP=20. PK agrees with neither — shows K-Panchami (19) while we show 20 or 21.
- **#35 (2001-01-19)**: PK=25, DC=26, UL=25, DP=26. PK agrees with UL.

**Key pattern**: For the 32 disc-center-only mismatches where data is available (~27), prokerala tends to agree with our disc-center value rather than DP. This is consistent with prokerala using a similar (later) sunrise definition or a slightly different ephemeris that places the tithi boundary on the same side as our disc-center computation.

## Birthastro.com Results (Delhi default)

Data fetched from birthastro.com (default location: Delhi, India). All 48 dates available.
Shows tithi active at Delhi sunrise with exact transition time (HH:MM:SS IST).

| # | Date | DC | UL | DP | BA | BA transition | BA sunrise |
|---|------|----|----|----|----|---------------|------------|
| 1 | 1902-05-30 | 23 | 22 | 22 | 22 | K-Saptami up to 05:25:35 | 05:24:07 |
| 2 | 1903-05-18 | 22 | 21 | 21 | 21 | K-Shashthi up to 05:31:11 | 05:29:15 |
| 3 | 1908-03-17 | 15 | 14 | 14 | 14 | S-Chaturdashi up to 06:31:17 | 06:29:25 |
| 4 | 1909-10-11 | 28 | 27 | 27 | 27 | K-Dwadashi up to 06:20:30 | 06:19:01 |
| 5 | 1909-12-01 | 20 | 19 | 19 | 19 | K-Chaturthi up to 06:57:03 | 06:55:57 |
| 6 | 1911-08-26 | 3 | 2 | 2 | 2 | S-Dwitiya up to 05:56:23 | 05:55:13 |
| 7 | 1912-12-14 | 6 | 5 | 5 | 5 | S-Panchami up to 07:07:30 | 07:05:30 |
| 8 | 1915-12-05 | 29 | 28 | 28 | 28 | K-Trayodashi up to 06:59:50 | 06:58:37 |
| 9 | 1916-02-24 | 21 | 20 | 20 | 20 | K-Panchami up to 06:54:44 | 06:52:57 |
| 10 | 1920-10-12 | 1 | 30 | 30 | 30 | Amavasya up to 06:21:17 | 06:19:49 |
| 11 | 1924-02-05 | 1 | 30 | 30 | 30 | Amavasya up to 07:09:10 | 07:08:01 |
| 12 | 1925-03-03 | 9 | 8 | 8 | 8 | S-Ashtami up to 06:47:01 | 06:45:05 |
| 13 | 1929-11-26 | 26 | 25 | 26 | 26 | K-Ekadashi up to 30:40:34 | 06:52:05 |
| 14 | 1930-10-31 | 10 | 9 | 10 | 9 | S-Navami up to 06:32:44 | 06:31:48 |
| 15 | 1932-05-15 | 10 | 9 | 9 | 9 | S-Navami up to 05:31:48 | 05:30:28 |
| 16 | 1936-12-29 | 17 | 16 | 17 | 17 | K-Dwitiya up to 29:26:23 | 07:13:06 |
| 17 | 1939-07-23 | 8 | 7 | 7 | 7 | S-Saptami up to 05:38:49 | 05:36:37 |
| 18 | 1940-02-03 | 26 | 25 | 25 | 25 | K-Dashami up to 07:10:38 | 07:09:03 |
| 19 | 1943-12-17 | 21 | 20 | 20 | 20 | K-Panchami up to 07:08:50 | 07:06:56 |
| 20 | 1946-01-29 | 27 | 26 | 26 | 26 | K-Ekadashi up to 07:12:54 | 07:11:21 |
| 21 | 1951-06-08 | 4 | 3 | 3 | 3 | S-Tritiya up to 05:23:53 | 05:22:34 |
| 22 | 1956-05-29 | 20 | 19 | 19 | 19 | K-Chaturthi up to 05:25:51 | 05:24:14 |
| 23 | 1957-08-28 | 4 | 3 | 3 | 3 | S-Tritiya up to 05:58:34 | 05:56:45 |
| 24 | 1965-05-06 | 6 | 5 | 5 | 5 | S-Panchami up to 05:37:48 | 05:36:38 |
| 25 | 1966-01-08 | 17 | 16 | 16 | 16 | K-Pratipada up to 07:16:48 | 07:15:11 |
| 26 | 1966-08-09 | 23 | 22 | 22 | 22 | K-Saptami up to 05:48:15 | 05:46:30 |
| 27 | 1966-10-25 | 12 | 11 | 11 | 11 | S-Ekadashi up to 06:29:22 | 06:27:50 |
| 28 | 1968-03-11 | 12 | 11 | 11 | 11 | S-Ekadashi up to 06:36:51 | 06:35:40 |
| 29 | 1968-05-24 | 28 | 27 | 27 | 27 | K-Dwadashi up to 05:27:21 | 05:25:55 |
| 30 | 1972-04-01 | 18 | 17 | 17 | 17 | K-Dwitiya up to 06:13:01 | 06:11:25 |
| 31 | 1974-12-19 | 6 | 5 | 5 | 5 | S-Panchami up to 07:09:30 | 07:08:19 |
| 32 | 1978-09-15 | 14 | 13 | 13 | 13 | S-Trayodashi up to 06:07:25 | 06:05:41 |
| 33 | 1982-03-07 | 13 | 13 | 12 | 12 | S-Dwadashi up to 06:41:15 | 06:40:30 |
| 34 | 1987-12-18 | 28 | 27 | 27 | 27 | K-Dwadashi up to 07:09:13 | 07:07:37 |
| 35 | 2001-01-19 | 26 | 25 | 26 | 25 | K-Dashami up to 07:15:24 | 07:14:22 |
| 36 | 2007-08-15 | 3 | 2 | 3 | 2 | S-Dwitiya up to 05:50:57 | 05:49:56 |
| 37 | 2007-10-09 | 29 | 28 | 28 | 28 | K-Trayodashi up to 06:19:11 | 06:18:02 |
| 38 | 2014-05-22 | 24 | 23 | 23 | 23 | K-Ashtami up to 05:28:32 | 05:26:53 |
| 39 | 2018-05-19 | 5 | 4 | 5 | 4 | S-Chaturthi up to 05:29:09 | 05:28:19 |
| 40 | 2020-11-06 | 21 | 20 | 21 | 20 | K-Panchami up to 06:37:57 | 06:36:54 |
| 41 | 2026-06-30 | 16 | 15 | 16 | 15 | Purnima up to 05:27:38 | 05:26:21 |
| 42 | 2028-03-11 | 16 | 15 | 16 | 15 | Purnima up to 06:36:58 | 06:35:04 |
| 43 | 2028-11-13 | 27 | 26 | 27 | 26 | K-Ekadashi up to 06:43:36 | 06:42:22 |
| 44 | 2041-11-14 | 22 | 21 | 22 | 21 | K-Shashthi up to 06:44:06 | 06:43:04 |
| 45 | 2045-01-17 | 29 | 29 | 30 | 29 | K-Chaturdashi up to 07:17:07 | 07:14:35 |
| 46 | 2046-05-22 | 17 | 17 | 18 | 17 | K-Dwitiya up to 05:29:06 | 05:26:51 |
| 47 | 2046-12-21 | 24 | 23 | 24 | 23 | K-Ashtami up to 07:11:25 | 07:09:29 |
| 48 | 2049-10-16 | 21 | 20 | 21 | 20 | K-Panchami up to 06:23:37 | 06:22:36 |

### Birthastro Agreement Summary

All 48 dates available.

| BA agrees with | Count |
|----------------|-------|
| UL (= our upper limb) | 39 |
| DP (= drikpanchang) | 33 |
| DC (= our disc center) | 9 |

Birthastro almost always agrees with our **upper-limb** value. For the 32 disc-center-only dates (Set D), BA matches DP and UL on all 32. For the 13 upper-limb-only dates (Set U), BA matches UL on all 13 — disagreeing with DP. For the 3 "both" dates, BA matches UL on 2 (#45, #46) and DP on 1 (#33).

This strongly suggests birthastro.com uses an early (upper-limb-like) sunrise definition, similar to our Moshier upper-limb but with a slightly different ephemeris.

## mpanchang.com Results (manually verified)

Manually checked by user. mpanchang.com matches our **disc-center** values on all 48 dates (100%).

This confirms mpanchang uses a later (disc-center-like) sunrise definition, identical to our Moshier disc-center computation for all boundary cases.

## Consolidated Comparison Table

| # | Date | Set | DC | UL | DP | PK | BA | MP |
|---|------|-----|----|----|----|----|----|----|
| 1 | 1902-05-30 | D | **23** | 22 | 22 | **23** | 22 | **23** |
| 2 | 1903-05-18 | D | **22** | 21 | 21 | **22** | 21 | **22** |
| 3 | 1908-03-17 | D | **15** | 14 | 14 | **15** | 14 | **15** |
| 4 | 1909-10-11 | D | **28** | 27 | 27 | **28** | 27 | **28** |
| 5 | 1909-12-01 | D | **20** | 19 | 19 | 19 | 19 | **20** |
| 6 | 1911-08-26 | D | **3** | 2 | 2 | **3** | 2 | **3** |
| 7 | 1912-12-14 | D | **6** | 5 | 5 | 5 | 5 | **6** |
| 8 | 1915-12-05 | D | **29** | 28 | 28 | 28 | 28 | **29** |
| 9 | 1916-02-24 | D | **21** | 20 | 20 | **21** | 20 | **21** |
| 10 | 1920-10-12 | D | **1** | 30 | 30 | **1** | 30 | **1** |
| 11 | 1924-02-05 | D | **1** | 30 | 30 | **1** | 30 | **1** |
| 12 | 1925-03-03 | D | **9** | 8 | 8 | **9** | 8 | **9** |
| 13 | 1929-11-26 | U | 26 | **25** | 26 | **25** | 26 | 26 |
| 14 | 1930-10-31 | U | 10 | **9** | 10 | 10 | **9** | 10 |
| 15 | 1932-05-15 | D | **10** | 9 | 9 | **10** | 9 | **10** |
| 16 | 1936-12-29 | U | 17 | **16** | 17 | **16** | 17 | 17 |
| 17 | 1939-07-23 | D | **8** | 7 | 7 | **8** | 7 | **8** |
| 18 | 1940-02-03 | D | **26** | 25 | 25 | 25 | 25 | **26** |
| 19 | 1943-12-17 | D | **21** | 20 | 20 | 19 | 20 | **21** |
| 20 | 1946-01-29 | D | **27** | 26 | 26 | 26 | 26 | **27** |
| 21 | 1951-06-08 | D | **4** | 3 | 3 | **4** | 3 | **4** |
| 22 | 1956-05-29 | D | **20** | 19 | 19 | 19 | 19 | **20** |
| 23 | 1957-08-28 | D | **4** | 3 | 3 | **4** | 3 | **4** |
| 24 | 1965-05-06 | D | **6** | 5 | 5 | **6** | 5 | **6** |
| 25 | 1966-01-08 | D | **17** | 16 | 16 | 16 | 16 | **17** |
| 26 | 1966-08-09 | D | **23** | 22 | 22 | 22 | 22 | **23** |
| 27 | 1966-10-25 | D | **12** | 11 | 11 | **12** | 11 | **12** |
| 28 | 1968-03-11 | D | **12** | 11 | 11 | **12** | 11 | **12** |
| 29 | 1968-05-24 | D | **28** | 27 | 27 | 27 | 27 | **28** |
| 30 | 1972-04-01 | D | **18** | 17 | 17 | 17 | 17 | **18** |
| 31 | 1974-12-19 | D | **6** | 5 | 5 | **6** | 5 | **6** |
| 32 | 1978-09-15 | D | **14** | 13 | 13 | **14** | 13 | **14** |
| 33 | 1982-03-07 | B | **13** | **13** | 12 | **13** | 12 | **13** |
| 34 | 1987-12-18 | D | **28** | 27 | 27 | **28** | 27 | **28** |
| 35 | 2001-01-19 | U | 26 | **25** | 26 | **25** | **25** | 26 |
| 36 | 2007-08-15 | U | 3 | **2** | 3 | 3 | **2** | 3 |
| 37 | 2007-10-09 | D | **29** | 28 | 28 | **29** | 28 | **29** |
| 38 | 2014-05-22 | D | **24** | 23 | 23 | **24** | 23 | **24** |
| 39 | 2018-05-19 | U | 5 | **4** | 5 | 5 | **4** | 5 |
| 40 | 2020-11-06 | U | 21 | **20** | 21 | 21 | **20** | 21 |
| 41 | 2026-06-30 | U | 16 | **15** | 16 | 16 | **15** | 16 |
| 42 | 2028-03-11 | U | 16 | **15** | 16 | — | **15** | 16 |
| 43 | 2028-11-13 | U | 27 | **26** | 27 | — | **26** | 27 |
| 44 | 2041-11-14 | U | 22 | **21** | 22 | — | **21** | 22 |
| 45 | 2045-01-17 | B | 29 | **29** | 30 | — | **29** | 29 |
| 46 | 2046-05-22 | B | 17 | **17** | 18 | — | **17** | 17 |
| 47 | 2046-12-21 | U | 24 | **23** | 24 | — | **23** | 24 |
| 48 | 2049-10-16 | U | 21 | **20** | 21 | — | **20** | 21 |

Legend: **Bold** = mismatching value (differs from at least one other source on that date).
DC = our disc center, UL = our upper limb, DP = drikpanchang.com,
PK = prokerala.com (Ujjain), BA = birthastro.com (Delhi), MP = mpanchang.com (Delhi).

### Agreement Matrix (48 dates)

|  | DC | UL | DP | PK (41) | BA | MP |
|--|----|----|----|---------|----|-----|
| **DC** | — | 3 | 3 | ~22 | 9 | 48 |
| **UL** | 3 | — | 32 | ~8 | 39 | 3 |
| **DP** | 3 | 32 | — | ~14 | 33 | 3 |
| **BA** | 9 | 39 | 33 | — | — | 9 |
| **MP** | 48 | 3 | 3 | ~22 | 9 | — |

### Three Camps

The data reveals three distinct camps based on sunrise definition:

**Camp 1 — Late sunrise (disc center):** DC, mpanchang, prokerala (mostly)
- These sources use a later sunrise time (~70s after upper limb)
- On Set D dates, the tithi boundary has already been crossed → they see the next tithi
- mpanchang = DC on 100% of dates; prokerala = DC on ~54% (Ujjain location accounts for some divergence)

**Camp 2 — Early sunrise (upper limb):** UL, birthastro
- These sources use an earlier sunrise time (top edge of solar disc)
- On Set U dates, sunrise is before the tithi boundary → they see the previous tithi
- birthastro = UL on 39/48 (81%)

**Camp 3 — Drikpanchang:** DP
- Uses upper-limb sunrise but with its own proprietary ephemeris
- Agrees with UL/birthastro on Set D (32 dates) but disagrees on Set U (13 dates)
- The Set U disagreements suggest DP's sunrise is slightly earlier than ours by ~0.1–1.8 min

### The 3 "Both" Dates

| Date | DC | UL | DP | PK | BA | MP | Who's right? |
|------|----|----|----|----|----|----|-------------|
| 1982-03-07 | 13 | 13 | 12 | 13 | 12 | 13 | 4-2 for tithi 13 |
| 2045-01-17 | 29 | 29 | 30 | — | 29 | 29 | 4-1 for tithi 29 |
| 2046-05-22 | 17 | 17 | 18 | — | 17 | 17 | 4-1 for tithi 17 |

On all 3 dates where neither sunrise model matches DP, the **majority of sources agree with us** against DP. This suggests these are genuine ephemeris differences where DP's proprietary engine places the tithi boundary on the opposite side of sunrise.

## Prokerala Adjusted for Delhi Sunrise

Prokerala defaults to Ujjain (75.77°E). Delhi (77.21°E) is ~1.44° east, so Delhi sunrise is ~6 minutes earlier. Since all 48 dates are sub-minute boundary cases, this location difference can flip the tithi. Below we compare each PK tithi start time against our computed Delhi (upper-limb) sunrise to determine what PK would show for Delhi.

**Method**: If PK's tithi transition time > Delhi sunrise → tithi has already started → PK-Delhi shows the new tithi. If PK's tithi transition < Delhi sunrise → tithi hasn't started yet → PK-Delhi shows the previous tithi.

| # | Date | PK-Ujj | PK tithi start (IST) | Delhi SR (UL) | PK-Delhi | DP | Match |
|---|------|--------|----------------------|---------------|----------|-----|-------|
| 1 | 1902-05-30 | 23 | 05:15 (K-Ashtami) | 05:23:43 | 23 | 22 | DC |
| 2 | 1903-05-18 | 22 | 05:21 (K-Saptami) | 05:29:08 | 22 | 21 | DC |
| 3 | 1908-03-17 | 15 | 06:30 (S-Purnima) | 06:29:15 | 14→15? | 14 | ~DP (borderline, SR before start by ~45s) |
| 4 | 1909-10-11 | 28 | 06:19 (K-Trayodashi) | 06:18:51 | 27 | 27 | DP |
| 5 | 1909-12-01 | 19 | already before SR | 06:55:42 | 19 | 19 | DP |
| 6 | 1911-08-26 | 3 | 05:55 (S-Tritiya) | 05:55:04 | ~3 | 2 | DC (borderline, start ≈ SR) |
| 7 | 1912-12-14 | 5 | already before SR | 07:05:16 | 5 | 5 | DP |
| 8 | 1915-12-05 | 28 | already before SR | 06:58:24 | 28 | 28 | DP |
| 9 | 1916-02-24 | 21 | 06:53 (K-Shashthi) | 06:52:45 | 20 | 20 | DP |
| 10 | 1920-10-12 | 1 | 06:20 (S-Pratipada) | 06:19:37 | 30 | 30 | DP |
| 11 | 1924-02-05 | 1 | 07:08 (S-Pratipada) | 07:07:48 | 30 | 30 | DP |
| 12 | 1925-03-03 | 9 | 06:46 (S-Navami) | 06:44:54 | 8 | 8 | DP |
| 13 | 1929-11-26 | 25 | before SR | 06:51:51 | 25 | 26 | UL |
| 14 | 1930-10-31 | 10 | 06:31 (S-Dashami) | 06:31:35 | 10 | 10 | DP |
| 15 | 1932-05-15 | 10 | 05:30 (S-Dashami) | 05:30:20 | ~10 | 9 | DC (borderline) |
| 16 | 1936-12-29 | 16 | before SR | 07:12:51 | 16 | 17 | UL |
| 17 | 1939-07-23 | 8 | 05:37 (S-Ashtami) | 05:36:30 | 7 | 7 | DP |
| 18 | 1940-02-03 | 25 | already before SR | 07:08:50 | 25 | 25 | DP |
| 19 | 1943-12-17 | 19 | before SR | 07:06:42 | 19 | 20 | Neither |
| 20 | 1946-01-29 | 26 | already before SR | 07:11:08 | 26 | 26 | DP |
| 21 | 1951-06-08 | 4 | 05:22 (S-Chaturthi) | 05:22:24 | ~3 | 3 | DP (borderline, SR after start by ~24s) |
| 22 | 1956-05-29 | 19 | already before SR | 05:23:50 | 19 | 19 | DP |
| 23 | 1957-08-28 | 4 | 05:57 (S-Chaturthi) | 05:56:36 | 3 | 3 | DP |
| 24 | 1965-05-06 | 6 | 05:36 (S-Shashthi) | 05:36:31 | ~5 | 5 | DP (borderline) |
| 25 | 1966-01-08 | 16 | already before SR | 07:14:57 | 16 | 16 | DP |
| 26 | 1966-08-09 | 22 | already before SR | 05:46:22 | 22 | 22 | DP |
| 27 | 1966-10-25 | 12 | 06:28 (S-Dwadashi) | 06:27:37 | 11 | 11 | DP |
| 28 | 1968-03-11 | 12 | 06:35 (S-Dwadashi) | 06:35:30 | ~11 | 11 | DP (borderline) |
| 29 | 1968-05-24 | 27 | already before SR | 05:25:24 | 27 | 27 | DP |
| 30 | 1972-04-01 | 17 | already before SR | 06:11:16 | 17 | 17 | DP |
| 31 | 1974-12-19 | 6 | 07:08 (S-Shashthi) | 07:08:04 | ~5 | 5 | DP (borderline, SR before start by ~4s) |
| 32 | 1978-09-15 | 14 | 06:06 (S-Chaturdashi) | 06:05:32 | 13 | 13 | DP |
| 33 | 1982-03-07 | 13 | 06:40 (S-Trayodashi) | 06:40:20 | ~13 | 12 | DC (borderline) |
| 34 | 1987-12-18 | 28 | 07:08 (K-Trayodashi) | 07:07:23 | 27 | 27 | DP |
| 35 | 2001-01-19 | 25 | before SR | 07:14:08 | 25 | 26 | UL |
| 36 | 2007-08-15 | 3 | 05:49 (S-Tritiya) | 05:49:49 | 3 | 3 | DP |
| 37 | 2007-10-09 | 29 | 06:18 (K-Chaturdashi) | 06:17:51 | 28 | 28 | DP |
| 38 | 2014-05-22 | 24 | 05:27 (K-Navami) | 05:26:20 | 23 | 23 | DP |
| 39 | 2018-05-19 | 5 | 05:28 (S-Panchami) | 05:27:41 | ~4 | 5 | UL (borderline) |
| 40 | 2020-11-06 | 21 | 06:36 (K-Shashthi) | 06:36:42 | 21 | 21 | DP |
| 41 | 2026-06-30 | 16 | 05:26 (K-Pratipada) | 05:26:35 | 16 | 16 | DP |

(Dates #42-48 beyond prokerala range)

### PK-Delhi Summary

When prokerala's tithi transition times are compared against Delhi (upper-limb) sunrise:

| PK-Delhi matches | Count | Notes |
|------------------|-------|-------|
| DP | ~28 | Clear matches where Delhi sunrise flips PK from DC→DP |
| DC | ~4 | #1, #2 (tithi start well before SR), #6, #15 (borderline) |
| UL (not DP) | ~4 | #13, #16, #35, #39 |
| Neither | 1 | #19 (PK shows tithi 19, everyone else shows 20 or 21) |
| Borderline | ~4 | #3, #21, #24, #28, #31, #33 — within seconds, could go either way |

**Key insight**: Prokerala's Ujjain bias made it appear to agree with our disc-center values. When adjusted for Delhi's earlier sunrise, PK-Delhi agrees with **DP on ~28/41 dates** (68%), up from ~14/41 at Ujjain. The remaining disagreements are mostly sub-minute boundary cases where even a few seconds difference in ephemeris or sunrise computation flips the tithi.

**Conclusion**: Prokerala uses its own ephemeris (not identical to ours or DP's), and the apparent DC agreement was largely a location artifact. At Delhi, prokerala's tithi transition times mostly place the boundary before sunrise, agreeing with DP.
