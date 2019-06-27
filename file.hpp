#ifndef FILE_HPP
#define FILE_HPP
#include <stddef.h>

class File final
{
public:
    explicit File(char const *fname) noexcept;
    ~File();

    operator bool () const noexcept { return mAddr != nullptr; }

    void const * GetContent() const noexcept { return mAddr; }
    size_t GetLength() const noexcept { return mLength; }

private:
    int mFd;
    void *mAddr;
    size_t mLength;
};

#endif // FILE_HPP

