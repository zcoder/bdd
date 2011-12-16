#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

#include "myrw.h"

size_t owrite(int fd, const void* pvoid, size_t size, off_t offset)
{
lseek( fd, offset, SEEK_SET );
return blockwrite( fd, pvoid, size );
}

size_t blockwrite(int fd, const void* pvoid, size_t size)
{
const char* buff = (const char*)pvoid;
int nwrite = 0;
//TODO: ret
while ( nwrite < size )
    {
    int err = write( fd, buff + nwrite, (size - nwrite) & INT_MAX );
    if ( err < 0 )
        {
        /*
        EAGAIN
        EBADF
        EDESTADDRREQ
        EFAULT
        EFBIG
        EINTR
        EINVAL
        EIO
        ENOSPC
        EPIPE
        */
        int err = errno;
        throw err;
        }
    else if ( 0 == err )
        {
        return nwrite;
        }
    nwrite +=err;
    }
return nwrite;
}

void zerowrite(int fd, size_t size)
{
const char zerobuff[64*1024] = {0};
while ( size > sizeof(zerobuff) )
    {
    int nwrite = write( fd, zerobuff, sizeof(zerobuff ) );
    if ( nwrite < 0 )
        {
        throw;
        }
    size -= nwrite;
    }
blockwrite( fd, zerobuff, size );
}

size_t oread(int fd, void* pvoid, size_t size, off_t offset)
{
lseek( fd, offset, SEEK_SET );
return blockread( fd, pvoid, size );
}

size_t blockread(int fd, void* pvoid, size_t size)
{
char* buff = (char*)pvoid;
size_t nread = 0;
while ( nread < size )
    {
    int err = read( fd, buff + nread, (size - nread) & INT_MAX );
    if ( 0 == err )
        {
        return nread;
        }
    else if ( -1 == err )
        {
        switch ( errno )
            {
            case EAGAIN:
            case EINTR:
                continue;
            case EBADF:
                fprintf( stderr, "invalid fd\n" );
                throw;
            case EFAULT:
                fprintf( stderr, "internal error\n" );
                throw;
            case EINVAL:
                fprintf( stderr, "unsuitable for reading\n" );
                throw;
            case EISDIR:
                fprintf( stderr, "is a dir\n" );
                throw;
            case EIO:
                return nread;
                break;
            }
        }
    nread += err;
    }
return nread;
}

