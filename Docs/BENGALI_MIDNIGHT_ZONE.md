# Bengali Midnight Zone Analysis

All Bengali sankrantis falling within the expanded midnight zone
(23:26–00:34 IST, ±34 min from midnight) across 1900–2050, sorted by
delta from midnight. This covers the 48-min midnight rule zone
(23:36–00:24) plus a 10-min margin on each side.

## Summary

| Metric | Value |
|--------|-------|
| Total month boundaries (1900–2050) | 1,812 |
| In expanded zone (23:26–00:34) | 82 (4.5%) |
| Correct | 74 (90.2%) |
| Wrong | 8 (9.8%) |

### Rule breakdown

| Rule | Count | Correct | Wrong |
|------|-------|---------|-------|
| Tithi rule | 65 | 59 | 6 |
| Karkata exception (always current day) | 9 | 8 | 1 |
| Makara exception (always next day) | 8 | 8 | 0 |

### Pre-midnight vs post-midnight

| Zone | Count | Correct | Wrong |
|------|-------|---------|-------|
| Pre-midnight (23:26–00:00) | 39 | 33 | 6 |
| Post-midnight (00:00–00:34) | 43 | 41 | 2 |

## Full Table

```
 #  Delta     Sank. IST      R#  Rashi       Month      Start date   Rule        Match
--- -------   ------------   --  ----------  ---------  -----------  ----------  --------
 1   -33.7m   23:26:18      R8  Vrischika   Ogrohayon  1933-11-15  tithi       ok
 2   -33.5m   23:26:32      R3  Mithuna     Asharh     1977-06-14  tithi       ok
 3   -32.3m   23:27:43      R3  Mithuna     Asharh     1938-06-14  tithi       ok
 4   -31.5m   23:28:28      R8  Vrischika   Ogrohayon  2011-11-16  tithi       ok
 5   -31.2m   23:28:45      R11 Kumbha      Falgun     1920-02-12  tithi       ok
 6   -30.1m   23:29:55      R7  Tula        Kartik     2050-10-17  tithi       ok
 7   -28.0m   23:31:57      R6  Kanya       Ashshin    1970-09-16  tithi       ok
 8   -27.8m   23:32:12      R11 Kumbha      Falgun     1959-02-12  tithi       ok
 9   -27.6m   23:32:23      R2  Vrishabha   Jyoishtho  2021-05-14  tithi       ok
10   -27.4m   23:32:34      R4  Karkata     Srabon     1944-07-15  Karkata(C)  ok
11   -27.1m   23:32:54      R1  Mesha       Boishakh   1923-04-13  tithi       ok
12   -26.6m   23:33:24      R12 Meena       Choitro    1940-03-13  tithi       ok
13   -25.9m   23:34:04      R4  Karkata     Srabon     1905-07-15  Karkata(C)  ok
14   -24.4m   23:35:38      R4  Karkata     Srabon     2026-07-16  Karkata(C)  ok
15   -24.2m   23:35:45      R10 Makara      Magh       2043-01-14  Makara(W)   ok
16   -24.0m   23:35:57      R1  Mesha       Boishakh   1962-04-13  tithi       ok
                             --- 23:36 boundary (midnight zone starts) ---
17   -23.7m   23:36:20      R6  Kanya       Ashshin    1931-09-16  tithi       ok
18   -22.8m   23:37:09      R10 Makara      Magh       1926-01-13  Makara(W)   ok
19   -22.7m   23:37:20      R10 Makara      Magh       1965-01-13  Makara(W)   ok
20   -22.3m   23:37:39      R1  Mesha       Boishakh   2040-04-13  tithi       ok
21   -22.0m   23:37:58      R5  Simha       Bhadro     1970-08-16  tithi       ok
22   -21.8m   23:38:12      R2  Vrishabha   Jyoishtho  1904-05-13  tithi       ok
23   -20.6m   23:39:22      R12 Meena       Choitro    1979-03-14  tithi       ok
24   -20.0m   23:40:01      R7  Tula        Kartik     2011-10-17  tithi       WRONG  (drik: +1 day)
25   -19.5m   23:40:27      R7  Tula        Kartik     1972-10-16  tithi       WRONG  (drik: +1 day)
26   -18.1m   23:41:52      R2  Vrishabha   Jyoishtho  1943-05-14  tithi       ok
27   -17.0m   23:43:00      R2  Vrishabha   Jyoishtho  1982-05-14  tithi       ok
28   -15.8m   23:44:13      R5  Simha       Bhadro     1931-08-16  tithi       ok
29   -14.5m   23:45:31      R4  Karkata     Srabon     1987-07-16  Karkata(C)  ok
30   -13.7m   23:46:20      R11 Kumbha      Falgun     1998-02-12  tithi       ok
31   -12.9m   23:47:07      R10 Makara      Magh       2004-01-14  Makara(W)   ok
32   -12.3m   23:47:40      R12 Meena       Choitro    2018-03-14  tithi       ok
33   -12.1m   23:47:54      R7  Tula        Kartik     1933-10-16  tithi       WRONG  (drik: +1 day)
34   -10.9m   23:49:04      R1  Mesha       Boishakh   2001-04-13  tithi       ok
35   -10.9m   23:49:08      R9  Dhanu       Poush      1958-12-15  tithi       WRONG  (drik: +1 day)
36    -9.4m   23:50:36      R9  Dhanu       Poush      1919-12-15  tithi       ok
37    -7.8m   23:52:10      R11 Kumbha      Falgun     2037-02-12  tithi       ok
38    -3.2m   23:56:48      R6  Kanya       Ashshin    1974-09-16  tithi       WRONG  (drik: +1 day)
39    -1.6m   23:58:24      R6  Kanya       Ashshin    2013-09-16  tithi       WRONG  (drik: +1 day)
                             --- 00:00 midnight ---
40    +1.2m   00:01:10      R4  Karkata     Srabon     1948-07-16  Karkata(C)  ok
41    +1.4m   00:01:23      R8  Vrischika   Ogrohayon  1937-11-16  tithi       ok
42    +1.4m   00:01:23      R3  Mithuna     Asharh     2020-06-15  tithi       ok
43    +1.8m   00:01:47      R5  Simha       Bhadro     1974-08-17  tithi       ok
44    +2.8m   00:02:50      R5  Simha       Bhadro     2013-08-17  tithi       ok
45    +3.7m   00:03:40      R1  Mesha       Boishakh   2044-04-14  tithi       ok
46    +4.4m   00:04:22      R8  Vrischika   Ogrohayon  2015-11-17  tithi       ok
47    +4.6m   00:04:35      R10 Makara      Magh       2008-01-15  Makara(W)   ok
48    +5.1m   00:05:05      R9  Dhanu       Poush      1997-12-16  tithi       ok
49    +5.7m   00:05:39      R9  Dhanu       Poush      2036-12-16  tithi       ok
50    +6.1m   00:06:08      R11 Kumbha      Falgun     1924-02-13  tithi       ok
51    +7.7m   00:07:39      R4  Karkata     Srabon     2030-07-17  Karkata(C)  ok
52    +8.0m   00:08:00      R8  Vrischika   Ogrohayon  1976-11-16  tithi       ok
53    +8.9m   00:08:55      R2  Vrishabha   Jyoishtho  1986-05-15  tithi       ok
54    +9.6m   00:09:36      R10 Makara      Magh       1930-01-14  Makara(W)   ok
55    +9.8m   00:09:45      R3  Mithuna     Asharh     1942-06-15  tithi       ok
56   +12.0m   00:11:57      R1  Mesha       Boishakh   2005-04-14  tithi       ok
57   +12.0m   00:11:59      R2  Vrishabha   Jyoishtho  2025-05-15  tithi       ok
58   +12.7m   00:12:42      R10 Makara      Magh       1969-01-14  Makara(W)   ok
59   +13.3m   00:13:15      R3  Mithuna     Asharh     1903-06-15  tithi       ok
60   +13.8m   00:13:46      R4  Karkata     Srabon     1991-07-17  Karkata(C)  ok
61   +14.1m   00:14:08      R3  Mithuna     Asharh     1981-06-15  tithi       ok
62   +14.4m   00:14:24      R12 Meena       Choitro    1905-03-14  tithi       ok
63   +14.5m   00:14:30      R6  Kanya       Ashshin    1935-09-17  tithi       ok
64   +15.1m   00:15:08      R1  Mesha       Boishakh   1927-04-14  tithi       ok
65   +16.7m   00:16:42      R7  Tula        Kartik     2015-10-18  tithi       ok
66   +18.2m   00:18:14      R2  Vrishabha   Jyoishtho  1947-05-15  tithi       ok
67   +18.6m   00:18:36      R12 Meena       Choitro    1944-03-14  tithi       ok
68   +18.8m   00:18:49      R7  Tula        Kartik     1937-10-17  tithi       ok
69   +19.7m   00:19:41      R5  Simha       Bhadro     1935-08-17  tithi       ok
70   +19.8m   00:19:49      R4  Karkata     Srabon     1909-07-16  Karkata(C)  ok
71   +21.1m   00:21:05      R10 Makara      Magh       2047-01-15  Makara(W)   ok
72   +21.6m   00:21:36      R12 Meena       Choitro    1983-03-15  tithi       ok
73   +21.8m   00:21:48      R12 Meena       Choitro    2022-03-15  tithi       ok
74   +23.3m   00:23:15      R7  Tula        Kartik     1976-10-17  tithi       WRONG  (drik: +1 day)
75   +23.8m   00:23:46      R11 Kumbha      Falgun     1963-02-13  tithi       ok
76   +23.9m   00:23:54      R2  Vrishabha   Jyoishtho  1908-05-14  tithi       ok
                             --- 00:24 boundary (midnight zone ends) ---
77   +24.9m   00:24:51      R1  Mesha       Boishakh   1966-04-14  tithi       ok
78   +25.1m   00:25:03      R11 Kumbha      Falgun     2041-02-13  tithi       ok
79   +28.0m   00:27:57      R11 Kumbha      Falgun     2002-02-13  tithi       ok
80   +29.0m   00:29:00      R3  Mithuna     Asharh     2024-06-15  tithi       ok
81   +31.2m   00:31:10      R9  Dhanu       Poush      1923-12-16  tithi       ok
82   +31.6m   00:31:38      R4  Karkata     Srabon     1952-07-16  Karkata(C)  WRONG  (drik: -1 day)
```

