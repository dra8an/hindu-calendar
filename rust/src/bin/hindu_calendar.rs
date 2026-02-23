use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{panchang, solar};

fn print_usage(prog: &str) {
    eprintln!(
        "Usage: {} [options]\n\
         \x20 -y YEAR      Gregorian year (default: current)\n\
         \x20 -m MONTH     Gregorian month 1-12 (default: current)\n\
         \x20 -d DAY       Specific day (if omitted, shows full month)\n\
         \x20 -s TYPE      Solar calendar: tamil, bengali, odia, malayalam\n\
         \x20              (if omitted, shows lunisolar panchang)\n\
         \x20 -l LAT,LON   Location (default: New Delhi 28.6139,77.2090)\n\
         \x20 -u OFFSET    UTC offset in hours (default: 5.5)\n\
         \x20 -h           Show this help",
        prog
    );
}

fn days_in_greg_month(year: i32, month: i32) -> i32 {
    const MDAYS: [i32; 13] = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    if month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        29
    } else {
        MDAYS[month as usize]
    }
}

fn parse_solar_type(s: &str) -> Option<SolarCalendarType> {
    match s {
        "tamil" => Some(SolarCalendarType::Tamil),
        "bengali" => Some(SolarCalendarType::Bengali),
        "odia" => Some(SolarCalendarType::Odia),
        "malayalam" => Some(SolarCalendarType::Malayalam),
        _ => None,
    }
}

fn print_solar_month(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    loc: &Location,
    cal_type: SolarCalendarType,
) {
    let ndays = days_in_greg_month(year, month);

    let sd1 = solar::gregorian_to_solar(eph, year, month, 1, loc, cal_type);
    let era = solar::solar_era_name(cal_type);
    let mname = solar::solar_month_name(sd1.month, cal_type);
    let cal_name = match cal_type {
        SolarCalendarType::Tamil => "Tamil",
        SolarCalendarType::Bengali => "Bengali",
        SolarCalendarType::Odia => "Odia",
        SolarCalendarType::Malayalam => "Malayalam",
    };
    println!("{} Solar Calendar — {} {} ({})", cal_name, mname, sd1.year, era);
    println!("Gregorian {:04}-{:02}\n", year, month);

    println!("{:<12} {:<5} {:<20} {}", "Date", "Day", "Solar Date", "");
    println!("{:<12} {:<5} {:<20}", "----------", "---", "--------------------");

    for d in 1..=ndays {
        let sd = solar::gregorian_to_solar(eph, year, month, d, loc, cal_type);
        let jd = eph.gregorian_to_jd(year, month, d);
        let dow = eph.day_of_week(jd);

        println!("{:04}-{:02}-{:02}   {:<5} {} {}, {}",
            year, month, d,
            DOW_SHORT[dow as usize],
            solar::solar_month_name(sd.month, cal_type),
            sd.day, sd.year);
    }
}

