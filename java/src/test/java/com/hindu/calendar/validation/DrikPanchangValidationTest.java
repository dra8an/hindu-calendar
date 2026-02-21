package com.hindu.calendar.validation;

import com.hindu.calendar.core.Masa;
import com.hindu.calendar.core.Tithi;
import com.hindu.calendar.ephemeris.Ephemeris;
import com.hindu.calendar.model.Location;
import com.hindu.calendar.model.MasaInfo;
import com.hindu.calendar.model.TithiInfo;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * 186 dates validated against drikpanchang.com.
 * Each date checks tithi, masa, adhika flag, and Saka year (4 assertions = 744 total).
 * Ported verbatim from tests/test_validation.c ref_data[].
 */
class DrikPanchangValidationTest {

    private static Tithi tithi;
    private static Masa masa;
    private static final Location LOC = Location.NEW_DELHI;

    @BeforeAll
    static void setUp() {
        Ephemeris eph = new Ephemeris();
        tithi = new Tithi(eph);
        masa = new Masa(eph, tithi);
    }

    private void check(int y, int m, int d, int expTithi, int expMasa, int expAdhika, int expSaka) {
        TithiInfo ti = tithi.tithiAtSunrise(y, m, d, LOC);
        MasaInfo mi = masa.masaForDate(y, m, d, LOC);
        String label = y + "-" + m + "-" + d;
        assertEquals(expTithi, ti.tithiNum(), "tithi " + label);
        assertEquals(expMasa, mi.name().number(), "masa " + label);
        assertEquals(expAdhika != 0, mi.isAdhika(), "adhika " + label);
        assertEquals(expSaka, mi.yearSaka(), "saka " + label);
    }

    // ===== Spot-checked against drikpanchang.com (54 dates, 1950-2045) =====

