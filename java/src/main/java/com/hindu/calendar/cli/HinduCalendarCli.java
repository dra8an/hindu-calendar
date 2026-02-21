package com.hindu.calendar.cli;

import com.hindu.calendar.core.*;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.*;

import java.time.LocalDate;

public class HinduCalendarCli {

    private static void printUsage() {
        System.err.println("""
                Usage: hindu-calendar [options]
                  -y YEAR      Gregorian year (default: current)
                  -m MONTH     Gregorian month 1-12 (default: current)
                  -d DAY       Specific day (if omitted, shows full month)
                  -s TYPE      Solar calendar: tamil, bengali, odia, malayalam
                               (if omitted, shows lunisolar panchang)
                  -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)
                  -u OFFSET    UTC offset in hours (default: 5.5)
                  -h           Show this help""");
    }

    public static void main(String[] args) {
        LocalDate now = LocalDate.now();
        int year = now.getYear();
        int month = now.getMonthValue();
        int day = 0;
        Location loc = Location.NEW_DELHI;
        boolean solarMode = false;
        SolarCalendarType solarType = SolarCalendarType.TAMIL;

        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-y" -> year = Integer.parseInt(args[++i]);
                case "-m" -> month = Integer.parseInt(args[++i]);
                case "-d" -> day = Integer.parseInt(args[++i]);
                case "-s" -> {
                    solarType = SolarCalendarType.fromString(args[++i]);
                    solarMode = true;
                }
                case "-l" -> {
                    String[] parts = args[++i].split(",");
                    loc = new Location(Double.parseDouble(parts[0]),
                            Double.parseDouble(parts[1]),
                            loc.altitude(), loc.utcOffset());
                }
                case "-u" -> {
                    double offset = Double.parseDouble(args[++i]);
                    loc = new Location(loc.latitude(), loc.longitude(), loc.altitude(), offset);
                }
                case "-h" -> {
                    printUsage();
                    return;
                }
                default -> {
                    System.err.println("Unknown option: " + args[i]);
                    printUsage();
                    System.exit(1);
                }
            }
        }

        if (month < 1 || month > 12) {
            System.err.println("Error: month must be 1-12");
            System.exit(1);
        }

        Ephemeris ephemeris = new Ephemeris();

        if (solarMode) {
            Solar solar = new Solar(ephemeris);
            if (day > 0) {
                printSolarDay(ephemeris, solar, year, month, day, loc, solarType);
            } else {
                printSolarMonth(ephemeris, solar, year, month, loc, solarType);
            }
        } else {
            Panchang panchang = new Panchang(ephemeris);
            if (day > 0) {
                double jd = ephemeris.gregorianToJd(year, month, day);
                double jdSunrise = ephemeris.sunriseJd(jd, loc);
                TithiInfo ti = panchang.getTithi().tithiAtSunrise(year, month, day, loc);
                HinduDate hd = panchang.gregorianToHindu(year, month, day, loc);
                PanchangDay pd = new PanchangDay(year, month, day, jdSunrise, hd, ti);
                System.out.print(panchang.formatDayPanchang(pd, loc.utcOffset()));
            } else {
                System.out.printf("Hindu Calendar — %04d-%02d (%.4f°N, %.4f°E, UTC%+.1f)%n%n",
                        year, month, loc.latitude(), loc.longitude(), loc.utcOffset());
                PanchangDay[] days = panchang.generateMonthPanchang(year, month, loc);
                System.out.print(panchang.formatMonthPanchang(days, loc.utcOffset()));
            }
        }
    }

    private static void printSolarMonth(Ephemeris ephemeris, Solar solar,
                                         int year, int month, Location loc,
                                         SolarCalendarType type) {
        int ndays = DateUtils.daysInMonth(year, month);
        SolarDate sd1 = solar.gregorianToSolar(year, month, 1, loc, type);
        String calName = type.name().charAt(0) + type.name().substring(1).toLowerCase();

        System.out.printf("%s Solar Calendar — %s %d (%s)%n",
                calName, type.monthName(sd1.month()), sd1.year(), type.eraName());
        System.out.printf("Gregorian %04d-%02d%n%n", year, month);

        System.out.printf("%-12s %-5s %-20s %s%n", "Date", "Day", "Solar Date", "");
        System.out.printf("%-12s %-5s %-20s%n", "----------", "---", "--------------------");

        for (int d = 1; d <= ndays; d++) {
            SolarDate sd = solar.gregorianToSolar(year, month, d, loc, type);
            double jd = ephemeris.gregorianToJd(year, month, d);
            int dow = ephemeris.dayOfWeek(jd);

            System.out.printf("%04d-%02d-%02d   %-5s %s %d, %d%n",
                    year, month, d,
                    DateUtils.dayOfWeekShort(dow),
                    type.monthName(sd.month()),
                    sd.day(), sd.year());
        }
    }

    private static void printSolarDay(Ephemeris ephemeris, Solar solar,
                                       int year, int month, int day, Location loc,
                                       SolarCalendarType type) {
        SolarDate sd = solar.gregorianToSolar(year, month, day, loc, type);
        double jd = ephemeris.gregorianToJd(year, month, day);
        int dow = ephemeris.dayOfWeek(jd);
        String calName = type.name().charAt(0) + type.name().substring(1).toLowerCase();

        System.out.printf("Date:         %04d-%02d-%02d (%s)%n", year, month, day,
                DateUtils.dayOfWeekName(dow));
        System.out.printf("Calendar:     %s Solar%n", calName);
        System.out.printf("Solar Date:   %s %d, %d (%s)%n",
                type.monthName(sd.month()), sd.day(), sd.year(), type.eraName());
        System.out.printf("Rashi:        %s%n",
                sd.rashi() >= 1 && sd.rashi() <= 12 ? Solar.RASHI_NAMES[sd.rashi()] : "???");
    }
}
