/// Validation tests against drikpanchang.com
/// 186 dates verified: tithi, masa, adhika, saka all 100% match.

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{tithi, masa};

struct RefData {
    y: i32, m: i32, d: i32,
    tithi: i32,
    masa: i32,
    adhika: bool,
    saka: i32,
}

const REF_DATA: &[RefData] = &[
    // ---- Spot-checked against drikpanchang.com (30 dates, 1950-2045) ----
    RefData { y: 1950, m: 1, d: 1, tithi: 12, masa: 10, adhika: false, saka: 1871 },
    RefData { y: 1975, m: 7, d: 15, tithi: 7, masa: 4, adhika: false, saka: 1897 },
    RefData { y: 1990, m: 3, d: 10, tithi: 14, masa: 12, adhika: false, saka: 1911 },
    RefData { y: 2000, m: 1, d: 1, tithi: 25, masa: 9, adhika: false, saka: 1921 },
    RefData { y: 2000, m: 4, d: 15, tithi: 12, masa: 1, adhika: false, saka: 1922 },
    RefData { y: 2001, m: 6, d: 21, tithi: 30, masa: 3, adhika: false, saka: 1923 },
    RefData { y: 2003, m: 10, d: 10, tithi: 15, masa: 7, adhika: false, saka: 1925 },
    RefData { y: 2004, m: 7, d: 18, tithi: 1, masa: 5, adhika: true, saka: 1926 },
    RefData { y: 2004, m: 8, d: 1, tithi: 16, masa: 5, adhika: true, saka: 1926 },
    RefData { y: 2006, m: 10, d: 22, tithi: 30, masa: 7, adhika: false, saka: 1928 },
    RefData { y: 2007, m: 3, d: 15, tithi: 26, masa: 12, adhika: false, saka: 1928 },
    RefData { y: 2007, m: 6, d: 20, tithi: 6, masa: 3, adhika: false, saka: 1929 },
    RefData { y: 2010, m: 8, d: 12, tithi: 3, masa: 5, adhika: false, saka: 1932 },
    RefData { y: 2010, m: 9, d: 4, tithi: 25, masa: 5, adhika: false, saka: 1932 },

    // ---- Original hand-verified dates ----
    RefData { y: 2012, m: 1, d: 1, tithi: 8, masa: 10, adhika: false, saka: 1933 },
    RefData { y: 2012, m: 3, d: 23, tithi: 1, masa: 1, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 5, d: 20, tithi: 30, masa: 2, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 8, d: 17, tithi: 30, masa: 5, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 8, d: 18, tithi: 1, masa: 6, adhika: true, saka: 1934 },
    RefData { y: 2012, m: 9, d: 16, tithi: 30, masa: 6, adhika: true, saka: 1934 },
    RefData { y: 2012, m: 9, d: 17, tithi: 2, masa: 6, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 11, d: 13, tithi: 29, masa: 7, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 12, d: 13, tithi: 30, masa: 8, adhika: false, saka: 1934 },
    RefData { y: 2012, m: 12, d: 14, tithi: 1, masa: 9, adhika: false, saka: 1934 },
    RefData { y: 2013, m: 1, d: 18, tithi: 7, masa: 10, adhika: false, saka: 1934 },
    RefData { y: 2013, m: 4, d: 10, tithi: 30, masa: 12, adhika: false, saka: 1934 },
    RefData { y: 2013, m: 4, d: 11, tithi: 1, masa: 1, adhika: false, saka: 1935 },

    RefData { y: 2015, m: 7, d: 16, tithi: 30, masa: 4, adhika: true, saka: 1937 },
    RefData { y: 2015, m: 7, d: 17, tithi: 1, masa: 4, adhika: false, saka: 1937 },
    RefData { y: 2015, m: 7, d: 18, tithi: 2, masa: 4, adhika: false, saka: 1937 },
    RefData { y: 2015, m: 8, d: 14, tithi: 30, masa: 4, adhika: false, saka: 1937 },
    RefData { y: 2015, m: 8, d: 15, tithi: 1, masa: 5, adhika: false, saka: 1937 },

    RefData { y: 2018, m: 1, d: 1, tithi: 14, masa: 10, adhika: false, saka: 1939 },
    RefData { y: 2018, m: 5, d: 17, tithi: 2, masa: 3, adhika: true, saka: 1940 },

    RefData { y: 2019, m: 6, d: 15, tithi: 13, masa: 3, adhika: false, saka: 1941 },

    RefData { y: 2020, m: 1, d: 1, tithi: 6, masa: 10, adhika: false, saka: 1941 },
    RefData { y: 2020, m: 3, d: 25, tithi: 1, masa: 1, adhika: false, saka: 1942 },
    RefData { y: 2020, m: 9, d: 18, tithi: 1, masa: 7, adhika: true, saka: 1942 },
    RefData { y: 2020, m: 11, d: 12, tithi: 27, masa: 7, adhika: false, saka: 1942 },

    RefData { y: 2021, m: 8, d: 13, tithi: 5, masa: 5, adhika: false, saka: 1943 },
    RefData { y: 2022, m: 4, d: 30, tithi: 30, masa: 1, adhika: false, saka: 1944 },

    RefData { y: 2023, m: 7, d: 18, tithi: 1, masa: 5, adhika: true, saka: 1945 },
    RefData { y: 2023, m: 8, d: 18, tithi: 2, masa: 5, adhika: false, saka: 1945 },

    RefData { y: 2024, m: 9, d: 1, tithi: 29, masa: 5, adhika: false, saka: 1946 },

    RefData { y: 2025, m: 1, d: 1, tithi: 2, masa: 10, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 1, d: 13, tithi: 15, masa: 10, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 1, d: 29, tithi: 30, masa: 10, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 1, d: 30, tithi: 1, masa: 11, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 3, d: 29, tithi: 30, masa: 12, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 3, d: 30, tithi: 1, masa: 1, adhika: false, saka: 1947 },

    RefData { y: 2026, m: 2, d: 14, tithi: 27, masa: 11, adhika: false, saka: 1947 },
    RefData { y: 2027, m: 12, d: 21, tithi: 24, masa: 9, adhika: false, saka: 1949 },
    RefData { y: 2030, m: 5, d: 8, tithi: 5, masa: 2, adhika: false, saka: 1952 },
    RefData { y: 2045, m: 11, d: 15, tithi: 7, masa: 8, adhika: false, saka: 1967 },

    // ---- Batch-verified adhika tithi dates (1900-1919), 21 dates ----
    RefData { y: 1900, m: 1, d: 20, tithi: 19, masa: 10, adhika: false, saka: 1821 },
    RefData { y: 1901, m: 1, d: 10, tithi: 20, masa: 10, adhika: false, saka: 1822 },
    RefData { y: 1901, m: 12, d: 11, tithi: 30, masa: 8, adhika: false, saka: 1823 },
    RefData { y: 1903, m: 1, d: 1, tithi: 2, masa: 10, adhika: false, saka: 1824 },
    RefData { y: 1903, m: 10, d: 31, tithi: 10, masa: 8, adhika: false, saka: 1825 },
    RefData { y: 1904, m: 10, d: 20, tithi: 11, masa: 7, adhika: false, saka: 1826 },
    RefData { y: 1905, m: 11, d: 11, tithi: 14, masa: 8, adhika: false, saka: 1827 },
    RefData { y: 1906, m: 9, d: 7, tithi: 19, masa: 6, adhika: false, saka: 1828 },
    RefData { y: 1907, m: 8, d: 7, tithi: 28, masa: 4, adhika: false, saka: 1829 },
    RefData { y: 1908, m: 7, d: 3, tithi: 4, masa: 4, adhika: false, saka: 1830 },
    RefData { y: 1909, m: 6, d: 23, tithi: 5, masa: 4, adhika: false, saka: 1831 },
    RefData { y: 1910, m: 5, d: 22, tithi: 13, masa: 2, adhika: false, saka: 1832 },
    RefData { y: 1911, m: 5, d: 10, tithi: 12, masa: 2, adhika: false, saka: 1833 },
    RefData { y: 1912, m: 5, d: 11, tithi: 24, masa: 2, adhika: false, saka: 1834 },
    RefData { y: 1913, m: 3, d: 30, tithi: 23, masa: 12, adhika: false, saka: 1834 },
    RefData { y: 1914, m: 6, d: 15, tithi: 22, masa: 3, adhika: false, saka: 1836 },
    RefData { y: 1915, m: 6, d: 16, tithi: 3, masa: 3, adhika: false, saka: 1837 },
    RefData { y: 1916, m: 6, d: 3, tithi: 2, masa: 3, adhika: false, saka: 1838 },
    RefData { y: 1917, m: 6, d: 26, tithi: 6, masa: 4, adhika: false, saka: 1839 },
    RefData { y: 1918, m: 4, d: 23, tithi: 12, masa: 1, adhika: false, saka: 1840 },
    RefData { y: 1919, m: 4, d: 11, tithi: 11, masa: 1, adhika: false, saka: 1841 },

    // ---- Batch-verified adhika tithi dates (1990-2020), 31 dates ----
    RefData { y: 1990, m: 11, d: 13, tithi: 26, masa: 8, adhika: false, saka: 1912 },
    RefData { y: 1991, m: 11, d: 15, tithi: 8, masa: 8, adhika: false, saka: 1913 },
    RefData { y: 1992, m: 11, d: 5, tithi: 10, masa: 8, adhika: false, saka: 1914 },
    RefData { y: 1993, m: 8, d: 31, tithi: 14, masa: 6, adhika: true, saka: 1915 },
    RefData { y: 1994, m: 9, d: 23, tithi: 18, masa: 6, adhika: false, saka: 1916 },
    RefData { y: 1995, m: 8, d: 23, tithi: 27, masa: 5, adhika: false, saka: 1917 },
    RefData { y: 1996, m: 8, d: 11, tithi: 27, masa: 4, adhika: false, saka: 1918 },
    RefData { y: 1997, m: 9, d: 3, tithi: 1, masa: 6, adhika: false, saka: 1919 },
    RefData { y: 1998, m: 8, d: 1, tithi: 8, masa: 5, adhika: false, saka: 1920 },
    RefData { y: 1999, m: 9, d: 15, tithi: 5, masa: 6, adhika: false, saka: 1921 },
    RefData { y: 2000, m: 10, d: 7, tithi: 9, masa: 7, adhika: false, saka: 1922 },
    RefData { y: 2001, m: 8, d: 2, tithi: 13, masa: 5, adhika: false, saka: 1923 },
    RefData { y: 2002, m: 8, d: 3, tithi: 24, masa: 4, adhika: false, saka: 1924 },
    RefData { y: 2003, m: 5, d: 30, tithi: 29, masa: 2, adhika: false, saka: 1925 },
    RefData { y: 2004, m: 6, d: 20, tithi: 2, masa: 4, adhika: false, saka: 1926 },
    RefData { y: 2005, m: 6, d: 9, tithi: 2, masa: 3, adhika: false, saka: 1927 },
    RefData { y: 2006, m: 4, d: 6, tithi: 8, masa: 1, adhika: false, saka: 1928 },
    RefData { y: 2007, m: 6, d: 1, tithi: 15, masa: 3, adhika: true, saka: 1929 },
    RefData { y: 2008, m: 4, d: 28, tithi: 22, masa: 1, adhika: false, saka: 1930 },
    RefData { y: 2009, m: 3, d: 17, tithi: 21, masa: 12, adhika: false, saka: 1930 },
    RefData { y: 2010, m: 3, d: 20, tithi: 4, masa: 1, adhika: false, saka: 1932 },
    RefData { y: 2011, m: 1, d: 14, tithi: 9, masa: 10, adhika: false, saka: 1932 },
    RefData { y: 2012, m: 1, d: 5, tithi: 11, masa: 10, adhika: false, saka: 1933 },
    RefData { y: 2013, m: 1, d: 26, tithi: 14, masa: 10, adhika: false, saka: 1934 },
    RefData { y: 2013, m: 12, d: 26, tithi: 23, masa: 9, adhika: false, saka: 1935 },
    RefData { y: 2015, m: 2, d: 6, tithi: 17, masa: 11, adhika: false, saka: 1936 },
    RefData { y: 2016, m: 2, d: 27, tithi: 19, masa: 11, adhika: false, saka: 1937 },
    RefData { y: 2017, m: 2, d: 16, tithi: 20, masa: 11, adhika: false, saka: 1938 },
    RefData { y: 2018, m: 6, d: 4, tithi: 20, masa: 3, adhika: true, saka: 1940 },
    RefData { y: 2019, m: 4, d: 2, tithi: 27, masa: 12, adhika: false, saka: 1940 },
    RefData { y: 2020, m: 3, d: 21, tithi: 27, masa: 12, adhika: false, saka: 1941 },

    // ---- Batch-verified kshaya tithi dates (1900-1919), 21 dates ----
    RefData { y: 1900, m: 1, d: 6, tithi: 6, masa: 10, adhika: false, saka: 1821 },
    RefData { y: 1901, m: 1, d: 4, tithi: 15, masa: 10, adhika: false, saka: 1822 },
    RefData { y: 1901, m: 12, d: 22, tithi: 12, masa: 9, adhika: false, saka: 1823 },
    RefData { y: 1903, m: 1, d: 11, tithi: 13, masa: 10, adhika: false, saka: 1824 },
    RefData { y: 1903, m: 12, d: 11, tithi: 23, masa: 9, adhika: false, saka: 1825 },
    RefData { y: 1904, m: 11, d: 27, tithi: 20, masa: 8, adhika: false, saka: 1826 },
    RefData { y: 1905, m: 12, d: 19, tithi: 23, masa: 9, adhika: false, saka: 1827 },
    RefData { y: 1906, m: 10, d: 26, tithi: 10, masa: 8, adhika: false, saka: 1828 },
    RefData { y: 1907, m: 11, d: 5, tithi: 30, masa: 7, adhika: false, saka: 1829 },
    RefData { y: 1908, m: 9, d: 14, tithi: 20, masa: 6, adhika: false, saka: 1830 },
    RefData { y: 1909, m: 9, d: 27, tithi: 13, masa: 6, adhika: false, saka: 1831 },
    RefData { y: 1910, m: 10, d: 16, tithi: 13, masa: 7, adhika: false, saka: 1832 },
    RefData { y: 1911, m: 8, d: 26, tithi: 3, masa: 6, adhika: false, saka: 1833 },
    RefData { y: 1912, m: 9, d: 6, tithi: 25, masa: 5, adhika: false, saka: 1834 },
    RefData { y: 1913, m: 10, d: 2, tithi: 3, masa: 7, adhika: false, saka: 1835 },
    RefData { y: 1914, m: 10, d: 21, tithi: 3, masa: 8, adhika: false, saka: 1836 },
    RefData { y: 1915, m: 10, d: 17, tithi: 10, masa: 7, adhika: false, saka: 1837 },
    RefData { y: 1916, m: 11, d: 6, tithi: 12, masa: 8, adhika: false, saka: 1838 },
    RefData { y: 1917, m: 10, d: 25, tithi: 10, masa: 7, adhika: false, saka: 1839 },
    RefData { y: 1918, m: 9, d: 24, tithi: 20, masa: 6, adhika: false, saka: 1840 },
    RefData { y: 1919, m: 10, d: 13, tithi: 20, masa: 7, adhika: false, saka: 1841 },

    // ---- Batch-verified kshaya tithi dates (1992-2022), 31 dates ----
    RefData { y: 1992, m: 8, d: 29, tithi: 2, masa: 6, adhika: false, saka: 1914 },
    RefData { y: 1993, m: 7, d: 24, tithi: 6, masa: 5, adhika: false, saka: 1915 },
    RefData { y: 1994, m: 7, d: 22, tithi: 15, masa: 4, adhika: false, saka: 1916 },
    RefData { y: 1995, m: 7, d: 16, tithi: 20, masa: 4, adhika: false, saka: 1917 },
    RefData { y: 1996, m: 7, d: 28, tithi: 13, masa: 4, adhika: false, saka: 1918 },
    RefData { y: 1997, m: 8, d: 15, tithi: 12, masa: 5, adhika: false, saka: 1919 },
    RefData { y: 1998, m: 7, d: 18, tithi: 25, masa: 4, adhika: false, saka: 1920 },
    RefData { y: 1999, m: 8, d: 7, tithi: 26, masa: 4, adhika: false, saka: 1921 },
    RefData { y: 2000, m: 8, d: 26, tithi: 27, masa: 5, adhika: false, saka: 1922 },
    RefData { y: 2001, m: 7, d: 27, tithi: 8, masa: 5, adhika: false, saka: 1923 },
    RefData { y: 2002, m: 8, d: 7, tithi: 29, masa: 4, adhika: false, saka: 1924 },
    RefData { y: 2003, m: 7, d: 13, tithi: 15, masa: 4, adhika: false, saka: 1925 },
    RefData { y: 2004, m: 7, d: 6, tithi: 20, masa: 4, adhika: false, saka: 1926 },
    RefData { y: 2005, m: 7, d: 19, tithi: 13, masa: 4, adhika: false, saka: 1927 },
    RefData { y: 2006, m: 6, d: 20, tithi: 25, masa: 3, adhika: false, saka: 1928 },
    RefData { y: 2007, m: 7, d: 10, tithi: 26, masa: 3, adhika: false, saka: 1929 },
    RefData { y: 2008, m: 6, d: 27, tithi: 24, masa: 3, adhika: false, saka: 1930 },
    RefData { y: 2009, m: 6, d: 22, tithi: 30, masa: 3, adhika: false, saka: 1931 },
    RefData { y: 2010, m: 5, d: 27, tithi: 15, masa: 2, adhika: false, saka: 1932 },
    RefData { y: 2011, m: 4, d: 19, tithi: 17, masa: 1, adhika: false, saka: 1933 },
    RefData { y: 2012, m: 5, d: 7, tithi: 17, masa: 2, adhika: false, saka: 1934 },
    RefData { y: 2013, m: 5, d: 27, tithi: 18, masa: 2, adhika: false, saka: 1935 },
    RefData { y: 2014, m: 4, d: 21, tithi: 22, masa: 1, adhika: false, saka: 1936 },
    RefData { y: 2015, m: 6, d: 12, tithi: 26, masa: 3, adhika: false, saka: 1937 },
    RefData { y: 2016, m: 6, d: 7, tithi: 3, masa: 3, adhika: false, saka: 1938 },
    RefData { y: 2017, m: 7, d: 20, tithi: 27, masa: 4, adhika: false, saka: 1939 },
    RefData { y: 2018, m: 8, d: 14, tithi: 4, masa: 5, adhika: false, saka: 1940 },
    RefData { y: 2019, m: 8, d: 2, tithi: 2, masa: 5, adhika: false, saka: 1941 },
    RefData { y: 2020, m: 8, d: 20, tithi: 2, masa: 6, adhika: false, saka: 1942 },
    RefData { y: 2021, m: 8, d: 17, tithi: 10, masa: 5, adhika: false, saka: 1943 },
    RefData { y: 2022, m: 8, d: 13, tithi: 17, masa: 5, adhika: false, saka: 1944 },

    // ---- Batch-verified kshaya tithi dates (2023-2050), 28 dates ----
    RefData { y: 2023, m: 9, d: 1, tithi: 17, masa: 5, adhika: false, saka: 1945 },
    RefData { y: 2024, m: 7, d: 25, tithi: 20, masa: 4, adhika: false, saka: 1946 },
    RefData { y: 2025, m: 8, d: 14, tithi: 21, masa: 5, adhika: false, saka: 1947 },
    RefData { y: 2026, m: 8, d: 11, tithi: 29, masa: 4, adhika: false, saka: 1948 },
    RefData { y: 2027, m: 8, d: 5, tithi: 4, masa: 5, adhika: false, saka: 1949 },
    RefData { y: 2028, m: 9, d: 18, tithi: 30, masa: 6, adhika: false, saka: 1950 },
    RefData { y: 2029, m: 10, d: 7, tithi: 30, masa: 6, adhika: false, saka: 1951 },
    RefData { y: 2030, m: 10, d: 26, tithi: 30, masa: 7, adhika: false, saka: 1952 },
    RefData { y: 2031, m: 10, d: 22, tithi: 7, masa: 7, adhika: false, saka: 1953 },
    RefData { y: 2032, m: 9, d: 22, tithi: 19, masa: 6, adhika: false, saka: 1954 },
    RefData { y: 2033, m: 10, d: 5, tithi: 12, masa: 7, adhika: false, saka: 1955 },
    RefData { y: 2034, m: 9, d: 29, tithi: 17, masa: 6, adhika: false, saka: 1956 },
    RefData { y: 2035, m: 9, d: 3, tithi: 2, masa: 6, adhika: false, saka: 1957 },
    RefData { y: 2036, m: 9, d: 21, tithi: 2, masa: 7, adhika: false, saka: 1958 },
    RefData { y: 2037, m: 10, d: 10, tithi: 2, masa: 7, adhika: false, saka: 1959 },
    RefData { y: 2038, m: 10, d: 4, tithi: 7, masa: 7, adhika: false, saka: 1960 },
    RefData { y: 2039, m: 10, d: 15, tithi: 28, masa: 7, adhika: true, saka: 1961 },
    RefData { y: 2040, m: 9, d: 19, tithi: 14, masa: 6, adhika: false, saka: 1962 },
    RefData { y: 2041, m: 9, d: 13, tithi: 19, masa: 6, adhika: false, saka: 1963 },
    RefData { y: 2042, m: 9, d: 25, tithi: 11, masa: 6, adhika: false, saka: 1964 },
    RefData { y: 2043, m: 8, d: 28, tithi: 24, masa: 5, adhika: false, saka: 1965 },
    RefData { y: 2044, m: 8, d: 24, tithi: 2, masa: 6, adhika: false, saka: 1966 },
    RefData { y: 2045, m: 8, d: 12, tithi: 30, masa: 4, adhika: false, saka: 1967 },
    RefData { y: 2046, m: 8, d: 31, tithi: 30, masa: 5, adhika: false, saka: 1968 },
    RefData { y: 2047, m: 8, d: 25, tithi: 5, masa: 6, adhika: false, saka: 1969 },
    RefData { y: 2048, m: 10, d: 15, tithi: 9, masa: 7, adhika: false, saka: 1970 },
    RefData { y: 2049, m: 10, d: 3, tithi: 7, masa: 7, adhika: false, saka: 1971 },
    RefData { y: 2050, m: 9, d: 4, tithi: 19, masa: 6, adhika: true, saka: 1972 },
];

