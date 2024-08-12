/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         create and throw
* ©overcq                on ‟Gentoo Linux 23.0” “x86_64”             2024‒6‒22 P
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
//------------------------------------------------------------------------------
#include "../lang.h"
#include "fs.h"
//==============================================================================
extern const gfp_t E_oux_E_fs_S_kmalloc_flags;
extern rwlock_t E_oux_E_fs_S_rw_lock;
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
static
bool
H_oux_E_fs_Q_file_T_blocks_sorted( unsigned device_i
, uint64_t file_i
){  if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   uint64_t sector_prev = H_oux_E_fs_Q_device_S[ device_i ].block_table[0].sector;
        for( uint64_t block_table_i = 1; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
        {   uint64_t sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].sector;
            if( sector < sector_prev )
            {   pr_err( "blocks not sorted: file_i=%llu\n", file_i );
                return no;
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   uint64_t sector_end = sector_prev + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.sectors.n;
                    if( sector_end > sector
                      || ( sector_end == sector
                        && ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.sectors.post
                          || H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location.sectors.pre
                      ))
                      || ( sector_end == sector - 1
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.sectors.post
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location.sectors.pre
                          >= H_oux_E_fs_S_sector_size
                    ))
                    {   pr_err( "blocks intersecting or adjacent: file_i=%llu\n", file_i );
                        return no;
                    }
                }else
                {   uint64_t end_prev = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.in_sector.start
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.in_sector.size;
                    if( sector_prev == sector - 1
                    && end_prev + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size
                    )
                    {   pr_err( "blocks intersecting or adjacent: file_i=%llu\n", file_i );
                        return no;
                    }
                }
            else
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   uint64_t sector_end = sector_prev + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.sectors.n;
                    if( sector_end == sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.sectors.post
                      >= H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location.in_sector.start
                    )
                    {   pr_err( "blocks intersecting or adjacent: file_i=%llu\n", file_i );
                        return no;
                    }
                }else
                {   uint64_t end_prev = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.in_sector.start
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i - 1 ].location.in_sector.size;
                    if( sector_prev == sector - 1
                    && end_prev == H_oux_E_fs_S_sector_size
                    && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ].location.in_sector.start
                    )
                    {   pr_err( "blocks adjacent: file_i=%llu\n", file_i );
                        return no;
                    }
                }
            sector_prev = sector;
        }
    }
    return yes;
}
#define H_oux_E_fs_Q_device_I_switch_item( type, item, end ) \
    if( data_i ) \
    {   do \
        {   item |= (type)*data << data_i * 8; \
            data += sizeof(type); \
        }while( ++data_i != sizeof(type)); \
        data_i = 0; \
    }else \
    {   if( data + sizeof(type) > (end) ) \
        {   do \
            {   item |= (type)*data << data_i++ * 8; \
            }while( ++data != (end) ); \
            break; \
        } \
        item = *( type * )data; \
        data += sizeof(type); \
    } \
    continue_from++
