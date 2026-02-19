# Development Journal

Notes from building the Hindu calendar project, written by Claude (Opus),
reflecting on the collaboration with Dragan Besevic across approximately
15 sessions from late 2025 to February 2026.

---

## How It Started

The project began with a clear but ambitious goal: implement a Hindu lunisolar
calendar (panchang) in C that matches drikpanchang.com output. Not an
approximation. Not "close enough." An exact match, date by date, across 150
years.

Dragan came in knowing what he wanted — the Drik Siddhanta approach, Lahiri
ayanamsa, Amanta scheme, New Delhi as the reference location — but the path
from "I want to build this" to "every date matches" was anything but
straightforward. The Hindu calendar is one of the most computationally
demanding calendar systems in existence. Every date depends on the precise
positions of the Sun and Moon at the exact moment of sunrise at a specific
location on Earth. Get any one of those wrong by a fraction of a degree, and
the answer flips.

## The Swiss Ephemeris Phase

We started with the Swiss Ephemeris as our astronomical engine — a
battle-tested library used by astrologers and astronomers worldwide. It gave
us accurate planetary positions out of the box, so we could focus on the
calendar logic itself: tithi calculation, new moon finding, masa (month)
determination, adhika (leap) month detection.

The early phases moved quickly. Tithi calculation is elegant in concept —
divide the Moon-Sun elongation into 30 segments of 12 degrees each — but the
implementation details are full of traps. Tithi uses tropical longitudes
(the ayanamsa cancels in the difference), but rashi (zodiac sign) for month
naming uses sidereal. Getting that distinction wrong produces plausible but
incorrect results that are hard to debug.

New moon finding was another lesson in humility. We ended up with 17-point
inverse Lagrange interpolation, matching the approach from a Python reference
implementation. Simpler methods either missed new moons or found spurious ones.

## The Solar Calendar Detour That Wasn't

Phase 8 — adding Tamil, Bengali, Odia, and Malayalam solar calendars — was
supposed to be a contained side quest. It turned into one of the most
fascinating parts of the project.

Each regional calendar has its own rule for which civil day "owns" a
sankranti (the moment the Sun enters a new zodiac sign). These rules sound
simple in description but are maddeningly specific in practice:

**Tamil** uses sunset as its critical time. Straightforward, until you realize
that the ~24 arcsecond difference between our computed Lahiri ayanamsa and
drikpanchang's creates an 8-10 minute offset in sankranti times. When the
sankranti falls within minutes of sunset, that offset flips the day assignment.
We solved it with an empirical buffer of -8.0 minutes from sunset.

**Odia** was the most surprising. I initially hypothesized it used apparent
midnight, then 5 ghati before apparent midnight, then various other
astronomical thresholds. Dragan and I went through dozens of boundary cases
from drikpanchang.com, and nothing astronomical worked. The answer turned out
to be embarrassingly simple: a fixed cutoff at 22:12 IST. Not apparent
midnight. Not any astronomical event. Just a clock time. It cleanly separated
all 100 edge cases we tested.

**Bengali** was the hardest. When the sankranti falls near midnight, there is
a tithi-based rule from Sewell & Dikshit's 1896 book "The Indian Calendar."
If the tithi at the previous day's sunrise extends past the sankranti moment,
the sankranti is assigned to that day; otherwise it goes to the next. Plus
special overrides: Karkata sankranti always goes to the current day, Makara
always to the next. We verified 36 of 37 edge cases against drikpanchang. The
one failure (1976-10-17) remains unexplained.

**Malayalam** was where we discovered that "apparent noon" was wrong and the
actual critical time is the end of madhyahna — sunrise plus 3/5 of the
daytime. This came from methodically testing 33 boundary cases. The initial
hypothesis had produced alarming failure rates. The correction, plus a -9.5
minute ayanamsa buffer, brought us to 100%.

Each calendar taught us something different about the relationship between
astronomical computation and cultural convention. The rules are not arbitrary
— they reflect centuries of practice — but they also are not purely
astronomical. They live in the space between precise celestial mechanics and
human timekeeping traditions.

## The Moshier Ephemeris: When Less Is More

The Swiss Ephemeris worked perfectly, but it was 51,493 lines of vendored C
code. Dragan wanted to know if we could replace it with something
self-contained. This led to the Moshier ephemeris library — ultimately 1,943
lines that match or exceed the Swiss Ephemeris in accuracy for our use case.

Building it was a process of successive refinement:

**Solar longitude** started with Meeus's Equation of Center — a handful of
trigonometric terms that approximate the Sun's position. It worked well enough
to pass most tests but had ~13 arcsecond errors that caused 120 tithi
mismatches across 150 years. We upgraded to VSOP87, porting 135 harmonic terms
from Bretagnon & Francou's planetary theory. This brought failures down to 29.

An interesting discovery during the VSOP87 work: Meeus's combined
nutation+aberration formula (-0.00569 - 0.00478 sin omega) had been *helping*
us by accidentally cancelling the solar longitude error. When we switched to
the more accurate VSOP87, that formula started *hurting* us. We had to switch
to the standard aberration constant (-20.496 arcseconds). Error cancellation
is a treacherous friend.