    @Test void test_1950_01_01() { check(1950,1,1, 12,10,0,1871); }
    @Test void test_1975_07_15() { check(1975,7,15, 7,4,0,1897); }
    @Test void test_1990_03_10() { check(1990,3,10, 14,12,0,1911); }
    @Test void test_2000_01_01() { check(2000,1,1, 25,9,0,1921); }
    @Test void test_2000_04_15() { check(2000,4,15, 12,1,0,1922); }
    @Test void test_2001_06_21() { check(2001,6,21, 30,3,0,1923); }
    @Test void test_2003_10_10() { check(2003,10,10, 15,7,0,1925); }
    @Test void test_2004_07_18() { check(2004,7,18, 1,5,1,1926); }
    @Test void test_2004_08_01() { check(2004,8,1, 16,5,1,1926); }
    @Test void test_2006_10_22() { check(2006,10,22, 30,7,0,1928); }
    @Test void test_2007_03_15() { check(2007,3,15, 26,12,0,1928); }
    @Test void test_2007_06_20() { check(2007,6,20, 6,3,0,1929); }
    @Test void test_2010_08_12() { check(2010,8,12, 3,5,0,1932); }
    @Test void test_2010_09_04() { check(2010,9,4, 25,5,0,1932); }
    @Test void test_2012_01_01() { check(2012,1,1, 8,10,0,1933); }
    @Test void test_2012_03_23() { check(2012,3,23, 1,1,0,1934); }
    @Test void test_2012_05_20() { check(2012,5,20, 30,2,0,1934); }
    @Test void test_2012_08_17() { check(2012,8,17, 30,5,0,1934); }
    @Test void test_2012_08_18() { check(2012,8,18, 1,6,1,1934); }
    @Test void test_2012_09_16() { check(2012,9,16, 30,6,1,1934); }
    @Test void test_2012_09_17() { check(2012,9,17, 2,6,0,1934); }
    @Test void test_2012_11_13() { check(2012,11,13, 29,7,0,1934); }
    @Test void test_2012_12_13() { check(2012,12,13, 30,8,0,1934); }
    @Test void test_2012_12_14() { check(2012,12,14, 1,9,0,1934); }
    @Test void test_2013_01_18() { check(2013,1,18, 7,10,0,1934); }
    @Test void test_2013_04_10() { check(2013,4,10, 30,12,0,1934); }
    @Test void test_2013_04_11() { check(2013,4,11, 1,1,0,1935); }
    @Test void test_2015_07_16() { check(2015,7,16, 30,4,1,1937); }
    @Test void test_2015_07_17() { check(2015,7,17, 1,4,0,1937); }
    @Test void test_2015_07_18() { check(2015,7,18, 2,4,0,1937); }
    @Test void test_2015_08_14() { check(2015,8,14, 30,4,0,1937); }
    @Test void test_2015_08_15() { check(2015,8,15, 1,5,0,1937); }
    @Test void test_2018_01_01() { check(2018,1,1, 14,10,0,1939); }
    @Test void test_2018_05_17() { check(2018,5,17, 2,3,1,1940); }
    @Test void test_2019_06_15() { check(2019,6,15, 13,3,0,1941); }
    @Test void test_2020_01_01() { check(2020,1,1, 6,10,0,1941); }
    @Test void test_2020_03_25() { check(2020,3,25, 1,1,0,1942); }
    @Test void test_2020_09_18() { check(2020,9,18, 1,7,1,1942); }
    @Test void test_2020_11_12() { check(2020,11,12, 27,7,0,1942); }
    @Test void test_2021_08_13() { check(2021,8,13, 5,5,0,1943); }
    @Test void test_2022_04_30() { check(2022,4,30, 30,1,0,1944); }
    @Test void test_2023_07_18() { check(2023,7,18, 1,5,1,1945); }
    @Test void test_2023_08_18() { check(2023,8,18, 2,5,0,1945); }
    @Test void test_2024_09_01() { check(2024,9,1, 29,5,0,1946); }
    @Test void test_2025_01_01() { check(2025,1,1, 2,10,0,1946); }
    @Test void test_2025_01_13() { check(2025,1,13, 15,10,0,1946); }
    @Test void test_2025_01_29() { check(2025,1,29, 30,10,0,1946); }
    @Test void test_2025_01_30() { check(2025,1,30, 1,11,0,1946); }
    @Test void test_2025_03_29() { check(2025,3,29, 30,12,0,1946); }
    @Test void test_2025_03_30() { check(2025,3,30, 1,1,0,1947); }
    @Test void test_2026_02_14() { check(2026,2,14, 27,11,0,1947); }
    @Test void test_2027_12_21() { check(2027,12,21, 24,9,0,1949); }
    @Test void test_2030_05_08() { check(2030,5,8, 5,2,0,1952); }
    @Test void test_2045_11_15() { check(2045,11,15, 7,8,0,1967); }

    // ===== Batch-verified adhika tithi dates (1900-1919), 21 dates =====

    @Test void test_at_1900_01_20() { check(1900,1,20, 19,10,0,1821); }
    @Test void test_at_1901_01_10() { check(1901,1,10, 20,10,0,1822); }
    @Test void test_at_1901_12_11() { check(1901,12,11, 30,8,0,1823); }
    @Test void test_at_1903_01_01() { check(1903,1,1, 2,10,0,1824); }
    @Test void test_at_1903_10_31() { check(1903,10,31, 10,8,0,1825); }
    @Test void test_at_1904_10_20() { check(1904,10,20, 11,7,0,1826); }
    @Test void test_at_1905_11_11() { check(1905,11,11, 14,8,0,1827); }
    @Test void test_at_1906_09_07() { check(1906,9,7, 19,6,0,1828); }
    @Test void test_at_1907_08_07() { check(1907,8,7, 28,4,0,1829); }
    @Test void test_at_1908_07_03() { check(1908,7,3, 4,4,0,1830); }
    @Test void test_at_1909_06_23() { check(1909,6,23, 5,4,0,1831); }
    @Test void test_at_1910_05_22() { check(1910,5,22, 13,2,0,1832); }
    @Test void test_at_1911_05_10() { check(1911,5,10, 12,2,0,1833); }
    @Test void test_at_1912_05_11() { check(1912,5,11, 24,2,0,1834); }
    @Test void test_at_1913_03_30() { check(1913,3,30, 23,12,0,1834); }
    @Test void test_at_1914_06_15() { check(1914,6,15, 22,3,0,1836); }
    @Test void test_at_1915_06_16() { check(1915,6,16, 3,3,0,1837); }
    @Test void test_at_1916_06_03() { check(1916,6,3, 2,3,0,1838); }
    @Test void test_at_1917_06_26() { check(1917,6,26, 6,4,0,1839); }
    @Test void test_at_1918_04_23() { check(1918,4,23, 12,1,0,1840); }
    @Test void test_at_1919_04_11() { check(1919,4,11, 11,1,0,1841); }

