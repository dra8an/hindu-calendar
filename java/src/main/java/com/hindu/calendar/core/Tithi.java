package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.Paksha;
import com.hindu.calendar.model.TithiInfo;

public class Tithi {

    private final Ephemeris ephemeris;

    public Tithi(Ephemeris ephemeris) {
        this.ephemeris = ephemeris;
    }

    public double lunarPhase(double jdUt) {
        double moon = ephemeris.lunarLongitude(jdUt);
        double sun = ephemeris.solarLongitude(jdUt);
        double phase = (moon - sun) % 360.0;
        if (phase < 0) phase += 360.0;
        return phase;
    }

    public int tithiAtMoment(double jdUt) {
        double phase = lunarPhase(jdUt);
        int t = (int) (phase / 12.0) + 1;
        if (t > 30) t = 30;
        return t;
    }

    public double findTithiBoundary(double jdStart, double jdEnd, int targetTithi) {
        double targetPhase = (targetTithi - 1) * 12.0;

        double lo = jdStart;
        double hi = jdEnd;

        for (int i = 0; i < 50; i++) {
            double mid = (lo + hi) / 2.0;
            double phase = lunarPhase(mid);

            double diff = phase - targetPhase;
            if (diff > 180.0) diff -= 360.0;
            if (diff < -180.0) diff += 360.0;

            if (diff >= 0)
                hi = mid;
            else
                lo = mid;
        }

        return (lo + hi) / 2.0;
    }

    public TithiInfo tithiAtSunrise(int year, int month, int day, Location loc) {
        double jd = ephemeris.gregorianToJd(year, month, day);
        double jdRise = ephemeris.sunriseJd(jd, loc);

        if (jdRise <= 0) {
            jdRise = jd + 0.5 - loc.utcOffset() / 24.0;
        }

        double riseUt = jdRise;
        int t = tithiAtMoment(riseUt);

        Paksha paksha = (t <= 15) ? Paksha.SHUKLA : Paksha.KRISHNA;
        int pakshaTithi = (t <= 15) ? t : t - 15;

        double jdStart = findTithiBoundary(riseUt - 2.0, riseUt, t);

        int nextTithi = (t % 30) + 1;
        double jdEnd = findTithiBoundary(riseUt, riseUt + 2.0, nextTithi);

        // Check for kshaya tithi
        boolean isKshaya = false;
        double jdTomorrow = jd + 1.0;
        double jdRiseTmrw = ephemeris.sunriseJd(jdTomorrow, loc);
        if (jdRiseTmrw > 0) {
            int tTmrw = tithiAtMoment(jdRiseTmrw);
            int diff = ((tTmrw - t) % 30 + 30) % 30;
            isKshaya = (diff > 1);
        }

        return new TithiInfo(t, paksha, pakshaTithi, jdStart, jdEnd, isKshaya);
    }
}
