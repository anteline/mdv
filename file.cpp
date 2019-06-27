#include <file.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include <errno.h>
#include <string.h>


static size_t GetFileSize(char const *fname) noexcept
{   
    struct stat st;
    if (stat(fname, &st) == -1)
        return 0u;
    return st.st_size;
}

File::File(char const *fname) noexcept
:   mFd(0),
    mAddr(nullptr),
    mLength(0u)
{
    if (fname == nullptr)
    {
        std::cerr << "Failed to open file because its path is nullptr." << std::endl;
        return;
    }

    size_t length = GetFileSize(fname);
    if (length == 0u)
    {
        std::cerr << "Failed to load file length. Path=\"" << fname << "\" Error=" << strerror(errno) << std::endl;
        return;
    }

    int fd = open(fname, O_RDONLY);
    if (fd < 0)
    {
        std::cerr << "Failed to open file. Path=\"" << fname << "\" Error=" << strerror(errno) << std::endl;
        return;
    }

    void *addr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        std::cerr << "Failed to map file. Path=\"" << fname << "\" Error=" << strerror(errno) << std::endl;
        return;
    }

    mFd = fd, mAddr = addr, mLength = length;
}

File::~File()
{
    if (mAddr != nullptr)
    {
        if (0 != munmap(mAddr, mLength))
            std::cerr << "Failed to unmap file. Error=" << strerror(errno) << std::endl;
    }

    if (mFd != 0)
        close(mFd);
}

