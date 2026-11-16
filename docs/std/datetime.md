# `std.datetime`

`stdlib/datetime.npp` converts epoch milliseconds to civil UTC dates using
integer arithmetic (Howard Hinnant's civil-from-days algorithm). It is pure
N++ and runs on the builtin native backend.

| Function | Description |
|---|---|
| `datetime_format(ms: i64) -> string` | `"YYYY-MM-DD HH:MM:SS"` for epoch ms |
| `datetime_now() -> string` | Current UTC time formatted |
| `datetime_weekday(ms: i64) -> i64` | 0 = Sunday … 6 = Saturday |
| `dt_is_leap(year: i64) -> bool` | Gregorian leap-year check |
| `dt_days_in_month(year: i64, month: i64) -> i64` | 28–31 |

## Example

```npp
import std.datetime
import std.time

fn main() {
    print(datetime_format(0))        // 1970-01-01 00:00:00
    print(datetime_now())
    print(datetime_weekday(time_ms()))
}
```