//------------------------------------------------------------------------------
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_M
, const char __user *, pathname
){  write_lock( &E_oux_E_fs_S_rw_lock );
    char *pathname_ = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    int error;
    long count = strncpy_from_user( pathname_, pathname, H_oux_E_fs_S_sector_size );
    if( count == -EFAULT )
    {   kfree( pathname_ );
        error = -EFAULT;
        goto Error_0;
    }
    if( count == H_oux_E_fs_S_sector_size )
    {   kfree( pathname_ );
        error = -ENAMETOOLONG;
        goto Error_0;
    }
    struct file *bdev_file = bdev_file_open_by_path( pathname_, BLK_OPEN_READ | BLK_OPEN_WRITE | BLK_OPEN_EXCL, H_oux_E_fs_Q_device_S, 0 );
    kfree( pathname_ );
    if( IS_ERR( bdev_file ))
    {   error = PTR_ERR( bdev_file );
        goto Error_0;
    }
    void *p = krealloc_array( H_oux_E_fs_Q_device_S, H_oux_E_fs_Q_device_S_n + 1, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S = p;
    H_oux_E_fs_Q_device_S_n++;
    long device_i = H_oux_E_fs_Q_device_S_n - 1;
    H_oux_E_fs_Q_device_S[ device_i ].bdev_file = bdev_file;
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_1;
    }
    loff_t offset = 0;
    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
    if( size != H_oux_E_fs_S_sector_size )
    {   pr_err( "read sector: 0\n" );
        error = -EIO;
        goto Error_2;
    }
    if( strncmp( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident ) - 1 ))
    {   pr_err( "no filesystem identification string\n" );
        error = -EIO;
        goto Error_2;
    }
    uint64_t *block_table_n = H_oux_J_align_up_p( sector + sizeof( H_oux_E_fs_Q_device_S_ident ) - 1, uint64_t );
    H_oux_E_fs_Q_device_S[ device_i ].block_table_n = block_table_n[0];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n = block_table_n[1];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start = block_table_n[2];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n = block_table_n[3];
    H_oux_E_fs_Q_device_S[ device_i ].directory_n = block_table_n[4];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start = block_table_n[5];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n = block_table_n[6];
    H_oux_E_fs_Q_device_S[ device_i ].file_n = block_table_n[7];
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_n < H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
      && ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n < H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
        || H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n > H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    ))
    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
      && ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n < H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
        || H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n > H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
    ))
    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
      && H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
      && (( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start >= H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
          && H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start < H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
        )
        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start < H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
          && H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n > H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
        )
        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start >= H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
          && H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start < H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
        )
        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start < H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
          && H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n > H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    ))))
    {   pr_err( "filesystem header inconsistent\n" );
        error = -EIO;
        goto Error_2;
    }
    char *data = ( void * )&block_table_n[8];
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_2;
    }
    H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_3;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_4;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    // Odczyt tablicy bloków do pamięci operacyjnej.
    unsigned continue_from = 0;
    unsigned data_i = 0;
    uint64_t block_table_i = ~0ULL;
    while( data < sector + H_oux_E_fs_S_sector_size ) // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
    {   switch( continue_from )
        { case 0:
                block_table_i++;
                if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                    goto End_loop;
                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                , sector + H_oux_E_fs_S_sector_size
                );
          default:
                if( continue_from == 1 )
                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                    , sector + H_oux_E_fs_S_sector_size
                    );
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                    )
                    {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                        error = -EIO;
                        goto Error_5;
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    switch( continue_from )
                    { case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                            {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                      case 4:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                            {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                            && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                              || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            ))
                            {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            continue_from = 0;
                    }
                else
                    switch( continue_from )
                    { case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                            {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                              > H_oux_E_fs_S_sector_size
                            )
                            {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            continue_from = 0;
                    }
        }
    }
End_loop:;
    for( uint64_t block_table_i_read = 0; block_table_i_read != H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n; block_table_i_read++ ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
    {   if( block_table_i_read > block_table_i )
        {   pr_err( "read beyond available: block_table_i=%llu\n", block_table_i );
            error = -EIO;
            goto Error_5;
        }
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector - 1 );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre );
                do
                {   switch( continue_from )
                    { case 0:
                            block_table_i++;
                            if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                                )
                                {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                          || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        ))
                                        {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                        {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                          > H_oux_E_fs_S_sector_size
                                        )
                                        {   pr_err( "in sector end not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + sector_i );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case 0:
                            block_table_i++;
                            if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                                )
                                {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                          || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        ))
                                        {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_S_sector_size
                                        );
                                        if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                        {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                          > H_oux_E_fs_S_sector_size
                                        )
                                        {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case 0:
                            block_table_i++;
                            if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                            );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                                )
                                {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                                        {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                          || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        ))
                                        {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                        {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                          > H_oux_E_fs_S_sector_size
                                        )
                                        {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_5;
                                        }
                                        continue_from = 0;
                                }
                    }
                }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector );
                error = -EIO;
                goto Error_5;
            }
            char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start;
            do // Czyta wpisy pliku tablicy bloków znajdujące się we fragmencie sektora.
            {   switch( continue_from )
                { case 0:
                        block_table_i++;
                        if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                        {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                        );
                  default:
                        if( continue_from == 1 )
                        {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                            );
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                            )
                            {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                        }
                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                            switch( continue_from )
                            { case 2:
                                    H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                              case 3:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                                    {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                              case 4:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                                    {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                    && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                      || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                    ))
                                    {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                                    continue_from = 0;
                            }
                        else
                            switch( continue_from )
                            { case 2:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                              case 3:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                    {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                      > H_oux_E_fs_S_sector_size
                                    )
                                    {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                                    continue_from = 0;
                            }
                }
            }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size );
        }
    }
    if( continue_from )
    {   pr_err( "too few blocks size\n" );
        error = -EIO;
        goto Error_5;
    }
    // Odczyt tablicy katalogów do pamięci operacyjnej.
    if( !H_oux_E_fs_Q_device_S[ device_i ].directory_n
    && H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
    )
    {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start );
        error = -EIO;
        goto Error_5;
    }
    uint64_t uid_last = ~0ULL;
    uint64_t directory_i = ~0ULL;
    data_i = 0;
    unsigned char_i;
    for( uint64_t directory_table_i = 0; directory_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n; directory_table_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre ));
                do
                {   switch( continue_from )
                    { case 0:
                            directory_i++;
                            if( directory_i == H_oux_E_fs_Q_device_S[ device_i ].directory_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                            {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            p = kmalloc( sector + H_oux_E_fs_S_sector_size - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                      case 2:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_S_sector_size );
                            if( data == sector + H_oux_E_fs_S_sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n; sector_i++ )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case 0:
                            directory_i++;
                            if( directory_i == H_oux_E_fs_Q_device_S[ device_i ].directory_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                            {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            p = kmalloc( sector + H_oux_E_fs_S_sector_size - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                      case 2:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_S_sector_size );
                            if( data == sector + H_oux_E_fs_S_sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = ( void * )sector;
                do
                {   switch( continue_from )
                    { case 0:
                            directory_i++;
                            if( directory_i == H_oux_E_fs_Q_device_S[ device_i ].directory_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                            {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                            {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                            );
                            p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                      case 2:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_S_sector_size );
                            if( data == sector + H_oux_E_fs_S_sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                error = -EIO;
                goto Error_5;
            }
            char *data = ( void * )( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start );
            do
            {   switch( continue_from )
                { case 0:
                        directory_i++;
                        if( directory_i == H_oux_E_fs_Q_device_S[ device_i ].directory_n )
                        {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        );
                  case 1:
                        if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                        {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        if( ~uid_last
                        && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                        )
                        {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        );
                        p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size - data, E_oux_E_fs_S_kmalloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        char_i = 0;
                  case 2:
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                            if( !*data )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                break;
                            }
                        }while( ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        );
                        if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        }else
                        {   data++;
                            continue_from = 0;
                        }
                }
            }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
              + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
            );
        }
    // Odczyt tablicy plików do pamięci operacyjnej.
    uid_last = ~0ULL;
    uint64_t file_i = ~0ULL;
    data_i = 0;
    if( !H_oux_E_fs_Q_device_S[ device_i ].file_n
    && H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
    )
    {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start );
        error = -EIO;
        goto Error_5;
    }
    for( uint64_t file_table_i = 0; file_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n; file_table_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre );
                do
                {   switch( continue_from )
                    { case 0:
                            file_i++;
                            if( file_i == H_oux_E_fs_Q_device_S[ device_i ].file_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                            {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
                            {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                            {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( !H_oux_E_fs_Q_file_T_blocks_sorted( device_i, file_i ))
                            {   error = -EIO;
                                goto Error_5;
                            }
                            p = kmalloc( sector + H_oux_E_fs_S_sector_size - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                      case 4:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_S_sector_size );
                            if( data == sector + H_oux_E_fs_S_sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n; sector_i++ )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case 0:
                            file_i++;
                            if( file_i == H_oux_E_fs_Q_device_S[ device_i ].file_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                            {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_S_sector_size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
                            {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_S_sector_size
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                            {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( !H_oux_E_fs_Q_file_T_blocks_sorted( device_i, file_i ))
                            {   error = -EIO;
                                goto Error_5;
                            }
                            p = kmalloc( sector + H_oux_E_fs_S_sector_size - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                      case 4:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_S_sector_size );
                            if( data == sector + H_oux_E_fs_S_sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_S_sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                    error = -EIO;
                    goto Error_5;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case 0:
                            file_i++;
                            if( file_i == H_oux_E_fs_Q_device_S[ device_i ].file_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                            );
                      case 1:
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                            {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                            {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
                            {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                            );
                            if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                            {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( !H_oux_E_fs_Q_file_T_blocks_sorted( device_i, file_i ))
                            {   error = -EIO;
                                goto Error_5;
                            }
                            p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post - data, E_oux_E_fs_S_kmalloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                      case 4:
                            do
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    break;
                                }
                            }while( ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = 0;
                            }
                    }
                }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                error = -EIO;
                goto Error_5;
            }
            char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start;
            do
            {   switch( continue_from )
                { case 0:
                        file_i++;
                        if( file_i == H_oux_E_fs_Q_device_S[ device_i ].file_n )
                        {   pr_err( "too much blocks size: block_table_i=%llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                  case 1:
                        if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                        {   pr_err( "uid empty: file_i=%llu\n", file_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        if( ~uid_last
                        && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                        )
                        {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                  case 2:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                  case 3:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                        if( !H_oux_E_fs_Q_file_T_blocks_sorted( device_i, file_i ))
                        {   error = -EIO;
                            goto Error_5;
                        }
                        p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size - data
                        , E_oux_E_fs_S_kmalloc_flags
                        );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        char_i = 0;
                  case 4:
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                            if( !*data )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_kmalloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_5;
                                }
                                break;
                            }
                        }while( ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                        if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name
                            , char_i + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            , E_oux_E_fs_S_kmalloc_flags
                            );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                error = -ENOMEM;
                                goto Error_5;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        }else
                        {   data++;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                            continue_from = 0;
                        }
                }
            }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
            );
        }
    if( continue_from )
    {   pr_err( "too few blocks size\n" );
        error = -EIO;
        goto Error_5;
    }
    // Utworzenie tablicy wolnych bloków.
    p = kmalloc_array( 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_5;
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].sector = 1;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.n = i_size_read( file_inode( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )) / H_oux_E_fs_S_sector_size - 1;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.pre = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.post = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table_n = 1;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
    {   if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
            error = -EIO;
            goto Error_6;
        }
        uint64_t i = H_oux_E_fs_Q_free_table_R( device_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            )
                i--;
            else
                while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                && ~--i
                ){} // Mogą być wpisy we fragmentach sektora powyżej w sektorze startowym, jeśli dla wyszukanego wpisu jest “!H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n”.
            if( !~i
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
            )
            {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_6;
            }
            if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
              || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
              ))
            && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
              || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
            ))) // Początkowe sektory (ewentualnie fragment poprzedniego (“pre”) sektora) z ostatnimi sektorami centralnych sektorów (ewentualnie fragmentem ostatniego (“post”) sektora) pozostają.
            {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                if( free_pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                {   pr_err( "free pre less than allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_6;
                }
                if( free_post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                {   pr_err( "free post less than allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_6;
                }
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_6;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                if( i + 2 < H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 2 ));
                if( free_sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - free_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = free_sector - 1;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                if( free_sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                  != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n =
                      free_sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                      - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n );
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = 0;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                }
            }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
              || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
              ))
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
            ) // Początkowe sektory oraz ewentualnie fragment poprzedniego (“pre”) sektora pozostają.
            {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                if( free_sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - free_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = free_sector - 1;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                }
            }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
            && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
              || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
            ))) // Ostatnie sektory centralnych sektorów z ewentualnym fragmentem ostatniego (“post”) sektora pozostają.
            {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                if( free_sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n =
                      free_sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                      - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n );
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = 0;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                }
            }else
            {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_6;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
            }
        }else
        {   while( ~--i
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            ){} // Przesuwanie na początkową pozycję do wyszukiwania wolnego bloku zawierającego blok pliku.
            if( ~i
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
            )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n >= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )
                    {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                        error = -EIO;
                        goto Error_6;
                    }
                    // Znaleziony wolny blok typu “sectors” o wcześniejszym ‘sektorze’ początkowym zawierający wycinany blok.
                    if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                    )) // Początkowy oraz końcowy fragment pozostają.
                    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_6;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                        if( i + 2 < H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 2 ));
                        uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                        uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start );
                        }
                        if( free_sector + free_sectors_n > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                        }else if( free_sector + free_sectors_n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                            && free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                            && !free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                            }else
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post;
                            }
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                        }
                    }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                    )) // Końcowy fragment pozostaje.
                    {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if( free_sector + free_sectors_n > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                        }else if( free_sector + free_sectors_n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                            && free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                            && !free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                            }else
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post;
                            }
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                    }else // Początkowy fragment pozostaje.
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                    }
                    continue;
                }
            }
            i++;
            if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_6;
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                  && H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                ))
                {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_6;
                }
                if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                  && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                    || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                ))
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                ) // Znaleziony wolny blok typu “sectors” o tym samym lub następnym ‘sektorze’ początkowym zawierający wycinany blok.
                {   if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                      )
                      && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1
                        || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == 1
                          && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                            || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size
                        ))
                        || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    ))
                    || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      && H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )) // Początkowy oraz końcowy fragment pozostają.
                    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_6;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                        if( i + 2 < H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 2 ));
                        uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                        uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                        if(( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && free_sectors_n > 1
                        )
                        || ( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && free_sectors_n
                        ))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sectors_n;
                            if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                        }else if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && free_sectors_n <= 1
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                        }
                    }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    && ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                    )
                    && (( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                      )
                      || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == 1
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
                    ))) // Początkowy fragment pozostaje.
                    {   uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                    }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                      && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                      && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    ))
                    || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      && H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )) // Końcowy fragment pozostaje.
                    {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1
                        )
                        || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        ))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n <= 1
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                    }else // Cały wolny blok znika.
                    {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_6;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                    }
                    continue;
                }
                i++;
                if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                )
                {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_6;
                }
            }
            while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Wyszukiwanie kandydata typu “in_sector” w tablicy wolnych bloków.
            {   i++;
                if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                )
                {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_6;
                }
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
            {   pr_err( "no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_6;
            }
            // Znaleziony wolny blok typu “in_sector” zawierający wycinany blok.
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Początkowy oraz końcowy fragment pozostają.
            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_6;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                if( i + 2 < H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 2 ));
                uint64_t free_size = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + free_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
            }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Początkowy fragment pozostaje.
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
            else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Końcowy fragment pozostaje
            {   uint64_t free_start = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
            }else // Cały wolny blok znika.
            {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_6;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
            }
        }
    }
    for( uint64_t i = 0; i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; i++ )
        pr_info( "%llu. sector: %llu, n: %llu, pre: %hu, post: %hu\n", i, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post );
    H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = ~0ULL;
    kfree(sector);
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return device_i;
Error_6:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].free_table );
Error_5:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
Error_4:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
Error_3:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
Error_2:
    kfree(sector);