## Observations

### All 8 failures

| # | Delta | Rashi | Month | Rule | Notes |
|---|-------|-------|-------|------|-------|
| 24 | -20.0m | R7 Tula | Kartik 2011 | tithi | Pre-midnight, our=Oct 17, drik=Oct 18 |
| 25 | -19.5m | R7 Tula | Kartik 1972 | tithi | Pre-midnight, our=Oct 16, drik=Oct 17 |
| 33 | -12.1m | R7 Tula | Kartik 1933 | tithi | Pre-midnight, our=Oct 17, drik=Oct 18 |
| 35 | -10.9m | R9 Dhanu | Poush 1958 | tithi | Pre-midnight, our=Dec 15, drik=Dec 16 |
| 38 | -3.2m | R6 Kanya | Ashshin 1974 | tithi | Pre-midnight, our=Sep 17, drik=Sep 18 |
| 39 | -1.6m | R6 Kanya | Ashshin 2013 | tithi | Pre-midnight, our=Sep 16, drik=Sep 17 |
| 74 | +23.3m | R7 Tula | Kartik 1976 | tithi | Post-midnight, our=Oct 17, drik=Oct 18 |
| 82 | +31.6m | R4 Karkata | Srabon 1952 | Karkata(C) | Outside zone, our=Jul 17, drik=Jul 16 |

### Patterns

1. **7 of 8 failures are pre-midnight** (sankranti before 00:00 IST). Our
   tithi rule assigns the month start to the current day; drikpanchang
   assigns it to the next day (+1).

2. **Only 3 rashis fail**: Tula (R7, 4 cases), Kanya (R6, 2 cases),
   Dhanu (R9, 1 case). The remaining rashis (R1-R5, R8, R10-R12) are
   all correct.

3. **Karkata exception fails once** (#82, Srabon 1952): the sankranti is
   at +31.6m, well outside the midnight zone. The Karkata "always current
   day" rule forces Jul 17 but drikpanchang says Jul 16 (-1 day).

4. **Makara exception is perfect**: all 8 cases correct.

5. **Pre-midnight margin (23:26–23:36) is clean**: all 16 cases correct,
   nearest to the zone boundary at -24.0m (#16).

6. **Post-midnight margin (00:24–00:34) has 1 failure**: Srabon 1952
   (#82) at +31.6m. This is the Karkata exception case — the only
   failure outside the standard midnight zone.

7. **Correct cases span the full range**: correctly-resolved sankrantis
   appear at all deltas from -33.7m to +23.9m, interleaved with the
   failures. There is no clean delta threshold that separates correct
   from incorrect.