#[test]
fn test_186_drikpanchang_dates() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;
    let mut passed = 0;
    let mut failed = 0;

    for (i, rd) in REF_DATA.iter().enumerate() {
        let ti = tithi::tithi_at_sunrise(&mut eph, rd.y, rd.m, rd.d, &delhi);
        let mi = masa::masa_for_date(&mut eph, rd.y, rd.m, rd.d, &delhi);

        if ti.tithi_num != rd.tithi {
            eprintln!("FAIL [{:03}] {:04}-{:02}-{:02} tithi: got {}, expected {}",
                i, rd.y, rd.m, rd.d, ti.tithi_num, rd.tithi);
            failed += 1;
        } else {
            passed += 1;
        }

        if mi.name.number() != rd.masa {
            eprintln!("FAIL [{:03}] {:04}-{:02}-{:02} masa: got {}, expected {}",
                i, rd.y, rd.m, rd.d, mi.name.number(), rd.masa);
            failed += 1;
        } else {
            passed += 1;
        }

        if mi.is_adhika != rd.adhika {
            eprintln!("FAIL [{:03}] {:04}-{:02}-{:02} adhika: got {}, expected {}",
                i, rd.y, rd.m, rd.d, mi.is_adhika, rd.adhika);
            failed += 1;
        } else {
            passed += 1;
        }

        if mi.year_saka != rd.saka {
            eprintln!("FAIL [{:03}] {:04}-{:02}-{:02} saka: got {}, expected {}",
                i, rd.y, rd.m, rd.d, mi.year_saka, rd.saka);
            failed += 1;
        } else {
            passed += 1;
        }
    }

    eprintln!("\n=== Validation: {}/{} passed, {} failed ===", passed, passed + failed, failed);
    assert_eq!(failed, 0, "{} validation tests failed", failed);
}
