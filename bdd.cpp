#define _FILE_OFFSET_BITS 64

#include <unistd.h>
#include <errno.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/hdreg.h>
#include <sys/ioctl.h>

#include <stdexcept>
#include <vector>
#include <string>
using std::string;

#include "param.h"
#include "myrw.h"
#include "chunk.h"

#define CHUNK_SIZE (512)

struct header
{
char        m_model[40];
char        m_sn[20];
uint64_t    m_ssize; //sector size
uint64_t    m_bsize; //block size (in sectors)
uint64_t    m_dsize; //disk size (in bytes)
uint64_t    m_good;
};

void dinfo(int dev, header& h);
uint64_t fsize(int fd);
uint64_t get_ch_count(const header& h, size_t ch_size);

struct status_arg
{
header*     m_header;
uint64_t*   m_pos;
uint64_t*   m_nreads;
uint64_t*   m_greads;
};

double time()
{
struct timespec ts;
clock_gettime( CLOCK_REALTIME, &ts );
double result = ts.tv_sec;
result *= 1000 * 1000 * 1000;
result += ts.tv_nsec;
result /= 1000 * 1000 * 1000;
return result;
}

void* status(void* p)
{
status_arg* arg = (status_arg*)p;
uint64_t old_nreads = *arg->m_nreads;
double otime = 0.0;
double ctime;
while ( true )
    {
    sleep( 1 );
    uint64_t pos = *arg->m_pos;
    uint64_t nreads = *arg->m_nreads;
    if ( old_nreads == nreads )
        {
        continue;
        }
    uint64_t greads = *arg->m_greads;
    uint64_t breads = nreads - greads;
    ctime = time();
    double etime = (ctime - otime);
    uint64_t speed = (nreads - old_nreads) * arg->m_header->m_bsize * arg->m_header->m_ssize;
    speed = (double)speed / etime;
    unsigned int g = speed / (1024*1024*1024);
    speed %= (1024*1024*1024);
    unsigned int m = speed / (1024*1024);
    speed %= (1024*1024);
    unsigned int k = speed / (1024);
    old_nreads = nreads;
    fprintf( stdout, "\rpos: %llu nreads: %llu greads: %llu breads: %llu", pos, nreads, greads, breads );
    fprintf( stdout, " speed: %ug %um %uk         ", g, m, k );
    fflush( stdout );
    otime = ctime;
    }
return NULL;
}

void init(int fd, header& h)
{
owrite( fd, &h, sizeof(header), 0 );
uint64_t size = get_ch_count( h, sizeof(size_t) * CHUNK_SIZE ) * CHUNK_SIZE * sizeof(size_t);
zerowrite( fd, size );
}

int create(const string& filename, header& h)
{
int fd = ::open( filename.c_str(), O_LARGEFILE | O_TRUNC | O_CREAT | O_RDWR, 0600 );
if ( -1 == fd )
    {
    throw std::runtime_error( "can't open/create bitmask file" );
    }
init( fd, h );
return fd;
}

int open(const string& filename, header& h)
{
int fd = ::open( filename.c_str(), O_LARGEFILE | O_CREAT | O_RDWR, 0600 );
if ( -1 == fd )
    {
    throw std::runtime_error( "can't open/create bitmask file" );
    }
header l_header;
int nread = oread( fd, &l_header, sizeof(header), 0 );
//TODO
if ( 0 == nread )
    {
    init( fd, h );
    return fd;
    }
if ( memcmp( &l_header, &h, sizeof(header) - sizeof(h.m_good) ) )
    {
    throw; //TODO:
    }
uint64_t size = get_ch_count( h, sizeof(size_t) * CHUNK_SIZE ) * CHUNK_SIZE * sizeof(size_t);
if ( fsize( fd ) - sizeof(header) != size )
    {
    throw; //TODO:
    }
return fd;
}

void dinfo(int dev, header& h)
{
static struct hd_driveid hd;

if ( ioctl( dev, HDIO_GET_IDENTITY, &hd ) )
    {
    throw;
    }

memcpy( h.m_model, hd.model, sizeof( h.m_model ) );
memcpy( h.m_sn, hd.serial_no, sizeof( h.m_sn ) );
}

uint64_t fsize(int fd)
{
off_t cur = lseek( fd, 0, SEEK_CUR );
lseek( fd, 0, SEEK_END );
uint64_t result = lseek( fd, 0, SEEK_CUR );
lseek( fd, cur, SEEK_SET );
return result;
}

uint64_t get_ch_count(const header& h, size_t ch_size)
{
uint64_t bitcount = ch_size * 8;
uint64_t scount = h.m_dsize / h.m_ssize;
uint64_t ch_count = scount / bitcount;
if ( 0 != scount % bitcount )
    {
    ++ch_count;
    }
return ch_count;
}

inline bool sec_read(int fd, const uint64_t& gpos)
{
}

int main(int argc, char* argv[])
{
params( argc, argv );
uint64_t p_secsize = 512; 
uint64_t p_blocksize = 1;
uint64_t p_chunksize = 4*1024;

int ifd = open( p_if, O_LARGEFILE | O_RDONLY );
if ( -1 == ifd )
    {
    throw std::runtime_error( "can't open dev file" );
    }

header h = {0};

//diskmap::dinfo( dev, h );
h.m_ssize = p_secsize;
h.m_bsize = p_blocksize;
h.m_dsize = fsize( ifd );
h.m_good = 0;

int mfd = -1;
int ofd = open( p_of, O_LARGEFILE | O_CREAT | O_WRONLY, 0600 );
if ( p_truncate )
    {
    mfd = create( p_mf, h );
    }
else
    {
    mfd = open( p_mf, h );
    }

typedef std::vector<size_t> map_t;
map_t map( CHUNK_SIZE / sizeof(size_t) );
chunk_t mapctl( mfd, sizeof(header), &(*map.begin()), map.size() * sizeof(size_t) );

uint64_t gpos = 0;
uint64_t nreads = 0;
uint64_t greads = 0;
uint64_t scount = h.m_dsize / h.m_ssize;
status_arg arg = { &h, &gpos, &nreads, &greads };
pthread_t th_id;
int err = pthread_create( &th_id, NULL, status, &arg );

uint64_t ch_count = get_ch_count( h, sizeof(size_t) * map.size() );
for (uint64_t ch_pos = 0; ch_pos < ch_count; ++ch_pos)
    {
    mapctl.set( ch_pos );
    mapctl.read();
    bool need_write = false;
    for (size_t sz_pos = 0; map.size() != sz_pos; ++sz_pos)
        {
        if ( (size_t)(-1) == map[sz_pos] )
            {
            continue;
            }
        size_t bitmask = map[sz_pos];
        for (int bit_pos = 0; bit_pos < sizeof(size_t)*8; ++bit_pos)
            {
            gpos = ( mapctl.get() * map.size() + sz_pos ) * sizeof(size_t) * 8 + bit_pos;
            bool bit = bitmask & (1 << bit_pos);
            if ( !bit )
                {
                char buff[512];
                bool good = sizeof(buff) == oread( ifd, buff, sizeof(buff), gpos * 512 );
                ++nreads;
                if ( good )
                    {
                    bitmask |= 1 << bit_pos;
                    owrite( ofd, buff, sizeof(buff), gpos * 512 );
                    need_write = true;
                    ++greads;
                    }
                }
            }
        map[sz_pos] = bitmask;
        }
    if ( need_write );
        {
        mapctl.write();
        }
    }

fprintf( stdout, "\n" );
return 0;
}

