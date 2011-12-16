#ifndef MYRWH
#define MYRWH

#include <stddef.h>
#include <fcntl.h>

size_t oread(int fd, void* pvoid, size_t size, off_t offset);
size_t owrite(int fd, const void* pvoid, size_t size, off_t offset);

size_t blockread(int fd, void* pvoid, size_t size);
size_t blockwrite(int fd, const void* pvoid, size_t size);

void   zerowrite(int fd, size_t size);

#endif

