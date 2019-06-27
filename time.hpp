#ifndef TIME_HPP
#define TIME_HPP
#include <array>
#include <cstdint>
#include <limits>


class Time;

class Interval
{
public:
    constexpr Interval() noexcept : mNanos(0) { }

    constexpr std::int64_t Seconds() const noexcept { return mNanos / 1000 / 1000 / 1000; }
    constexpr std::int64_t TotalMilliseconds() const noexcept { return mNanos / 1000 / 1000; }
    constexpr std::int64_t TotalMicroseconds() const noexcept { return mNanos / 1000; }
    constexpr std::int64_t TotalNanoseconds() const noexcept { return mNanos; }

    constexpr std::int64_t FractionalMilliseconds() const noexcept { return (mNanos / 1000 / 1000) % 1000; }
    constexpr std::int64_t FractionalMicroseconds() const noexcept { return (mNanos / 1000) % (1000 * 1000); }
    constexpr std::int64_t FractionalNanoseconds() const noexcept { return mNanos % (1000 * 1000 * 1000); }

    constexpr bool operator==(Interval const &o) const noexcept { return mNanos == o.mNanos; }
    constexpr bool operator!=(Interval const &o) const noexcept { return !(*this == o); }

    constexpr bool operator<(Interval const &o) const noexcept { return mNanos < o.mNanos; }
    constexpr bool operator<=(Interval const &o) const noexcept { return !(o < *this); }
    constexpr bool operator>(Interval const &o) const noexcept { return o < *this; }
    constexpr bool operator>=(Interval const &o) const noexcept { return !(*this < o); }

    constexpr Interval operator+() const noexcept { return *this; }
    constexpr Interval operator-() const noexcept { return Interval(-mNanos); }

    constexpr Interval operator+(Interval const &o) const noexcept { return Interval(mNanos + o.mNanos); }
    constexpr Interval operator-(Interval const &o) const noexcept { return Interval(mNanos - o.mNanos); }
    constexpr Interval operator*(std::int64_t n) const noexcept { return Interval(mNanos * n); }
    constexpr Interval operator/(std::int64_t n) const noexcept { return Interval(mNanos / n); }
    constexpr std::int64_t operator/(Interval const &o) const noexcept { return mNanos / o.mNanos; }
    constexpr Interval operator%(Interval const& o) const noexcept { return Interval(mNanos % o.mNanos); }

    Interval& operator+=(Interval const &o) noexcept { return *this = *this + o; }
    Interval& operator-=(Interval const &o) noexcept { return *this = *this - o; }
    Interval& operator*=(std::int64_t n) noexcept { return *this = *this * n; }
    Interval& operator/=(std::int64_t n) noexcept { return *this = *this / n; }
    Interval& operator%=(Interval const& o) noexcept { return *this = *this % o; }

    friend constexpr Interval Nanoseconds(std::int64_t value) noexcept;

    std::array<char, 32> ToString() const noexcept;

private:
    explicit constexpr Interval(std::int64_t nanos) noexcept : mNanos(nanos) { }

    std::int64_t mNanos;

    template <typename Stream>
    friend Stream & operator <<(Stream &stream, Interval const &interval) noexcept
    {
        stream << interval.ToString().data();
        return stream;
    }
};

inline constexpr Interval operator*(std::int64_t n, Interval const &i) noexcept { return i * n; }

inline constexpr Interval Nanoseconds(std::int64_t value) noexcept { return Interval(value); }
inline constexpr Interval Microseconds(std::int64_t value) noexcept { return Nanoseconds(value * 1000); }
inline constexpr Interval Milliseconds(std::int64_t value) noexcept { return Nanoseconds(value * 1000 * 1000); }
inline constexpr Interval Seconds(std::int64_t value) noexcept { return Nanoseconds(value * 1000 * 1000 * 1000); }
inline constexpr Interval Minutes(std::int64_t value) noexcept { return Nanoseconds(value * 60 * 1000 * 1000 * 1000); }
inline constexpr Interval Hours(std::int64_t value) noexcept { return Nanoseconds(value * 60 * 60 * 1000 * 1000 * 1000); }
inline constexpr Interval Days(std::int64_t value) noexcept { return Nanoseconds(value * 24 * 60 * 60 * 1000 * 1000 * 1000); }
inline constexpr Interval Weeks(std::int64_t value) noexcept { return Nanoseconds(value * 7 * 24 * 60 * 60 * 1000 * 1000 * 1000); }


inline constexpr Time DistantPast() noexcept;
inline constexpr Time DistantFuture() noexcept;
inline constexpr Time PosixEpoch() noexcept;