    // ===== Batch-verified adhika tithi dates (1990-2020), 31 dates =====

    @Test void test_at_1990_11_13() { check(1990,11,13, 26,8,0,1912); }
    @Test void test_at_1991_11_15() { check(1991,11,15, 8,8,0,1913); }
    @Test void test_at_1992_11_05() { check(1992,11,5, 10,8,0,1914); }
    @Test void test_at_1993_08_31() { check(1993,8,31, 14,6,1,1915); }
    @Test void test_at_1994_09_23() { check(1994,9,23, 18,6,0,1916); }
    @Test void test_at_1995_08_23() { check(1995,8,23, 27,5,0,1917); }
    @Test void test_at_1996_08_11() { check(1996,8,11, 27,4,0,1918); }
    @Test void test_at_1997_09_03() { check(1997,9,3, 1,6,0,1919); }
    @Test void test_at_1998_08_01() { check(1998,8,1, 8,5,0,1920); }
    @Test void test_at_1999_09_15() { check(1999,9,15, 5,6,0,1921); }
    @Test void test_at_2000_10_07() { check(2000,10,7, 9,7,0,1922); }
    @Test void test_at_2001_08_02() { check(2001,8,2, 13,5,0,1923); }
    @Test void test_at_2002_08_03() { check(2002,8,3, 24,4,0,1924); }
    @Test void test_at_2003_05_30() { check(2003,5,30, 29,2,0,1925); }
    @Test void test_at_2004_06_20() { check(2004,6,20, 2,4,0,1926); }
    @Test void test_at_2005_06_09() { check(2005,6,9, 2,3,0,1927); }
    @Test void test_at_2006_04_06() { check(2006,4,6, 8,1,0,1928); }
    @Test void test_at_2007_06_01() { check(2007,6,1, 15,3,1,1929); }
    @Test void test_at_2008_04_28() { check(2008,4,28, 22,1,0,1930); }
    @Test void test_at_2009_03_17() { check(2009,3,17, 21,12,0,1930); }
    @Test void test_at_2010_03_20() { check(2010,3,20, 4,1,0,1932); }
    @Test void test_at_2011_01_14() { check(2011,1,14, 9,10,0,1932); }
    @Test void test_at_2012_01_05() { check(2012,1,5, 11,10,0,1933); }
    @Test void test_at_2013_01_26() { check(2013,1,26, 14,10,0,1934); }
    @Test void test_at_2013_12_26() { check(2013,12,26, 23,9,0,1935); }
    @Test void test_at_2015_02_06() { check(2015,2,6, 17,11,0,1936); }
    @Test void test_at_2016_02_27() { check(2016,2,27, 19,11,0,1937); }
    @Test void test_at_2017_02_16() { check(2017,2,16, 20,11,0,1938); }
    @Test void test_at_2018_06_04() { check(2018,6,4, 20,3,1,1940); }
    @Test void test_at_2019_04_02() { check(2019,4,2, 27,12,0,1940); }
    @Test void test_at_2020_03_21() { check(2020,3,21, 27,12,0,1941); }

    // ===== Batch-verified kshaya tithi dates (1900-1919), 21 dates =====