Error_1:
    filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
    else
    {   unsigned device_i;
        for( device_i = H_oux_E_fs_Q_device_S_n - 2; ~device_i; device_i-- )
            if( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i;
    }
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
#undef H_oux_E_fs_Q_device_I_switch_item
#define H_oux_E_fs_Q_device_I_switch_item( type, item, end ) \
    if( data_i ) \
    {   do \
        {   *data++ = ( item >> data_i * 8 ) & 0xff; \
        }while( ++data_i != sizeof(type)); \
        data_i = 0; \
    }else \
    {   if( data + sizeof(type) > (end) ) \
        {   do \
            {   *data = ( item >> data_i++ * 8 ) & 0xff; \
            }while( ++data != (end) ); \
            break; \
        } \
        *( type * )data = item; \
        data += sizeof(type); \
    } \
    continue_from++
//------------------------------------------------------------------------------
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_W
, unsigned, device_i
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid )
        {   pr_err( "file locked: file_i=%llu\n", file_i );
            error = -EBUSY;
            goto Error_0;
        }
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    if( ~H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from ) // Zapis pierwszego sektora może być niezależny od tego, ale narazie powinno działać.
    {   strncpy( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident ) - 1 );
        uint64_t *block_table_n = H_oux_J_align_up_p( sector + sizeof( H_oux_E_fs_Q_device_S_ident ) - 1, uint64_t );
        block_table_n[0] = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
        block_table_n[1] = H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n;
        block_table_n[2] = H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start;
        block_table_n[3] = H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n;
        block_table_n[4] = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
        block_table_n[5] = H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start;
        block_table_n[6] = H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n;
        block_table_n[7] = H_oux_E_fs_Q_device_S[ device_i ].file_n;
        char *data = ( void * )&block_table_n[8];
        pr_info( "block_table_n: %llu, block_table_block_table_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_n, H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n );
        pr_info( "directory_table_start: %llu, directory_table_n: %llu, directory_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start, H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n, H_oux_E_fs_Q_device_S[ device_i ].directory_n );
        pr_info( "file_table_start: %llu, file_table_n: %llu, file_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start, H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n, H_oux_E_fs_Q_device_S[ device_i ].file_n );
        unsigned continue_from = 0;
        unsigned data_i = 0;
        uint64_t block_table_i = ~0ULL;
        while( data < sector + H_oux_E_fs_S_sector_size )
        {   switch( continue_from )
            { case 0:
                    block_table_i++;
                    if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                        goto End_loop;
                    pr_info( "sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
                    H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    , sector + H_oux_E_fs_S_sector_size
                    );
              default:
                    if( continue_from == 1 )
                    {   pr_info( "type: %u\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type );
                        H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                        , sector + H_oux_E_fs_S_sector_size
                        );
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        switch( continue_from )
                        { case 2:
                                pr_info( "n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n );
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                          pr_info( "pre: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre );
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 4:
                                pr_info( "post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                continue_from = 0;
                        }
                    else
                        switch( continue_from )
                        { case 2:
                                pr_info( "start: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start );
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                                pr_info( "size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                continue_from = 0;
                        }
            }
        }
End_loop:;
        loff_t offset = 0;
        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
        if( size != H_oux_E_fs_S_sector_size )
        {   pr_err( "write sector: 0\n" );
            error = -EIO;
            goto Error_1;
        }
        for( uint64_t block_table_i_write = 0; block_table_i_write != H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n; block_table_i_write++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case 0:
                                block_table_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_S_sector_size
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                            continue_from = 0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                            continue_from = 0;
                                    }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                block_table_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_S_sector_size
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                            continue_from = 0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_S_sector_size
                                            );
                                            continue_from = 0;
                                    }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector + sector_i );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                block_table_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            );
                                            continue_from = 0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            );
                                            continue_from = 0;
                                    }
                        }
                    }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector );
                    error = -EIO;
                    goto Error_1;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case 0:
                            block_table_i++;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector
                                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                );
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        );
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        );
                                        continue_from = 0;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        );
                                        continue_from = 0;
                                }
                    }
                }while( data < sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                );
                if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector );
                        error = -EIO;
                        goto Error_1;
                    }
                }
            }
        }
    }
    if( ~H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
    {   unsigned continue_from = 0;
        uint64_t directory_i = ~0ULL;
        unsigned data_i = 0;
        unsigned char_i;
        for( uint64_t directory_table_i = 0; directory_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n; directory_table_i++ )
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case 0:
                                directory_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                char_i = 0;
                          case 2:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_S_sector_size
                                );
                                if( data != sector + H_oux_E_fs_S_sector_size )
                                {   data++;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                directory_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                char_i = 0;
                          case 2:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_S_sector_size
                                );
                                if( data != sector + H_oux_E_fs_S_sector_size )
                                {   data++;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                      ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                directory_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                                );
                                char_i = 0;
                          case 2:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_S_sector_size
                                );
                                if( data != sector + H_oux_E_fs_S_sector_size )
                                {   data++;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                          ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                    error = -EIO;
                    goto Error_1;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case 0:
                            directory_i++;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
                              + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
                              + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                            );
                            char_i = 0;
                      case 2:
                            do
                            {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                            }while( *data
                            && ++data != sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size );
                            if( data != sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size )
                            {   data++;
                                continue_from = 0;
                            }
                    }
                }while( data < sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                );
                if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                        error = -EIO;
                        goto Error_1;
                    }
                }
            }
    }
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
    {   unsigned continue_from = 0;
        uint64_t file_i = ~0ULL;
        unsigned data_i = 0;
        unsigned char_i;
        for( uint64_t file_table_i = 0; file_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n; file_table_i++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case 0:
                                file_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                char_i = 0;
                          case 4:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_S_sector_size
                                );
                                if( data != sector + H_oux_E_fs_S_sector_size )
                                {   data++;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                file_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_S_sector_size
                                );
                                char_i = 0;
                          case 4:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_S_sector_size
                                );
                                if( data != sector + H_oux_E_fs_S_sector_size )
                                {   data++;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_S_sector_size );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                      ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                        error = -EIO;
                        goto Error_1;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                file_i++;
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                                char_i = 0;
                          case 4:
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                                {   data++;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                        }
                    }while( data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                          ) * H_oux_E_fs_S_sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size != H_oux_E_fs_S_sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                    error = -EIO;
                    goto Error_1;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case 0:
                            file_i++;
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                            char_i = 0;
                      case 4:
                            do
                            {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                            }while( *data
                            && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                            if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            )
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = 0;
                            }
                    }
                }while( data < sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                );
                if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                        error = -EIO;
                        goto Error_1;
                    }
                }
            }
        }
    }
    // Wyrzucenie z pamięci operacyjnej struktur systemu plików.
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
    filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
    else
    {   unsigned device_i;
        for( device_i = H_oux_E_fs_Q_device_S_n - 2; ~device_i; device_i-- )
            if( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_1;
        }
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i;
    }
