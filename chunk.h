#ifndef CHUNKH
#define CHUNKH

#include <stddef.h>
#include "myrw.h"

class chunk_t
{
public:

protected:
int     m_fd;
off_t   m_offset;
void*   m_buff;
size_t  m_size;
int64_t m_index;

public:
                    chunk_t(int fd, off_t offset, void* buff, size_t size);
                    ~chunk_t();
inline  void        close();
inline  void        read();
inline  void        write();

inline  void        set(uint64_t index);
inline  uint64_t    get() const;
inline  void        next();
};

chunk_t::chunk_t(int fd, off_t offset, void* buff, size_t size):
    m_fd( fd ), m_offset( offset ), m_buff( buff ), m_size( size ), m_index( 0 )
{
}

chunk_t::~chunk_t()
{
close();
}

void chunk_t::close()
{
if ( -1 != m_fd )
    {
    ::close( m_fd );
    m_fd = -1;
    }
}

void chunk_t::set(uint64_t index)
{
m_index = index;
}

uint64_t chunk_t::get() const
{
return m_index;
}

void chunk_t::next()
{
++m_index;
read();
}

void chunk_t::read()
{
oread( m_fd, m_buff, m_size, m_offset + m_index * m_size );
//TODO:
}

void chunk_t::write()
{
owrite( m_fd, m_buff, m_size, m_offset + m_index * m_size );
//TODO:
}

#endif

