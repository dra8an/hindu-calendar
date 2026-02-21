package com.hindu.calendar.core;

import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.*;

public class Panchang {

    private final Ephemeris ephemeris;
    private final Tithi tithi;
    private final Masa masa;

    public Panchang(Ephemeris ephemeris) {
        this.ephemeris = ephemeris;
        this.tithi = new Tithi(ephemeris);
        this.masa = new Masa(ephemeris, tithi);
    }

    public Tithi getTithi() {
        return tithi;
    }

    public Masa getMasa() {
        return masa;
    }

    public HinduDate gregorianToHindu(int year, int month, int day, Location loc) {
        TithiInfo ti = tithi.tithiAtSunrise(year, month, day, loc);
        MasaInfo mi = masa.masaForDate(year, month, day, loc);

        boolean isAdhikaTithi = false;
        if (day > 1) {
            TithiInfo tiPrev = tithi.tithiAtSunrise(year, month, day - 1, loc);
            isAdhikaTithi = (ti.tithiNum() == tiPrev.tithiNum());
        }

        return new HinduDate(
                mi.yearSaka(), mi.yearVikram(),
                mi.name(), mi.isAdhika(),
                ti.paksha(), ti.pakshaTithi(),
                isAdhikaTithi
        );
    }

    public PanchangDay[] generateMonthPanchang(int year, int month, Location loc) {
        int ndays = DateUtils.daysInMonth(year, month);
        PanchangDay[] days = new PanchangDay[ndays];

        for (int d = 1; d <= ndays; d++) {
            double jd = ephemeris.gregorianToJd(year, month, d);
            double jdSunrise = ephemeris.sunriseJd(jd, loc);
            TithiInfo ti = tithi.tithiAtSunrise(year, month, d, loc);
            HinduDate hd = gregorianToHindu(year, month, d, loc);

            days[d - 1] = new PanchangDay(year, month, d, jdSunrise, hd, ti);
        }

        return days;
    }

    /** Format JD as local time [h, m, s] given UTC offset */
    public static int[] jdToLocalTime(double jdUt, double utcOffset) {
        double localJd = jdUt + 0.5 + utcOffset / 24.0;
        double frac = localJd - Math.floor(localJd);
        double hours = frac * 24.0;
        int h = (int) hours;
        int m = (int) ((hours - h) * 60.0);
        int s = (int) (((hours - h) * 60.0 - m) * 60.0 + 0.5);
        if (s == 60) { s = 0; m++; }
        if (m == 60) { m = 0; h++; }
        return new int[]{h, m, s};
    }

    public String formatMonthPanchang(PanchangDay[] days, double utcOffset) {
        if (days.length == 0) return "";

        StringBuilder sb = new StringBuilder();
        sb.append(String.format("%-12s %-5s %-10s %-28s %s%n",
                "Date", "Day", "Sunrise", "Tithi", "Hindu Date"));
        sb.append(String.format("%-12s %-5s %-10s %-28s %s%n",
                "----------", "---", "--------", "----------------------------",
                "----------------------------"));

        for (PanchangDay pd : days) {
            double jd = ephemeris.gregorianToJd(pd.gregYear(), pd.gregMonth(), pd.gregDay());
            int dow = ephemeris.dayOfWeek(jd);

            int[] hms = jdToLocalTime(pd.jdSunrise(), utcOffset);

            String pakshaStr = (pd.tithi().paksha() == Paksha.SHUKLA) ? "Shukla" : "Krishna";
            int pt = pd.tithi().pakshaTithi();
            String tithiName;
            if (pd.tithi().tithiNum() == 30) {
                tithiName = "Amavasya";
            } else if (pd.tithi().tithiNum() == 15) {
                tithiName = "Purnima";
            } else {
                tithiName = TithiInfo.TITHI_NAMES[pt];
            }

            String masaStr = pd.hinduDate().masa().displayName();
            String adhikaPrefix = pd.hinduDate().isAdhikaMasa() ? "Adhika " : "";

            sb.append(String.format("%04d-%02d-%02d   %-5s %02d:%02d:%02d   %-6s %-13s (%s-%d)   %s%s %s %d, Saka %d%n",
                    pd.gregYear(), pd.gregMonth(), pd.gregDay(),
                    DateUtils.dayOfWeekShort(dow),
                    hms[0], hms[1], hms[2],
                    pakshaStr, tithiName,
                    (pd.tithi().paksha() == Paksha.SHUKLA) ? "S" : "K",
                    pt,
                    adhikaPrefix, masaStr, pakshaStr, pt,
                    pd.hinduDate().yearSaka()));
        }

        return sb.toString();
    }

    public String formatDayPanchang(PanchangDay day, double utcOffset) {
        double jd = ephemeris.gregorianToJd(day.gregYear(), day.gregMonth(), day.gregDay());
        int dow = ephemeris.dayOfWeek(jd);

        int[] hms = jdToLocalTime(day.jdSunrise(), utcOffset);

        String pakshaStr = (day.tithi().paksha() == Paksha.SHUKLA) ? "Shukla" : "Krishna";
        int pt = day.tithi().pakshaTithi();
        String tithiName;
        if (day.tithi().tithiNum() == 30) {
            tithiName = "Amavasya";
        } else if (day.tithi().tithiNum() == 15) {
            tithiName = "Purnima";
        } else {
            tithiName = TithiInfo.TITHI_NAMES[pt];
        }

        String masaStr = day.hinduDate().masa().displayName();
        String adhikaPrefix = day.hinduDate().isAdhikaMasa() ? "Adhika " : "";

        StringBuilder sb = new StringBuilder();
        sb.append(String.format("Date:       %04d-%02d-%02d (%s)%n",
                day.gregYear(), day.gregMonth(), day.gregDay(),
                DateUtils.dayOfWeekName(dow)));
        sb.append(String.format("Sunrise:    %02d:%02d:%02d IST%n", hms[0], hms[1], hms[2]));
        sb.append(String.format("Tithi:      %s %s (%s-%d)%n", pakshaStr, tithiName,
                (day.tithi().paksha() == Paksha.SHUKLA) ? "S" : "K", pt));
        sb.append(String.format("Hindu Date: %s%s %s %d, Saka %d (Vikram %d)%n",
                adhikaPrefix, masaStr, pakshaStr, pt,
                day.hinduDate().yearSaka(), day.hinduDate().yearVikram()));
        if (day.tithi().isKshaya())
            sb.append("Note:       Kshaya tithi (next tithi is skipped)\n");
        if (day.hinduDate().isAdhikaTithi())
            sb.append("Note:       Adhika tithi (same tithi as previous day)\n");

        return sb.toString();
    }
}