    @Test void test_kt_1900_01_06() { check(1900,1,6, 6,10,0,1821); }
    @Test void test_kt_1901_01_04() { check(1901,1,4, 15,10,0,1822); }
    @Test void test_kt_1901_12_22() { check(1901,12,22, 12,9,0,1823); }
    @Test void test_kt_1903_01_11() { check(1903,1,11, 13,10,0,1824); }
    @Test void test_kt_1903_12_11() { check(1903,12,11, 23,9,0,1825); }
    @Test void test_kt_1904_11_27() { check(1904,11,27, 20,8,0,1826); }
    @Test void test_kt_1905_12_19() { check(1905,12,19, 23,9,0,1827); }
    @Test void test_kt_1906_10_26() { check(1906,10,26, 10,8,0,1828); }
    @Test void test_kt_1907_11_05() { check(1907,11,5, 30,7,0,1829); }
    @Test void test_kt_1908_09_14() { check(1908,9,14, 20,6,0,1830); }
    @Test void test_kt_1909_09_27() { check(1909,9,27, 13,6,0,1831); }
    @Test void test_kt_1910_10_16() { check(1910,10,16, 13,7,0,1832); }
    @Test void test_kt_1911_08_26() { check(1911,8,26, 3,6,0,1833); }
    @Test void test_kt_1912_09_06() { check(1912,9,6, 25,5,0,1834); }
    @Test void test_kt_1913_10_02() { check(1913,10,2, 3,7,0,1835); }
    @Test void test_kt_1914_10_21() { check(1914,10,21, 3,8,0,1836); }
    @Test void test_kt_1915_10_17() { check(1915,10,17, 10,7,0,1837); }
    @Test void test_kt_1916_11_06() { check(1916,11,6, 12,8,0,1838); }
    @Test void test_kt_1917_10_25() { check(1917,10,25, 10,7,0,1839); }
    @Test void test_kt_1918_09_24() { check(1918,9,24, 20,6,0,1840); }
    @Test void test_kt_1919_10_13() { check(1919,10,13, 20,7,0,1841); }

    // ===== Batch-verified kshaya tithi dates (1992-2022), 31 dates =====

    @Test void test_kt_1992_08_29() { check(1992,8,29, 2,6,0,1914); }
    @Test void test_kt_1993_07_24() { check(1993,7,24, 6,5,0,1915); }
    @Test void test_kt_1994_07_22() { check(1994,7,22, 15,4,0,1916); }
    @Test void test_kt_1995_07_16() { check(1995,7,16, 20,4,0,1917); }
    @Test void test_kt_1996_07_28() { check(1996,7,28, 13,4,0,1918); }
    @Test void test_kt_1997_08_15() { check(1997,8,15, 12,5,0,1919); }
    @Test void test_kt_1998_07_18() { check(1998,7,18, 25,4,0,1920); }
    @Test void test_kt_1999_08_07() { check(1999,8,7, 26,4,0,1921); }
    @Test void test_kt_2000_08_26() { check(2000,8,26, 27,5,0,1922); }
    @Test void test_kt_2001_07_27() { check(2001,7,27, 8,5,0,1923); }
    @Test void test_kt_2002_08_07() { check(2002,8,7, 29,4,0,1924); }
    @Test void test_kt_2003_07_13() { check(2003,7,13, 15,4,0,1925); }
    @Test void test_kt_2004_07_06() { check(2004,7,6, 20,4,0,1926); }
    @Test void test_kt_2005_07_19() { check(2005,7,19, 13,4,0,1927); }
    @Test void test_kt_2006_06_20() { check(2006,6,20, 25,3,0,1928); }
    @Test void test_kt_2007_07_10() { check(2007,7,10, 26,3,0,1929); }
    @Test void test_kt_2008_06_27() { check(2008,6,27, 24,3,0,1930); }
    @Test void test_kt_2009_06_22() { check(2009,6,22, 30,3,0,1931); }
    @Test void test_kt_2010_05_27() { check(2010,5,27, 15,2,0,1932); }
    @Test void test_kt_2011_04_19() { check(2011,4,19, 17,1,0,1933); }
    @Test void test_kt_2012_05_07() { check(2012,5,7, 17,2,0,1934); }
    @Test void test_kt_2013_05_27() { check(2013,5,27, 18,2,0,1935); }
    @Test void test_kt_2014_04_21() { check(2014,4,21, 22,1,0,1936); }
    @Test void test_kt_2015_06_12() { check(2015,6,12, 26,3,0,1937); }
    @Test void test_kt_2016_06_07() { check(2016,6,7, 3,3,0,1938); }
    @Test void test_kt_2017_07_20() { check(2017,7,20, 27,4,0,1939); }
    @Test void test_kt_2018_08_14() { check(2018,8,14, 4,5,0,1940); }
    @Test void test_kt_2019_08_02() { check(2019,8,2, 2,5,0,1941); }
    @Test void test_kt_2020_08_20() { check(2020,8,20, 2,6,0,1942); }
    @Test void test_kt_2021_08_17() { check(2021,8,17, 10,5,0,1943); }
    @Test void test_kt_2022_08_13() { check(2022,8,13, 17,5,0,1944); }

