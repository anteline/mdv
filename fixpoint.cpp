#include <fixpoint.hpp>
#include <stdio.h>
#include <string.h>

std::array<char, 24> Fixpoint::ToString() const noexcept
{
    std::array<char, 24> rtn;
    memset(&rtn, 0, sizeof(rtn));
    rtn[0] = '-';
    char *addr = rtn.data() + int(mValue < 0);
    size_t length = sizeof(rtn) - uint32_t(int32_t(mValue < 0));

    int64_t value = abs(mValue);
    int64_t integral = value / GetFactor();
    int64_t fractional = value % GetFactor();
    if (fractional == 0)
    {
        snprintf(addr, length, "%ld", integral);
    }
    else
    {
        int idx = snprintf(addr, length, "%ld.%04ld", integral, fractional) - 1;
        for (uint32_t i = 0; i < GetPrecision(); ++i)
        {
            if (rtn[idx - int32_t(i)] == '0')
            {
                rtn[idx - int32_t(i)] = char(0);
            }
            else
            {
                break;
            }
        }
    }
    return rtn;
}
