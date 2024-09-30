use chrono::DateTime;
use chrono::Duration;
use chrono::Local;
use chrono::TimeZone;
use chrono::Utc;

/* pub fn get_duration(duration: u64) -> String {
    let duration_str = String::new();
    duration_str
} */

pub fn get_since_and_passed_time(timestamp_u64: u64) -> (String, String) {
    let since_local = get_since_date_local(timestamp_u64);

    let now = Local::now();

    let duration = now.signed_duration_since(since_local);

    (
        since_local.to_rfc2822(),
        most_significant_duration(duration),
    )
}

fn get_since_date_local(timestamp_u64: u64) -> DateTime<Local> {
    let since = match Utc.timestamp_micros(timestamp_u64 as i64) {
        chrono::offset::LocalResult::Single(a) => a,
        chrono::offset::LocalResult::Ambiguous(a, _b) => a,
        chrono::offset::LocalResult::None => panic!("timestamp_opt None"),
    };

    let since_local: DateTime<Local> = DateTime::from(since);
    since_local
}

fn most_significant_duration(duration: Duration) -> String {
    let days = duration.num_days();

    if days > 0 {
        let plur = if days == 1 { "" } else { "s" };

        return format!("{days} day{plur}");
    }

    let hours = duration.num_hours();
    if hours > 0 {
        let plur = if hours == 1 { "" } else { "s" };

        return format!("{hours} hour{plur}");
    }

    let minutes = duration.num_minutes();

    let dur = match minutes {
        0 => {
            format!("{}s", duration.num_seconds())
        }
        1..10 => {
            let seconds = duration.num_seconds() % 60;
            format!("{minutes}min {}s", seconds)
        }
        _ => {
            format!("{minutes} minutes")
        }
    };

    dur
}

#[cfg(test)]
mod tests {

    use chrono::TimeDelta;

    use super::*;

    #[test]
    fn test_since() {
        let since = get_since_and_passed_time(1727116768682604);
        println!("since {:?}", since);
        let since = get_since_and_passed_time(1727116768682442);
        println!("since {:?}", since);
        let since = get_since_and_passed_time(1727116768682435);
        println!("since {:?}", since);
        let since = get_since_and_passed_time(1727413184243915);
        println!("since {:?}", since);
    }

    #[test]
    fn test_duration() {
        let now = Local::now();

        let tomorrow_midnight = (now + Duration::days(1))
            .date_naive()
            .and_hms_opt(0, 0, 0)
            .unwrap();

        let tomorrow_midnight_local = tomorrow_midnight
            .and_local_timezone(Local)
            .earliest()
            .unwrap();

        let duration = tomorrow_midnight_local
            .signed_duration_since(now)
            .to_std()
            .unwrap();

        println!(
            "Duration between {:?} and {:?}: {:?}",
            now, tomorrow_midnight, duration
        );
    }

    #[test]
    fn test_duration2() {
        let prev = get_since_date_local(1727116768682604);

        let now = Local::now();

        let duration = now.signed_duration_since(prev);

        println!(
            "Duration between {:?} and {:?}: {:?}",
            prev,
            now,
            duration.to_std().unwrap()
        );

        println!("{} ago", most_significant_duration(duration))
    }

    #[test]
    fn most_significant_duration_test() {
        let a = TimeDelta::minutes(1) + TimeDelta::seconds(30);
        println!("{:?}", a);
        println!("{:?}", most_significant_duration(a));

        let b = TimeDelta::minutes(2);
        println!("{:?}", b);
        println!("{:?}", most_significant_duration(b));

        let a = TimeDelta::minutes(10) + TimeDelta::seconds(30);
        println!("{:?}", a);
        println!("{:?}", most_significant_duration(a));

        let a = TimeDelta::minutes(9) + TimeDelta::seconds(30);
        println!("{:?}", a);
        println!("{:?}", most_significant_duration(a));
    }

    #[test]
    fn test_duration_() {
        /*         ActiveEnterTimestamp	1727500436962647
        ActiveEnterTimestampMonotonic	383605378536
        ActiveExitTimestamp	1727501504134907
        ActiveExitTimestampMonotonic	384672550797 */

        let enter = get_since_date_local(1727500436962647);
        let exit = get_since_date_local(1727501504134907);

        let d = exit.signed_duration_since(enter);
        println!("{:?} {:?}", most_significant_duration(d), d);
    }
}
