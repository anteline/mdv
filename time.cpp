#include <time.hpp>

#include <cassert>
#include <cstdio>


enum : int64_t { NANOS_IN_ONE_SEC = 1000 * 1000 * 1000 };

namespace {

template <typename ...Args>
static void Write(char *&addr, size_t &len, char const *fmt, Args const &...args) noexcept
{
    int res = snprintf(addr, len, fmt, args...);
    assert(0 <= res and unsigned(res) <= len);

    addr += res;
    len -= size_t(ssize_t(res));
}

} // anonymous namespace

std::array<char, 32> Interval::ToString() const noexcept
{
    // format is xd23h59m59s999ms999us999ns
    static_assert(std::numeric_limits<int64_t>::max() / NANOS_IN_ONE_SEC / 86400 < 999999, "Insufficient buff size.");
    std::array<char, 32> buff;
    char *addr = buff.data();
    size_t len = sizeof(buff);

    int64_t days = mNanos / NANOS_IN_ONE_SEC / 86400;
    if (days != 0)
        Write(addr, len, "%ldd", days);

    int64_t secsLessThanOneDay = mNanos / NANOS_IN_ONE_SEC % 86400;
    if (secsLessThanOneDay != 0)
    {
        int64_t hours = secsLessThanOneDay / 3600;
        if (hours != 0)
            Write(addr, len, "%ldh", hours);

        int64_t mins  = secsLessThanOneDay % 3600 / 60;
        if (mins != 0)
            Write(addr, len, "%ldm", mins);

        int64_t secs  = secsLessThanOneDay % 60;
        if (secs != 0)
            Write(addr, len, "%lds", secs);
    }

    int64_t nanosLessThanOneSec = mNanos % NANOS_IN_ONE_SEC;
    if (nanosLessThanOneSec != 0)
    {
        int64_t ms = nanosLessThanOneSec / 1000 / 1000;
        if (ms != 0)
            Write(addr, len, "%ldms", ms);

        int64_t us = nanosLessThanOneSec / 1000 % 1000;
        if (us != 0)
            Write(addr, len, "%ldus", us);

        int64_t ns = nanosLessThanOneSec % 1000;
        if (ns != 0)
            Write(addr, len, "%ldns", ns);
    }
    return buff;
}

static void DumpClockTime(char *buff, Interval clockTime) noexcept
{
    Interval nanos = clockTime % Seconds(1);
    int64_t numSeconds = clockTime / Seconds(1);
    int64_t hh = numSeconds / 3600;
    int64_t mm = numSeconds % 3600 / 60;
    int64_t ss = numSeconds % 60;
    snprintf(buff, 20, "%02ld:%02ld:%02ld.%09ld", hh, mm, ss, nanos.TotalNanoseconds());
}

std::array<char, 32> Time::ToString() const noexcept
{
    int32_t const daysPassedAtMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

    int32_t yyyy = 2000, mm = 2, dd = 29;

    int32_t numDays = int32_t((*this - PosixEpoch()) / Days(1)); // number of days since epoch
    if (numDays != 11016) // not 2000-02-29
    {
        numDays += 98249 + int(numDays < 11016); // number of days since 1701-01-01, exclude 2000-02-29
        int32_t numCenturies = numDays / 36524;
        int32_t numDaysInTheCentury = numDays % 36524;
        int32_t numDaysIn4Years = numDaysInTheCentury % 1461;
        int32_t numYearsInTheCentury = numDaysInTheCentury / 1461 * 4 + numDaysIn4Years / 365 - int(numDaysIn4Years == 1460);
        yyyy = 1701 + numCenturies * 100 + numYearsInTheCentury;
        bool leapYear = (yyyy % 4 == 0) and (yyyy % 100 != 0);

        mm = 12, dd = 31;
        if (numDaysIn4Years != 1460)
        {
            int32_t numDaysInTheYear = numDaysIn4Years % 365;
            if (numDaysInTheYear == 59)
            {
                if (leapYear)
                {
                    mm = 2, dd = 29;
                }
                else
                {
                    mm = 3, dd = 1;
                }
            }
            else
            {
                if ((59 < numDaysInTheYear) and leapYear)
                    --numDaysInTheYear;

                mm = 1;
                while (daysPassedAtMonth[mm] <= numDaysInTheYear)
                    ++mm;
                dd = numDaysInTheYear - daysPassedAtMonth[mm - 1] + 1;
            }
        }
    }

    std::array<char, 32> rtn;
    sprintf(rtn.data(), "%04d-%02d-%02d ", yyyy, mm, dd);
    DumpClockTime(rtn.data() + 11, (*this - PosixEpoch()) % Days(1));
    return rtn;
}

void Time::ToString(char (&buff)[20]) const noexcept
{
    DumpClockTime(buff, (*this - PosixEpoch()) % Days(1));
}