class Time
{
public:
    constexpr Time() noexcept : mNanoPosixTime(0) { }

    constexpr Time(int32_t yyyy, int32_t mm, int32_t dd) noexcept : mNanoPosixTime(IsValidDay(yyyy, mm, dd) ? DaysToDate(yyyy, mm, dd).TotalNanoseconds() : std::numeric_limits<std::int64_t>::min()) { }

    constexpr bool operator==(Time const &o) const noexcept { return mNanoPosixTime == o.mNanoPosixTime; }
    constexpr bool operator!=(Time const &o) const noexcept { return !(*this == o); }

    constexpr bool operator<(Time const &o) const noexcept { return mNanoPosixTime < o.mNanoPosixTime; }
    constexpr bool operator<=(Time const &o) const noexcept { return !(o < *this); }
    constexpr bool operator>(Time const &o) const noexcept { return o < *this; }
    constexpr bool operator>=(Time const &o) const noexcept { return !(*this < o); }

    constexpr Time operator+(Interval const &i) const noexcept { return Time(mNanoPosixTime + i.TotalNanoseconds()); }
    constexpr Time operator-(Interval const &i) const noexcept { return Time(mNanoPosixTime - i.TotalNanoseconds()); }

    constexpr Interval operator-(Time const &o) const noexcept { return Nanoseconds(mNanoPosixTime - o.mNanoPosixTime); }

    constexpr Interval ClockTime() const noexcept { return Nanoseconds(mNanoPosixTime % (86400ll * 1000 * 1000 * 1000)); }
    constexpr Time Date() const noexcept { return *this - ClockTime(); }

    Time& operator+=(Interval i) noexcept { return *this = *this + i; }
    Time& operator-=(Interval i) noexcept { return *this = *this - i; }

    friend constexpr Time DistantPast() noexcept;
    friend constexpr Time DistantFuture() noexcept;
    friend constexpr Time PosixEpoch() noexcept;

    std::array<char, 32> ToString() const noexcept;

private:
    explicit constexpr Time(std::int64_t npt) noexcept : mNanoPosixTime(npt) { }

    static constexpr bool IsLeapYear(std::int32_t yyyy) noexcept
    {
        return !(yyyy % 4) && ((yyyy % 100) || !(yyyy % 400));
    }

    static constexpr int32_t DaysInMonth(int32_t mm) noexcept
    {
        // {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12} => {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
        return 30 + int(mm % 2 == int(mm < 8)) - int(mm == 2);
    }

    static constexpr bool IsValidDay(std::int32_t yyyy, std::int32_t mm, std::int32_t dd) noexcept
    {
        return (1700 < yyyy) && (yyyy < 2201) && (0 < mm) && (mm < 13) && (0 < dd) && (dd <= DaysInMonth(mm)) && ((mm != 2) || (dd < 29) || IsLeapYear(yyyy));
    }

    static constexpr int32_t DaysPassedAtMonth(int32_t mm) noexcept
    {
        return (mm + 1) / 3 - int((2 < mm) & (mm < 9)) - int(mm == 3) - int(mm == 5) + (mm - 1) * 30;
    }

    static constexpr int32_t DaysInTheYear(std::int32_t yyyy, std::int32_t mm, std::int32_t dd) noexcept
    {
        return DaysPassedAtMonth(mm) + int32_t(IsLeapYear(yyyy) and 2 < mm) + dd - 1;
    }

    static constexpr int32_t DaysInPreviousYears(int32_t years) noexcept
    {
        return years * 365 + years / 4 - years / 100 + int32_t(299 < years) - 98250;
    }

    static constexpr Interval DaysToDate(std::int32_t yyyy, std::int32_t mm, std::int32_t dd) noexcept
    {
        return Days(DaysInPreviousYears(yyyy - 1701) + DaysInTheYear(yyyy, mm, dd));
    }

    std::int64_t mNanoPosixTime;

    void ToString(char (&buff)[20]) const noexcept;

    template <typename Stream>
    friend Stream & operator <<(Stream &stream, Time const &time) noexcept
    {
        char buff[20] = {0};
        time.ToString(buff);
        stream << buff;
        return stream;
    }
};

inline constexpr Time operator+(Interval const &interval, Time const &time) noexcept { return time + interval; }

inline constexpr Time DistantPast() noexcept { return Time(std::numeric_limits<std::int64_t>::min()); }
inline constexpr Time DistantFuture() noexcept { return Time(std::numeric_limits<std::int64_t>::max()); }
inline constexpr Time PosixEpoch() noexcept { return Time(0); }

#endif