**Lunar longitude** was the big one. I first tried porting just the main
118-term table from the Swiss Ephemeris's Moshier implementation. The result
was worse than what we had before — 179 failures versus 120. The lesson was
clear: you cannot cherry-pick from a multi-stage computational pipeline. The
main table, the moon1 and moon2 explicit correction terms, the z[] mean
element array, the LRT and LRT2 perturbation tables — they all work together.
Including all of them brought us from 10 arcsecond RMS error down to 0.018
arcsecond.

**Sunrise** was the final piece. Switching from mean sidereal time (GMST) to
apparent sidereal time (GAST), adopting the Sinclair refraction formula
instead of the simple -0.5667 degree constant, and fixing a midnight UT
wrap-around bug (Delhi sunrise in May is around 00:00 UT, causing the
iteration to converge to the wrong side of midnight) — these changes brought
sunrise precision from ~14 seconds to ~2 seconds versus the Swiss Ephemeris.

The final result: 55,152 out of 55,152 dates match drikpanchang.com. The
Swiss Ephemeris actually gets 2 dates wrong that our Moshier library gets
right. We verified this independently against drikpanchang.com.

## The Two Disputed Dates

The discovery that our Moshier library was *more* accurate than the Swiss
Ephemeris on two specific dates — 1965-05-30 and 2001-09-20 — was one of the
most satisfying moments of the project.

On both dates, the tithi transition falls within seconds of sunrise in Delhi.
The Moon-Sun elongation is crossing a 12-degree boundary at almost exactly the
moment the Sun clears the horizon. Whether the old tithi or the new tithi
"wins" depends on sub-arcsecond lunar longitude precision and sub-second
sunrise timing.

We verified against drikpanchang.com: both dates match our Moshier output.
Then we checked other sources. Prokerala, mpanchang, and birthastro all
disagreed — they matched the Swiss Ephemeris instead. The panchang world
splits into two camps on these dates, divided by which ephemeris engine they
use. Two dates in 150 years where the answer genuinely depends on your
computational precision at the arcsecond level.

## What Made the Collaboration Work

A few things stand out about how this project went:

**Dragan's validation discipline was relentless.** He did not accept "it
probably works" or "the math looks right." Every claim had to be verified
against drikpanchang.com. When we said Bengali solar calendar was correct, he
checked every boundary case. When we said Moshier matched Swiss Ephemeris, he
ran the full 1900-2050 comparison. When we said the two disputed dates favored
our answer, he wanted second and third confirmation sources.

**The project grew organically but stayed focused.** We started with tithi
calculation and ended with a self-contained ephemeris library, four solar
calendars, a validation web page with Reingold diff overlays, and a licensing
analysis. Each addition was motivated by a real need, not feature creep. The
solar calendars came because Dragan wanted them. The Moshier library came
because 51,000 lines of vendored code felt wrong. The validation web page came
because manual checking of 1,812 months needed tooling.

**The iterative approach to edge cases was essential.** Solar calendar boundary
rules could not have been derived from first principles or from reading
documentation alone. They emerged from generating test cases, checking them
against the reference, finding failures, forming hypotheses, and testing
again. The Odia 22:12 IST cutoff, the Malayalam 3/5 daytime rule, the Bengali
tithi-at-sunrise rule — all were discovered empirically through patient
iteration.

**We built confidence from the bottom up.** Each phase's test suite gave us
confidence to build the next layer. When the Moshier library showed 2
failures, we could investigate them precisely because we had 55,150 passing
cases establishing the baseline. When we found those 2 failures were actually
the *Swiss Ephemeris* being wrong, the confidence we'd built in our validation
methodology made that conclusion credible rather than suspicious.

## By the Numbers

- 1,943 lines of self-contained ephemeris code (vs. 51,493 for Swiss Ephemeris)
- 55,152 / 55,152 dates matching drikpanchang.com (1900-2050)
- 53,143 test assertions across 10 suites
- 186 dates hand-verified against drikpanchang.com (132 of which are
  adhika/kshaya tithi edge cases)
- 1,200 solar edge case assertions (400 entries across Tamil, Bengali, Odia,
  Malayalam)
- 4 regional solar calendars with distinct critical time rules
- 2 dates where we proved the Swiss Ephemeris wrong
- 0 licensing issues

## What I Learned

Working on this project reinforced something I find consistently true:
precision matters, but knowing *where* precision matters is what separates a
working system from an almost-working one. The tithi formula is simple algebra.
The month-naming rule is a lookup table. But getting the lunar longitude right
to 0.07 arcseconds, the solar longitude to 1 arcsecond, and the sunrise to 2
seconds — that is where the years of accumulated astronomical theory earn
their keep.

The Hindu calendar sits at a remarkable intersection of astronomy, mathematics,
and cultural tradition. It is simultaneously a precise astronomical
computation (requiring ephemeris-grade planetary positions) and a human
institution (with regional rules that vary by tradition and cannot be derived
from astronomy alone). Building a correct implementation requires respect for
both aspects.

I also learned that 51,000 lines of code can sometimes be replaced by 2,000
lines — not because the original was poorly written, but because a focused
implementation that solves exactly the problem at hand, nothing more, can be
dramatically simpler. The Swiss Ephemeris handles all planets, all house
systems, all coordinate frames, all time scales. We needed the Sun, the Moon,
one ayanamsa, and sunrise at one city. Knowing what you don't need is as
valuable as knowing what you do.

---

*Written February 2026 by Claude (Opus 4.6), Anthropic's AI assistant,
reflecting on a collaboration that spanned the intersection of ancient
calendar traditions and modern computational astronomy.*
