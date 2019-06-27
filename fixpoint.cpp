#include <fixpoint.hpp>
#include <string.h>

std::array<char, 24> Fixpoint::ToString() const noexcept
{
    std::array<char, 24> rtn;
    memset(&rtn, 0, sizeof(rtn));

    char buff[22];
    int64_t value = abs(mValue);
    size_t idx = sizeof(buff);
    do
    {
        buff[--idx] = char(value % 10 + '0');
        value /= 10;
    }
    while (0 < value);

    rtn[0] = '-';
    memcpy(rtn.data() + int(mValue < 0), buff + idx, sizeof(buff) - idx);
    return rtn;
}