fn print_solar_day(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    day: i32,
    loc: &Location,
    cal_type: SolarCalendarType,
) {
    let sd = solar::gregorian_to_solar(eph, year, month, day, loc, cal_type);
    let jd = eph.gregorian_to_jd(year, month, day);
    let dow = eph.day_of_week(jd);
    let era = solar::solar_era_name(cal_type);
    let cal_name = match cal_type {
        SolarCalendarType::Tamil => "Tamil",
        SolarCalendarType::Bengali => "Bengali",
        SolarCalendarType::Odia => "Odia",
        SolarCalendarType::Malayalam => "Malayalam",
    };

    println!("Date:         {:04}-{:02}-{:02} ({})", year, month, day, DOW_NAMES[dow as usize]);
    println!("Calendar:     {} Solar", cal_name);
    println!("Solar Date:   {} {}, {} ({})",
        solar::solar_month_name(sd.month, cal_type), sd.day, sd.year, era);
    if sd.rashi >= 1 && sd.rashi <= 12 {
        println!("Rashi:        {}", RASHI_NAMES[sd.rashi as usize]);
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    // Defaults: current date, New Delhi
    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs();
    // Simple UTC time calculation (good enough for defaults)
    let days_since_epoch = (now / 86400) as i64;
    let secs_today = (now % 86400) as i64;
    // Approximate current date from JD
    let jd_now = days_since_epoch as f64 + 2440587.5 + secs_today as f64 / 86400.0;
    let (cur_year, cur_month, _) = hindu_calendar::ephemeris::julian_day::jd_to_gregorian(jd_now);

    let mut year = cur_year;
    let mut month = cur_month;
    let mut day = 0i32;
    let mut loc = Location::NEW_DELHI;
    let mut solar_mode = false;
    let mut solar_type = SolarCalendarType::Tamil;

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "-y" if i + 1 < args.len() => {
                i += 1;
                year = args[i].parse().unwrap_or(year);
            }
            "-m" if i + 1 < args.len() => {
                i += 1;
                month = args[i].parse().unwrap_or(month);
            }
            "-d" if i + 1 < args.len() => {
                i += 1;
                day = args[i].parse().unwrap_or(0);
            }
            "-s" if i + 1 < args.len() => {
                i += 1;
                match parse_solar_type(&args[i]) {
                    Some(t) => {
                        solar_type = t;
                        solar_mode = true;
                    }
                    None => {
                        eprintln!("Error: unknown solar calendar type '{}'", args[i]);
                        eprintln!("Valid types: tamil, bengali, odia, malayalam");
                        std::process::exit(1);
                    }
                }
            }
            "-l" if i + 1 < args.len() => {
                i += 1;
                let parts: Vec<&str> = args[i].split(',').collect();
                if parts.len() == 2 {
                    if let (Ok(lat), Ok(lon)) = (parts[0].parse::<f64>(), parts[1].parse::<f64>()) {
                        loc.latitude = lat;
                        loc.longitude = lon;
                    } else {
                        eprintln!("Error: invalid location format. Use LAT,LON");
                        std::process::exit(1);
                    }
                } else {
                    eprintln!("Error: invalid location format. Use LAT,LON");
                    std::process::exit(1);
                }
            }
            "-u" if i + 1 < args.len() => {
                i += 1;
                loc.utc_offset = args[i].parse().unwrap_or(loc.utc_offset);
            }
            "-h" => {
                print_usage(&args[0]);
                return;
            }
            _ => {
                eprintln!("Unknown option: {}", args[i]);
                print_usage(&args[0]);
                std::process::exit(1);
            }
        }
        i += 1;
    }

    if month < 1 || month > 12 {
        eprintln!("Error: month must be 1-12");
        std::process::exit(1);
    }

    let mut eph = Ephemeris::new();

    if solar_mode {
        if day > 0 {
            print_solar_day(&mut eph, year, month, day, &loc, solar_type);
        } else {
            print_solar_month(&mut eph, year, month, &loc, solar_type);
        }
    } else if day > 0 {
        let jd = eph.gregorian_to_jd(year, month, day);
        let jd_sunrise = eph.sunrise_jd(jd, &loc);
        let ti = hindu_calendar::core::tithi::tithi_at_sunrise(&mut eph, year, month, day, &loc);
        let hd = panchang::gregorian_to_hindu(&mut eph, year, month, day, &loc);

        let pd = PanchangDay {
            greg_year: year,
            greg_month: month,
            greg_day: day,
            jd_sunrise,
            hindu_date: hd,
            tithi: ti,
        };
        panchang::print_day_panchang(&eph, &pd, loc.utc_offset);
    } else {
        println!("Hindu Calendar — {:04}-{:02} ({:.4}°N, {:.4}°E, UTC{:+.1})\n",
            year, month, loc.latitude, loc.longitude, loc.utc_offset);

        let days = panchang::generate_month_panchang(&mut eph, year, month, &loc);
        panchang::print_month_panchang(&eph, &days, loc.utc_offset);
    }
}