Error_1:
    kfree(sector);
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
, uint64_t __user *, uid
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t parent_directory_i;
    if( ~parent )
    {   error = H_oux_E_fs_Q_directory_R( device_i, parent, &parent_directory_i );
        if(error)
            goto Error_0;
    }
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    if( !name_ )
    {   error = -ENOMEM;
        goto Error_0;
    }
    long n = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( n == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        kfree( name_ );
        goto Error_0;
    }
    void *p = krealloc( name_, n + 1, E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    name_ = p;
    p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, 2 * sizeof( uint64_t ) + n + 1
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
    );
    if(error)
    {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            kfree( name_ );
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].directory = p;
        kfree( name_ );
        goto Error_0;
    }
    uint64_t directory_i;
    uint64_t uid_;
    if( H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   uid_ = H_oux_E_fs_Q_device_S[ device_i ].directory[ H_oux_E_fs_Q_device_S[ device_i ].directory_n - 1 ].uid + 1;
        if( uid_ )
            directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
        else
        {   for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
            {   if( uid_ != H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                    break;
                uid_++;
            }
        }
    }else
    {   uid_ = 0;
        directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
    }
    if( directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        memmove( &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i + 1 ], &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ], ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - directory_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ));
    H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = uid_;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = ~parent ? H_oux_E_fs_Q_device_S[ device_i ].directory[ parent_directory_i ].uid : ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = name_;
    if( H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from > directory_i )
        H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = directory_i;
    put_user( uid_, uid );
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_directory_W
, unsigned, device_i
, uint64_t, uid
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    write_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    if( directory_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ], &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - ( directory_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, --H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
, uint64_t __user *, uid
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t directory_i;
    if( ~parent )
    {   error = H_oux_E_fs_Q_directory_R( device_i, parent, &directory_i );
        if(error)
            goto Error_0;
    }
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    if( !name_ )
    {   error = -ENOMEM;
        goto Error_0;
    }
    long n = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( n == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        kfree( name_ );
        goto Error_0;
    }
    void *p = krealloc( name_, n + 1, E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    name_ = p;
    p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, 4 * sizeof( uint64_t ) + n + 1
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
    );
    if(error)
    {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            kfree( name_ );
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].file = p;
        kfree( name_ );
        goto Error_0;
    }
    uint64_t uid_;
    uint64_t file_i;
    if( H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   uid_ = H_oux_E_fs_Q_device_S[ device_i ].file[ H_oux_E_fs_Q_device_S[ device_i ].file_n - 1 ].uid + 1;
        if( uid_ )
            file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n;
        else
        {   for( file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
            {   if( uid_ != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                    break;
                uid_++;
            }
        }
    }else
    {   uid_ = 0;
        file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n;
    }
    if( file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n )
        memmove( &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i + 1 ], &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ], ( H_oux_E_fs_Q_device_S[ device_i ].file_n - file_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ));
    H_oux_E_fs_Q_device_S[ device_i ].file_n++;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = uid_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = ~parent ? H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid : ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = 0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = name_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
    if( H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from > file_i )
        H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = file_i;
    put_user( uid_, uid );
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_file_W
, unsigned, device_i
, uint64_t, uid
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    write_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    )
    {   error = -EPERM;
        goto Error_0;
    }
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   uint64_t free_table_i = H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1;
        for( uint64_t block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n - 1; ~block_table_i; block_table_i-- )
        {   bool realloc_subtract, realloc_add;
            struct H_oux_E_fs_Z_block block = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ];
            //TODO Można zamiast tego tworzyć indeksy bloków wstawianych do tablicy wolnych bloków, i na końcu ‘realokować’ i przenosić dane. Tylko ze względu na “memmove”.
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table_n
            && free_table_i
            )
            {   free_table_i = H_oux_E_fs_Q_free_table_R_with_max( device_i, block.sector, free_table_i - 1 );
                uint64_t upper_block_table_i = ~0ULL;
                while( free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                )
                    free_table_i++;
                if( free_table_i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    free_table_i--;
                if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if(( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                          && block.location.sectors.post + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre == H_oux_E_fs_S_sector_size
                        )
                        || block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                        )
                        {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1 )
                                block.location.sectors.n++;
                            block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post;
                            upper_block_table_i = free_table_i;
                        }
                    }else
                    {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                        && block.location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                        )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size )
                                block.location.sectors.post += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                            else
                            {   block.location.sectors.n++;
                                block.location.sectors.post = 0;
                            }
                            upper_block_table_i = free_table_i;
                        }
                    }
                else
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                        && block.location.in_sector.start + block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre == H_oux_E_fs_S_sector_size
                        )
                        {   uint16_t start = block.location.in_sector.start;
                            uint16_t size = block.location.in_sector.size;
                            block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                            if(start)
                            {   block.sector++;
                                block.location.sectors.pre = size + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                            }else
                            {   block.location.sectors.n++;
                                block.location.sectors.pre = 0;
                            }
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post;
                            upper_block_table_i = free_table_i;
                        }
                    }else
                    {   while( free_table_i
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i - 1 ].sector == block.sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i - 1 ].location.in_sector.start > block.location.in_sector.start + block.location.in_sector.size
                        )
                            free_table_i--;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start == block.location.in_sector.start + block.location.in_sector.size
                        )
                        {   if( block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size )
                                block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                            else
                            {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                block.location.sectors.n = 1;
                                block.location.sectors.pre = block.location.sectors.post = 0;
                            }
                            upper_block_table_i = free_table_i;
                        }
                    }
                bool lower = no;
                while( ~free_table_i
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector >= block.sector
                )
                    free_table_i--;
                if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( ~free_table_i )
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector - 1
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                            )
                            {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n + 1;
                                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                                block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                                lower = yes;
                            }else if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].location.sectors.n == block.sector
                            )
                            {   free_table_i++;
                                block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                                block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                                lower = yes;
                            }
                        }else
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector - 1
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                            )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start )
                                    block.location.sectors.pre += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                                else
                                {   block.sector--;
                                    block.location.sectors.n++;
                                    block.location.sectors.pre = 0;
                                }
                                lower = yes;
                            }
                }else
                    if( ~free_table_i
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_S_sector_size
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n + 1;
                        block.location.sectors.post = 0;
                        lower = yes;
                    }else if( ~free_table_i
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector
                    && !block.location.in_sector.start
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.post = 0;
                        lower = yes;
                    }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].location.sectors.post == block.location.in_sector.start
                    )
                    {   free_table_i++;
                        uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + size != H_oux_E_fs_S_sector_size )
                        {   block.location.sectors.n = 0;
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + size;
                        }else
                        {   block.location.sectors.n = 1;
                            block.location.sectors.post = 0;
                        }
                        lower = yes;
                    }else if( ~free_table_i
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_in_sector
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        block.location.sectors.n = 0;
                        block.location.sectors.post = size;
                        lower = yes;
                    }else
                    {   while( ++free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size < block.location.in_sector.start
                        ){}
                        if( free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == block.location.in_sector.start
                        )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size + block.location.in_sector.size != H_oux_E_fs_S_sector_size )
                            {   block.location.in_sector.start -= H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                                block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                            }else
                            {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                block.location.sectors.n = 1;
                                block.location.sectors.pre = block.location.sectors.post = 0;
                            }
                            lower = yes;
                        }
                    }
                if( ~upper_block_table_i )
                    free_table_i = upper_block_table_i;
                if( ~upper_block_table_i
                && lower
                )
                    realloc_subtract = yes;
                else if( ~upper_block_table_i
                || lower
                )
                {   realloc_add = no;
                    realloc_subtract = no;
                }else
                {   realloc_add = yes;
                    realloc_subtract = no;
                    if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        free_table_i++;
                }
            }else
            {   realloc_add = yes;
                free_table_i = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
            }
            if( realloc_subtract )
            {   if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                {   for( uint64_t file_i_ = 0; file_i_ != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i_++ )
                        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start > H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i )
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start--;
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_i + 1 )
                    );
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_0;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                free_table_i--;
            }else if( realloc_add )
            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_0;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                {   memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i
                    , H_oux_E_fs_Q_device_S[ device_i ].block_table_n - 1 - free_table_i
                    );
                    for( uint64_t file_i_ = 0; file_i_ != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i_++ )
                        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i )
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start++;
                }
            }
            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ] = block;
        }
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
            , H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
            , H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + 1 )
            );
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n -= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    }
    if( file_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].file_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ], &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].file_n - ( file_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, --H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
