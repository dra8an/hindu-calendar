package com.hindu.calendar.model;

public enum MasaName {
    CHAITRA(1, "Chaitra"),
    VAISHAKHA(2, "Vaishakha"),
    JYESHTHA(3, "Jyeshtha"),
    ASHADHA(4, "Ashadha"),
    SHRAVANA(5, "Shravana"),
    BHADRAPADA(6, "Bhadrapada"),
    ASHVINA(7, "Ashvina"),
    KARTIKA(8, "Kartika"),
    MARGASHIRSHA(9, "Margashirsha"),
    PAUSHA(10, "Pausha"),
    MAGHA(11, "Magha"),
    PHALGUNA(12, "Phalguna");

    private final int number;
    private final String displayName;

    MasaName(int number, String displayName) {
        this.number = number;
        this.displayName = displayName;
    }

    public int number() {
        return number;
    }

    public String displayName() {
        return displayName;
    }

    public static MasaName fromNumber(int num) {
        for (MasaName m : values()) {
            if (m.number == num) return m;
        }
        throw new IllegalArgumentException("Invalid masa number: " + num);
    }
}
