package com.hindu.calendar.model;

public enum SolarCalendarType {
    TAMIL(1, 78, 79, "Saka", new String[]{
            "", "Chithirai", "Vaikaasi", "Aani", "Aadi", "Aavani", "Purattaasi",
            "Aippasi", "Karthikai", "Maargazhi", "Thai", "Maasi", "Panguni"
    }),
    BENGALI(1, 593, 594, "Bangabda", new String[]{
            "", "Boishakh", "Joishtho", "Asharh", "Srabon", "Bhadro", "Ashshin",
            "Kartik", "Ogrohaeon", "Poush", "Magh", "Falgun", "Choitro"
    }),
    ODIA(1, 78, 79, "Saka", new String[]{
            "", "Baisakha", "Jyeshtha", "Ashadha", "Shravana", "Bhadrapada", "Ashvina",
            "Kartika", "Margashirsha", "Pausha", "Magha", "Phalguna", "Chaitra"
    }),
    MALAYALAM(5, 824, 825, "Kollam", new String[]{
            "", "Chingam", "Kanni", "Thulam", "Vrishchikam", "Dhanu", "Makaram",
            "Kumbham", "Meenam", "Medam", "Edavam", "Mithunam", "Karkadakam"
    });

    private final int firstRashi;
    private final int gyOffsetOn;
    private final int gyOffsetBefore;
    private final String eraName;
    private final String[] months;

    SolarCalendarType(int firstRashi, int gyOffsetOn, int gyOffsetBefore,
                      String eraName, String[] months) {
        this.firstRashi = firstRashi;
        this.gyOffsetOn = gyOffsetOn;
        this.gyOffsetBefore = gyOffsetBefore;
        this.eraName = eraName;
        this.months = months;
    }

    public int firstRashi() { return firstRashi; }
    public int gyOffsetOn() { return gyOffsetOn; }
    public int gyOffsetBefore() { return gyOffsetBefore; }
    public String eraName() { return eraName; }

    public String monthName(int month) {
        if (month < 1 || month > 12) return "???";
        return months[month];
    }

    public static SolarCalendarType fromString(String name) {
        return switch (name.toLowerCase()) {
            case "tamil" -> TAMIL;
            case "bengali" -> BENGALI;
            case "odia" -> ODIA;
            case "malayalam" -> MALAYALAM;
            default -> throw new IllegalArgumentException("Unknown solar calendar: " + name);
        };
    }
}
