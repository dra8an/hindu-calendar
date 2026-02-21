package com.hindu.calendar.model;

public enum Paksha {
    SHUKLA(0),   // Bright half (waxing, tithis 1-15)
    KRISHNA(1);  // Dark half (waning, tithis 1-15)

    private final int value;

    Paksha(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }
}
