use crate::ephemeris::Ephemeris;
use crate::model::*;
use super::{tithi, masa};

fn days_in_month(year: i32, month: i32) -> i32 {
    const MDAYS: [i32; 13] = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    if month == 2 {
        if (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 {
            return 29;
        }
    }
    MDAYS[month as usize]
}

pub fn gregorian_to_hindu(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    day: i32,
    loc: &Location,
) -> HinduDate {
    let ti = tithi::tithi_at_sunrise(eph, year, month, day, loc);
    let mi = masa::masa_for_date(eph, year, month, day, loc);

    let is_adhika_tithi = if day > 1 {
        let ti_prev = tithi::tithi_at_sunrise(eph, year, month, day - 1, loc);
        ti.tithi_num == ti_prev.tithi_num
    } else {
        false
    };

    HinduDate {
        masa: mi.name,
        is_adhika_masa: mi.is_adhika,
        year_saka: mi.year_saka,
        year_vikram: mi.year_vikram,
        paksha: ti.paksha,
        tithi: ti.paksha_tithi,
        is_adhika_tithi,
    }
}

pub fn generate_month_panchang(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    loc: &Location,
) -> Vec<PanchangDay> {
    let ndays = days_in_month(year, month);
    let mut days = Vec::with_capacity(ndays as usize);

    for d in 1..=ndays {
        let jd = eph.gregorian_to_jd(year, month, d);
        let jd_sunrise = eph.sunrise_jd(jd, loc);
        let ti = tithi::tithi_at_sunrise(eph, year, month, d, loc);
        let hd = gregorian_to_hindu(eph, year, month, d, loc);

        days.push(PanchangDay {
            greg_year: year,
            greg_month: month,
            greg_day: d,
            jd_sunrise,
            hindu_date: hd,
            tithi: ti,
        });
    }

    days
}

/// Format JD as local time (h, m, s)
pub fn jd_to_local_time(jd_ut: f64, utc_offset: f64) -> (i32, i32, i32) {
    let local_jd = jd_ut + 0.5 + utc_offset / 24.0;
    let frac = local_jd - local_jd.floor();
    let hours = frac * 24.0;
    let mut h = hours as i32;
    let mut m = ((hours - h as f64) * 60.0) as i32;
    let mut s = (((hours - h as f64) * 60.0 - m as f64) * 60.0 + 0.5) as i32;
    if s == 60 { s = 0; m += 1; }
    if m == 60 { m = 0; h += 1; }
    (h, m, s)
}

pub fn print_month_panchang(eph: &Ephemeris, days: &[PanchangDay], utc_offset: f64) {
    if days.is_empty() { return; }

    println!("{:<12} {:<5} {:<10} {:<28} {}",
        "Date", "Day", "Sunrise", "Tithi", "Hindu Date");
    println!("{:<12} {:<5} {:<10} {:<28} {}",
        "----------", "---", "--------", "----------------------------",
        "----------------------------");

    for pd in days {
        let jd = eph.gregorian_to_jd(pd.greg_year, pd.greg_month, pd.greg_day);
        let dow = eph.day_of_week(jd);

        let (sh, sm, ss) = jd_to_local_time(pd.jd_sunrise, utc_offset);

        let paksha_str = match pd.tithi.paksha {
            Paksha::Shukla => "Shukla",
            Paksha::Krishna => "Krishna",
        };
        let pt = pd.tithi.paksha_tithi;
        let tithi_name = if pd.tithi.tithi_num == 30 {
            "Amavasya"
        } else if pd.tithi.tithi_num == 15 {
            "Purnima"
        } else {
            TITHI_NAMES[pt as usize]
        };

        let masa_str = MASA_NAMES[pd.hindu_date.masa.number() as usize];
        let adhika_prefix = if pd.hindu_date.is_adhika_masa { "Adhika " } else { "" };

        let pk_char = if pd.tithi.paksha == Paksha::Shukla { "S" } else { "K" };

        println!("{:04}-{:02}-{:02}   {:<5} {:02}:{:02}:{:02}   {:<6} {:<13} ({}-{})   {}{} {} {}, Saka {}",
            pd.greg_year, pd.greg_month, pd.greg_day,
            DOW_SHORT[dow as usize],
            sh, sm, ss,
            paksha_str, tithi_name, pk_char, pt,
            adhika_prefix, masa_str, paksha_str, pt,
            pd.hindu_date.year_saka);
    }
}

pub fn print_day_panchang(eph: &Ephemeris, day: &PanchangDay, utc_offset: f64) {
    let jd = eph.gregorian_to_jd(day.greg_year, day.greg_month, day.greg_day);
    let dow = eph.day_of_week(jd);

    let (sh, sm, ss) = jd_to_local_time(day.jd_sunrise, utc_offset);

    let paksha_str = match day.tithi.paksha {
        Paksha::Shukla => "Shukla",
        Paksha::Krishna => "Krishna",
    };
    let pt = day.tithi.paksha_tithi;
    let tithi_name = if day.tithi.tithi_num == 30 {
        "Amavasya"
    } else if day.tithi.tithi_num == 15 {
        "Purnima"
    } else {
        TITHI_NAMES[pt as usize]
    };
    let masa_str = MASA_NAMES[day.hindu_date.masa.number() as usize];
    let adhika_prefix = if day.hindu_date.is_adhika_masa { "Adhika " } else { "" };
    let pk_char = if day.tithi.paksha == Paksha::Shukla { "S" } else { "K" };

    println!("Date:       {:04}-{:02}-{:02} ({})",
        day.greg_year, day.greg_month, day.greg_day,
        DOW_NAMES[dow as usize]);
    println!("Sunrise:    {:02}:{:02}:{:02} IST", sh, sm, ss);
    println!("Tithi:      {} {} ({}-{})", paksha_str, tithi_name, pk_char, pt);
    println!("Hindu Date: {}{} {} {}, Saka {} (Vikram {})",
        adhika_prefix, masa_str, paksha_str, pt,
        day.hindu_date.year_saka, day.hindu_date.year_vikram);
    if day.tithi.is_kshaya {
        println!("Note:       Kshaya tithi (next tithi is skipped)");
    }
    if day.hindu_date.is_adhika_tithi {
        println!("Note:       Adhika tithi (same tithi as previous day)");
    }
}
