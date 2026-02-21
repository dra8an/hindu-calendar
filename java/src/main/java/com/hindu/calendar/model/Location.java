package com.hindu.calendar.model;

public record Location(double latitude, double longitude, double altitude, double utcOffset) {
    /** Default location: New Delhi */
    public static final Location NEW_DELHI = new Location(28.6139, 77.2090, 0.0, 5.5);
}