    // ===== Batch-verified kshaya tithi dates (2023-2050), 28 dates =====

    @Test void test_kt_2023_09_01() { check(2023,9,1, 17,5,0,1945); }
    @Test void test_kt_2024_07_25() { check(2024,7,25, 20,4,0,1946); }
    @Test void test_kt_2025_08_14() { check(2025,8,14, 21,5,0,1947); }
    @Test void test_kt_2026_08_11() { check(2026,8,11, 29,4,0,1948); }
    @Test void test_kt_2027_08_05() { check(2027,8,5, 4,5,0,1949); }
    @Test void test_kt_2028_09_18() { check(2028,9,18, 30,6,0,1950); }
    @Test void test_kt_2029_10_07() { check(2029,10,7, 30,6,0,1951); }
    @Test void test_kt_2030_10_26() { check(2030,10,26, 30,7,0,1952); }
    @Test void test_kt_2031_10_22() { check(2031,10,22, 7,7,0,1953); }
    @Test void test_kt_2032_09_22() { check(2032,9,22, 19,6,0,1954); }
    @Test void test_kt_2033_10_05() { check(2033,10,5, 12,7,0,1955); }
    @Test void test_kt_2034_09_29() { check(2034,9,29, 17,6,0,1956); }
    @Test void test_kt_2035_09_03() { check(2035,9,3, 2,6,0,1957); }
    @Test void test_kt_2036_09_21() { check(2036,9,21, 2,7,0,1958); }
    @Test void test_kt_2037_10_10() { check(2037,10,10, 2,7,0,1959); }
    @Test void test_kt_2038_10_04() { check(2038,10,4, 7,7,0,1960); }
    @Test void test_kt_2039_10_15() { check(2039,10,15, 28,7,1,1961); }
    @Test void test_kt_2040_09_19() { check(2040,9,19, 14,6,0,1962); }
    @Test void test_kt_2041_09_13() { check(2041,9,13, 19,6,0,1963); }
    @Test void test_kt_2042_09_25() { check(2042,9,25, 11,6,0,1964); }
    @Test void test_kt_2043_08_28() { check(2043,8,28, 24,5,0,1965); }
    @Test void test_kt_2044_08_24() { check(2044,8,24, 2,6,0,1966); }
    @Test void test_kt_2045_08_12() { check(2045,8,12, 30,4,0,1967); }
    @Test void test_kt_2046_08_31() { check(2046,8,31, 30,5,0,1968); }
    @Test void test_kt_2047_08_25() { check(2047,8,25, 5,6,0,1969); }
    @Test void test_kt_2048_10_15() { check(2048,10,15, 9,7,0,1970); }
    @Test void test_kt_2049_10_03() { check(2049,10,3, 7,7,0,1971); }
    @Test void test_kt_2050_09_04() { check(2050,9,4, 19,6,1,1972); }
}
