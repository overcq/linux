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
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
//------------------------------------------------------------------------------
#include "../lang.h"
#include "fs.h"
//==============================================================================
extern struct rw_semaphore E_oux_E_fs_S_rw_lock;
extern void *H_oux_E_fs_Q_device_S_holder;
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
#define H_oux_E_fs_Q_device_I_switch_item( type, item, end ) \
    if( data_i ) \
    {   do \
        {   item |= (type)*data++ << data_i++ * 8; \
        }while( data_i != sizeof(type) \
        && data != (end) \
        ); \
        if( data_i == sizeof(type) ) \
            data_i = 0; \
        else \
            break; \
    }else \
    {   if( data == (end) ) \
            break; \
        if( data + sizeof(type) > (end) ) \
        {   item = 0; \
            do \
            {   item |= (type)*data++ << data_i++ * 8; \
            }while( data != (end) ); \
            break; \
        } \
        item = *( type * )data; \
        data += sizeof(type); \
    } \
    continue_from++
//------------------------------------------------------------------------------
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_M
, const char __user *, pathname
){  const unsigned max_path = 4096, default_sector_size = 4096;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    char *pathname_ = kmalloc( max_path, E_oux_E_fs_S_alloc_flags );
    int error;
    long count = strncpy_from_user( pathname_, pathname, max_path );
    if( count == -EFAULT )
    {   error = -EFAULT;
        kfree( pathname_ );
        goto Error_0;
    }
    if( count == max_path )
    {   error = -ENAMETOOLONG;
        kfree( pathname_ );
        goto Error_0;
    }
    unsigned device_i;
    for( device_i = 0; device_i != H_oux_E_fs_Q_device_S_n; device_i++ )
        if( !H_oux_E_fs_Q_device_S[ device_i ].bdev_file )
            break;
    if( device_i == H_oux_E_fs_Q_device_S_n )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S, H_oux_E_fs_Q_device_S_n + 1, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n++;
    }
    struct file *bdev_file = bdev_file_open_by_path( pathname_, BLK_OPEN_READ | BLK_OPEN_WRITE | BLK_OPEN_EXCL, H_oux_E_fs_Q_device_S_holder, 0 );
    kfree( pathname_ );
    if( IS_ERR( bdev_file ))
    {   error = PTR_ERR( bdev_file );
        goto Error_1;
    }
    H_oux_E_fs_Q_device_S[ device_i ].bdev_file = bdev_file;
    char *sector = kmalloc( default_sector_size, E_oux_E_fs_S_alloc_flags );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_2;
    }
    loff_t offset = 0;
    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, default_sector_size, &offset );
    if( size != default_sector_size )
    {   pr_err( "read sector: 0\n" );
        error = -EIO;
        goto Error_3;
    }
    if( strncmp( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident )))
    {   pr_err( "no filesystem identification string\n" );
        error = -EIO;
        goto Error_3;
    }
    unsigned sector_size_bit = sector[ sizeof( H_oux_E_fs_Q_device_S_ident ) ];
    if( sector_size_bit != 9
    && sector_size_bit != 12
    )
    {   pr_err( "sector size other than 512 or 4096\n" );
        error = -EIO;
        goto Error_3;
    }
    H_oux_E_fs_Q_device_S[ device_i ].sector_size = 1 << sector_size_bit;
    if( H_oux_E_fs_Q_device_S[ device_i ].sector_size != default_sector_size )
    {   void *p = krealloc( sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
        if( !sector )
        {   error = -ENOMEM;
            goto Error_3;
        }
        sector = p;
    }
    uint64_t *block_table_n = H_oux_J_align_up_p( sector + sizeof( H_oux_E_fs_Q_device_S_ident ) + 1, uint64_t );
    H_oux_E_fs_Q_device_S[ device_i ].block_table_n = block_table_n[0];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n = block_table_n[1];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start = block_table_n[2];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n = block_table_n[3];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start = block_table_n[4];
    H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n = block_table_n[5];
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
        goto Error_3;
    }
    char *data = ( void * )&block_table_n[6];
    H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( uint16_t )(( char * )&block_table_n[6] - sector );
    void *p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_3;
    }
    H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    H_oux_E_fs_Q_device_S[ device_i ].block_table_size = 0;
    // Odczyt tablicy bloków do pamięci operacyjnej.
    unsigned continue_from = ~0;
    unsigned data_i = 0;
    uint64_t block_table_i = ~0ULL;
    do // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
    {   switch( continue_from )
        { case ~0:
          case 0:
                if( !~continue_from )
                {   block_table_i++;
                    if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                        goto End_loop;
                    continue_from = 0;
                }
                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                );
                if( continue_from == 1 )
                    H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
          default:
                if( continue_from == 1 )
                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                    , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    );
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                    )
                    {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                        error = -EIO;
                        goto Error_4;
                    }
                    H_oux_E_fs_Q_device_S[ device_i ].block_table_size++;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    switch( continue_from )
                    { case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 3 )
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 4 )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                            }
                      case 4:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 5 )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                  || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                ))
                                {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                continue_from = ~0;
                            }
                    }
                else
                    switch( continue_from )
                    { case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 3 )
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 4 )
                            {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                  > H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                )
                                {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                continue_from = ~0;
                            }
                    }
        }
    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
End_loop:
    for( uint64_t block_table_i_read = 0; block_table_i_read != H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n; block_table_i_read++ ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
    {   if( block_table_i_read > block_table_i ) //NDFN Przemyśleć i zagwarantować, by zawsze starczało.
        {   pr_err( "read beyond available: block_table_i=%llu\n", block_table_i );
            error = -EIO;
            goto Error_4;
        }
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector - 1 );
                    error = -EIO;
                    goto Error_4;
                }
                char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre );
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   block_table_i++;
                                if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                                {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                                )
                                {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 4 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 5 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                              || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            ))
                                            {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 4 )
                                        {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                            {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                              > H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            )
                                            {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + sector_i );
                    error = -EIO;
                    goto Error_4;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   block_table_i++;
                                if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                                {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector
                                )
                                {   pr_err( "location type unknown: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 4 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 5 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                              || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            ))
                                            {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        );
                                        if( continue_from == 4 )
                                        {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                            {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                              > H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            )
                                            {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n );
                    error = -EIO;
                    goto Error_4;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   block_table_i++;
                                if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                                {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                    error = -EIO;
                                    goto Error_4;
                                }
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                            );
                            if( continue_from == 1 )
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
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
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table_size++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( continue_from == 4 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                        }
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( continue_from == 5 )
                                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                            {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                              || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            ))
                                            {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( continue_from == 3 )
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post
                                        );
                                        if( continue_from == 4 )
                                        {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                            {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                              > H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            )
                                            {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                                error = -EIO;
                                                goto Error_4;
                                            }
                                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                            continue_from = ~0;
                                        }
                                }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
            if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector );
                error = -EIO;
                goto Error_4;
            }
            char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start;
            do // Czyta wpisy pliku tablicy bloków znajdujące się we fragmencie sektora.
            {   switch( continue_from )
                { case ~0:
                  case 0:
                        if( !~continue_from )
                        {   block_table_i++;
                            if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                            {   pr_err( "too much blocks size: block_table_i=%llu\n", block_table_i );
                                error = -EIO;
                                goto Error_4;
                            }
                            continue_from = 0;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                        );
                        if( continue_from == 1 )
                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
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
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].block_table_size++;
                        }
                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                            switch( continue_from )
                            { case 2:
                                    H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( continue_from == 3 )
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint64_t );
                              case 3:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( continue_from == 4 )
                                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                        {   pr_err( "pre not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_4;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                    }
                              case 4:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( continue_from == 5 )
                                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                        {   pr_err( "post not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_4;
                                        }
                                        if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                          || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        ))
                                        {   pr_err( "sectors and pre or post 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_4;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                        continue_from = ~0;
                                    }
                            }
                        else
                            switch( continue_from )
                            { case 2:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( continue_from == 3 )
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                              case 3:
                                    H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                    , sector
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
                                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
                                    );
                                    if( continue_from == 4 )
                                    {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size )
                                        {   pr_err( "in sector size 0: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_4;
                                        }
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                          > H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                        )
                                        {   pr_err( "in sector not less than sector: block_table_i=%llu\n", block_table_i );
                                            error = -EIO;
                                            goto Error_4;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table_size += sizeof( uint16_t );
                                        continue_from = ~0;
                                    }
                            }
                }
            }while( data != sector
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size
            );
        }
    }
    if( ~continue_from )
    {   pr_err( "too few blocks size\n" );
        error = -EIO;
        goto Error_4;
    }
    //for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
    //{   pr_info( "block_table_i: %llu, sector: %llu\n", block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        //if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            //pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
        //else
            //pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
    //}
    // Utworzenie tablicy wolnych bloków.
    p = kmalloc_array( 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_4;
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].sector = 1;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.n = i_size_read( file_inode( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )) / H_oux_E_fs_Q_device_S[ device_i ].sector_size - 1;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.pre = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.post = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table_n = 1;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
    {   /*for( uint64_t i = 0; i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; i++ )
        {   pr_info( "%llu. sector: %llu\n", i, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector );
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post );
            else
                pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size );
        }
        pr_info( "block %llu. sector: %llu\n", block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );*/
        if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "(1) no free block for allocated: block_table_i=%llu\n", block_table_i );
            error = -EIO;
            goto Error_5;
        }
        uint64_t i = H_oux_E_fs_Q_free_table_R( device_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            )
                i--;
            else
            {   while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                && ~--i
                ){} // Mogą być wpisy we fragmentach sektora powyżej, jeśli dla wyszukanego wpisu jest “!H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n”.
                if( !~i
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                  < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                  == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                  && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                ))
                    i++;
            }
            if( !~i
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
            )
            {   pr_err( "(2) no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_5;
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
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                )
                {   pr_err( "free pre less than allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                  == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                )
                {   pr_err( "free post less than allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_5;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                    , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                    );
                uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector -  H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        )
                        {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = post;
                        }
                    }else
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n =
                  free_sector + free_sectors_n
                  - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector;
                if( free_sector + free_sectors_n
                  != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n--;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n
                    || free_post
                    )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector++;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                    }else
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                    }
                }else if( free_sector + free_sectors_n
                  != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = 0;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                    }else
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    }
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                }
            }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
              || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
              ))
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
            ) // Początkowe sektory oraz ewentualnie fragment poprzedniego (“pre”) sektora pozostają.
            {   uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector -  H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        )
                        {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = post;
                        }
                    }else
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                }
            }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
            && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
              || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
            )) // Ostatnie sektory centralnych sektorów z ewentualnym fragmentem ostatniego (“post”) sektora pozostają.
            {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n =
                  free_sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                  - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                if( free_sector + free_sectors_n
                  != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                    || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                    )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector++;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    }else
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                    }
                }else if( free_sector + free_sectors_n
                  != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n )
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = 0;
                    else
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    }
                }else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                }
            }else
            {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                    , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                    );
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_5;
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
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                  && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post >= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                ))
                {   // Znaleziony wolny blok typu “sectors” o wcześniejszym ‘sektorze’ początkowym zawierający wycinany blok.
                    if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                    )) // Początkowy oraz końcowy fragment pozostają.
                    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                        if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2
                            , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                            );
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
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start );
                        }
                        if( free_sector + free_sectors_n > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                        }else if( free_sector + free_sectors_n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            && free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            && !free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
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
                      || H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )
                    && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                      || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                    )) // Końcowy fragment pozostaje.
                    {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if( free_sector + free_sectors_n > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                        }else if( free_sector + free_sectors_n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            && free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = free_sector + free_sectors_n - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 );
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            && !free_post
                            )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
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
            {   pr_err( "(3) no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_5;
            }
Next_sector:if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                  && H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                ))
                {   pr_err( "(4) no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_5;
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
                            || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        ))
                        || ( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                          && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    )))
                    || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      && H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )) // Początkowy oraz końcowy fragment pozostają.
                    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                        if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2
                            , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                            );
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
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                        if(( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && ( free_sectors_n > 1
                            || ( free_sectors_n == 1
                              && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        )))
                        || ( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && ( free_sectors_n
                            || ( free_post
                              && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        ))))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sectors_n;
                            if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                        }else if(( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && free_sectors_n == 1
                        )
                        || ( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && free_post
                        ))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post;
                        }else if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
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
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size
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
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
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
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - free_pre;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                    }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                      && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                      && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                      && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                        || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                    ))
                    || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                      && H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )) // Końcowy fragment pozostaje.
                    {   uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                        uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                        if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1
                            || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == 1
                              && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        )))
                        || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                            || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        ))))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            if( free_sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                        }else if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                          && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == 1
                        )
                        || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1
                          && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                        ))
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post;
                        }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                        }
                    }else // Cały wolny blok znika.
                    {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i
                            , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                            );
                        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                        if( !p )
                        {   error = -ENOMEM;
                            goto Error_5;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                    }
                    continue;
                }
                i++;
                if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        goto Next_sector;
                    pr_err( "(5) no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
            }
            while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Wyszukiwanie kandydata typu “in_sector” w tablicy wolnych bloków.
            {   i++;
                if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + 1 )
                        goto Next_sector;
                    pr_err( "(6) no free block for allocated: block_table_i=%llu\n", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
            {   pr_err( "(7) no free block for allocated: block_table_i=%llu\n", block_table_i );
                error = -EIO;
                goto Error_5;
            }
            // Znaleziony wolny blok typu “in_sector” zawierający wycinany blok.
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
              > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
            ) // Początkowy oraz końcowy fragment pozostają.
            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_5;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 )
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                    , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                    );
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
                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i
                    , H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1
                    , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
                    );
                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_5;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
            }
        }
        for( uint64_t j = 0; j != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; j++ )
        {   if( j == i )
                continue;
            if( H_oux_E_fs_Q_block_T_cross( device_i
            , &H_oux_E_fs_Q_device_S[ device_i ].free_table[i]
            , &H_oux_E_fs_Q_device_S[ device_i ].free_table[j]
            )) // Sprawdzenie na czas testów.
            {   pr_err( "cross: i=%llu, j=%llu\n", i, j );
                /*for( uint64_t i = 0; i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; i++ )
                {   pr_info( "%llu. sector: %llu\n", i, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector );
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post );
                    else
                        pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size );
                }*/
                goto Error_5;
            }
        }
    }
    // Odczyt tablicy katalogów do pamięci operacyjnej.
    p = kmalloc_array( 0, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_5;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    H_oux_E_fs_Q_device_S[ device_i ].directory_n = 0;
    uint64_t directory_i = 0;
    uint64_t uid_last = ~0ULL;
    data_i = 0;
    unsigned char_i;
    for( uint64_t directory_table_i = 0; directory_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n; directory_table_i++ )
    {   /*pr_info( "directory_table_start: %llu, directory_table_i: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start, directory_table_i );
        pr_info( "sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size );*/
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                    error = -EIO;
                    goto Error_6;
                }
                char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre );
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory = p;
                                directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                                {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            }
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 2 )
                            {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                char_i = 0;
                            }
                      case 2:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_6;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n; sector_i++ )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i );
                    error = -EIO;
                    goto Error_6;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory = p;
                                directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                                {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            }
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 2 )
                            {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                char_i = 0;
                            }
                      case 2:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_6;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                    );
                    error = -EIO;
                    goto Error_6;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory = p;
                                directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                                {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                    error = -EIO;
                                    goto Error_6;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                            }
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                            );
                            if( continue_from == 2 )
                            {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                char_i = 0;
                            }
                      case 2:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                            {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                        error = -ENOMEM;
                                        goto Error_6;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            }else
                            {   data++;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
            if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                error = -EIO;
                goto Error_6;
            }
            char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start;
            do
            {   switch( continue_from )
                { case ~0:
                  case 0:
                        if( !~continue_from )
                        {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_6;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory = p;
                            directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
                            continue_from = 0;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        );
                        if( continue_from == 1 )
                        {   if( !~H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                            {   pr_err( "uid empty: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_6;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: directory_i=%llu\n", directory_i );
                                error = -EIO;
                                goto Error_6;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
                        }
                  case 1:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        );
                        if( continue_from == 2 )
                        {   p = kmalloc( sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                              - data
                            , E_oux_E_fs_S_alloc_flags
                            );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_6;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                        }
                  case 2:
                        while( data != sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data;
                            if( !*data )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_6;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                                break;
                            }
                            data++;
                        }
                        if( data == sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                        )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                error = -ENOMEM;
                                goto Error_6;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        }else
                        {   data++;
                            continue_from = ~0;
                        }
                }
            }while( data != sector
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
            );
        }
    }
    if( ~continue_from )
    {   pr_err( "too few blocks size\n" );
        error = -EIO;
        goto Error_6;
    }
    //pr_info( "directories:\n" );
    //for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        //pr_info( "%llu. uid: %llu, parent: %llu, name: %s\n", directory_i, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
    // Odczyt tablicy plików do pamięci operacyjnej.
    p = kmalloc_array( 0, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_6;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    H_oux_E_fs_Q_device_S[ device_i ].file_n = 0;
    uint64_t file_i = 0;
    uid_last = ~0ULL;
    data_i = 0;
    for( uint64_t file_table_i = 0; file_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n; file_table_i++ )
    {   /*pr_info( "file_table_start: %llu, file_table_i: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start, file_table_i );
        pr_info( "sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size );*/
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                    error = -EIO;
                    goto Error_7;
                }
                char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre );
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file = p;
                                file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                                {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            }
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 4 )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                && !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                )
                                {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                                {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                char_i = 0;
                            }
                      case 4:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_7;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n; sector_i++ )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i );
                    error = -EIO;
                    goto Error_7;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file = p;
                                file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                                {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            }
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            );
                            if( continue_from == 4 )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                && !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                )
                                {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                                {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                char_i = 0;
                            }
                      case 4:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_7;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
            {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                    error = -EIO;
                    goto Error_7;
                }
                char *data = sector;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file = p;
                                file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                            );
                            if( continue_from == 1 )
                            {   if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                                {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( ~uid_last
                                && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                )
                                {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                            }
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
                            if( continue_from == 4 )
                            {   if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                && !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                )
                                {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                                {   pr_err( "block count empty: file_i=%llu\n", file_i );
                                    error = -EIO;
                                    goto Error_7;
                                }
                                p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post - data, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                char_i = 0;
                            }
                      case 4:
                            while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                                if( !*data )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_7;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                    break;
                                }
                                data++;
                            }
                            if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   data++;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
            }
        }else
        {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
            if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
            {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                error = -EIO;
                goto Error_7;
            }
            char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start;
            do
            {   switch( continue_from )
                { case ~0:
                  case 0:
                        if( !~continue_from )
                        {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_7;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file = p;
                            file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n++;
                            continue_from = 0;
                        }
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                        if( continue_from == 1 )
                        {   if( !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                            {   pr_err( "uid empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_7;
                            }
                            if( ~uid_last
                            && uid_last >= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            )
                            {   pr_err( "uid not sorted or duplicate: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_7;
                            }
                            uid_last = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
                        }
                  case 1:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                  case 2:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                  case 3:
                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        );
                        if( continue_from == 4 )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            && !~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            )
                            {   pr_err( "block start empty: file_i=%llu\n", file_i );
                                error = -EIO;
                                goto Error_7;
                            }
                            p = kmalloc( sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size - data
                            , E_oux_E_fs_S_alloc_flags
                            );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_7;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                        }
                  case 4:
                        while( data != sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data;
                            if( !*data )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, E_oux_E_fs_S_alloc_flags );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_7;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                break;
                            }
                            data++;
                        }
                        if( data == sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                        )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                error = -ENOMEM;
                                goto Error_7;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        }else
                        {   data++;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                            continue_from = ~0;
                        }
                }
            }while( data != sector
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
            );
        }
    }
    if( ~continue_from )
    {   pr_err( "too few blocks size\n" );
        error = -EIO;
        goto Error_7;
    }
    //pr_info( "files:\n" );
    //for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        //pr_info( "%llu. uid: %llu, parent: %llu, name: %s, lock_pid: %d, lock_read: %d\n", file_i, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read );
    H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = ~0ULL;
    H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = ~0ULL;
    kfree(sector);
    up_write( &E_oux_E_fs_S_rw_lock );
    return device_i;
Error_7:
    for( uint64_t file_i_ = 0; file_i_ != file_i; file_i_++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
Error_6:
    for( uint64_t directory_i_ = 0; directory_i_ != directory_i; directory_i_++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i_ ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
Error_5:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].free_table );
Error_4:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
Error_3:
    kfree(sector);
Error_2:
    filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
Error_1:
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
    else
    {   unsigned device_i_;
        for( device_i_ = H_oux_E_fs_Q_device_S_n - 2; ~device_i_; device_i_-- )
            if( H_oux_E_fs_Q_device_S[ device_i_ ].bdev_file )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i_, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
            error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i_;
    }
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
#undef H_oux_E_fs_Q_device_I_switch_item
#define H_oux_E_fs_Q_device_I_switch_item( type, item, end ) \
    if( data_i ) \
    {   do \
        {   *data++ = ( item >> data_i++ * 8 ) & 0xff; \
        }while( data_i != sizeof(type) \
        && data != (end) \
        ); \
        if( data_i == sizeof(type) ) \
            data_i = 0; \
        else \
            break; \
    }else \
    {   if( data == (end) ) \
            break; \
        if( data + sizeof(type) > (end) ) \
        {   do \
            {   *data++ = ( item >> data_i++ * 8 ) & 0xff; \
            }while( data != (end) ); \
            break; \
        } \
        *( type * )data = item; \
        data += sizeof(type); \
    } \
    continue_from++
//------------------------------------------------------------------------------
int
H_oux_E_fs_Q_device_I_save( unsigned device_i
){  char *sector = kzalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
    if( !sector )
        return -ENOMEM;
    int error = 0;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
    {   strcpy( sector, H_oux_E_fs_Q_device_S_ident );
        sector[ sizeof( H_oux_E_fs_Q_device_S_ident ) ] = __builtin_ctz( H_oux_E_fs_Q_device_S[ device_i ].sector_size );
        uint64_t *block_table_n = H_oux_J_align_up_p( sector + sizeof( H_oux_E_fs_Q_device_S_ident ) + 1, uint64_t );
        block_table_n[0] = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
        block_table_n[1] = H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n;
        block_table_n[2] = H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start;
        block_table_n[3] = H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n;
        block_table_n[4] = H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start;
        block_table_n[5] = H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n;
        char *data = ( void * )&block_table_n[6];
        pr_info( "block_table_n: %llu, block_table_block_table_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_n, H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n );
        pr_info( "directory_table_start: %llu, directory_table_n: %llu, directory_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start, H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n, H_oux_E_fs_Q_device_S[ device_i ].directory_n );
        pr_info( "file_table_start: %llu, file_table_n: %llu, file_n: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start, H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n, H_oux_E_fs_Q_device_S[ device_i ].file_n );
        /*for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
        {   pr_info( "block_table_i: %llu, sector: %llu\n", block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
            else
                pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
        }*/
        unsigned continue_from = ~0;
        unsigned data_i = 0;
        uint64_t block_table_i = ~0ULL;
        do
        {   switch( continue_from )
            { case ~0:
              case 0:
                    if( !~continue_from )
                    {   block_table_i++;
                        if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
                            goto End_loop;
                        continue_from = 0;
                    }
                    H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    );
              default:
                    if( continue_from == 1 )
                    {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                        , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        );
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        switch( continue_from )
                        { case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 4:
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 5 )
                                    continue_from = ~0;
                        }
                    else
                        switch( continue_from )
                        { case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 4 )
                                    continue_from = ~0;
                        }
            }
        }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
End_loop:;
        loff_t offset = 0;
        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
        {   pr_err( "write sector: 0\n" );
            error = -EIO;
            goto Error_0;
        }
        for( uint64_t block_table_i_write = 0; block_table_i_write != H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n; block_table_i_write++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   block_table_i++;
                                    if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n ) // Sprawdzenie na czas testów.
                                    {   pr_err( "block table overflow\n" );
                                        error = -EIO;
                                        goto Error_0;
                                    }
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                            if( continue_from == 5 )
                                                continue_from = ~0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                            if( continue_from == 4 )
                                                continue_from = ~0;
                                    }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector - 1 );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   block_table_i++;
                                    if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n ) // Sprawdzenie na czas testów.
                                    {   pr_err( "block table overflow\n" );
                                        error = -EIO;
                                        goto Error_0;
                                    }
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                            if( continue_from == 5 )
                                                continue_from = ~0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                            );
                                            if( continue_from == 4 )
                                                continue_from = ~0;
                                    }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector + sector_i );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n
                    ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n
                        );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   block_table_i++;
                                    if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n ) // Sprawdzenie na czas testów.
                                    {   pr_err( "block table overflow\n" );
                                        error = -EIO;
                                        goto Error_0;
                                    }
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                );
                          default:
                                if( continue_from == 1 )
                                {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                    , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                    );
                                }
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                            );
                                      case 4:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                            );
                                            if( continue_from == 5 )
                                                continue_from = ~0;
                                    }
                                else
                                    switch( continue_from )
                                    { case 2:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                            );
                                      case 3:
                                            H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post
                                            );
                                            if( continue_from == 4 )
                                                continue_from = ~0;
                                    }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post );
                    if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n
                        ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n
                            );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector );
                    error = -EIO;
                    goto Error_0;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   block_table_i++;
                                if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].block_table_n ) // Sprawdzenie na czas testów.
                                {   pr_err( "block table overflow\n" );
                                    error = -EIO;
                                    goto Error_0;
                                }
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                            );
                      default:
                            if( continue_from == 1 )
                            {   H_oux_E_fs_Q_device_I_switch_item( char, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type
                                , sector
                                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                );
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                        );
                                  case 4:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                        );
                                        if( continue_from == 5 )
                                            continue_from = ~0;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                        );
                                  case 3:
                                        H_oux_E_fs_Q_device_I_switch_item( uint16_t, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                        , sector
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                                        );
                                        if( continue_from == 4 )
                                            continue_from = ~0;
                                }
                    }
                }while( data != sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size
                );
                if( block_table_i >= H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector );
                        error = -EIO;
                        goto Error_0;
                    }
                }
            }
        }
        if( ~continue_from ) // Sprawdzenie na czas testów.
        {   pr_err( "block table underflow\n" );
            error = -EIO;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = ~0ULL;
    }
    if( ~H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
    {   unsigned continue_from = ~0;
        uint64_t directory_i = ~0ULL;
        unsigned data_i = 0;
        unsigned char_i;
        for( uint64_t directory_table_i = 0; directory_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n; directory_table_i++ )
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   directory_i++;
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 2 )
                                    char_i = 0;
                          case 2:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector - 1 );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   directory_i++;
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 2 )
                                    char_i = 0;
                          case 2:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector + sector_i );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                      ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                        );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   directory_i++;
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                                );
                                if( continue_from == 2 )
                                    char_i = 0;
                          case 2:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.post );
                    if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                          ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.sectors.n
                            );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                    error = -EIO;
                    goto Error_0;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   directory_i++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
                              + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
                              + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                            );
                            if( continue_from == 2 )
                                char_i = 0;
                      case 2:
                            if( data == sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                            )
                                break;
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
                                continue_from = ~0;
                            }
                    }
                }while( data != sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].location.in_sector.size
                );
                if( directory_i >= H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start + directory_table_i ].sector );
                        error = -EIO;
                        goto Error_0;
                    }
                }
            }
        H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = ~0ULL;
    }
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
    {   unsigned continue_from = ~0;
        uint64_t file_i = ~0ULL;
        unsigned data_i = 0;
        unsigned char_i;
        for( uint64_t file_table_i = 0; file_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n; file_table_i++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.pre );
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   file_i++;
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 4 )
                                    char_i = 0;
                          case 4:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector - 1 );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n; sector_i++ )
                {   char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   file_i++;
                                    continue_from = 0;
                                }
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( continue_from == 4 )
                                    char_i = 0;
                          case 4:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + sector_i );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                      ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                        error = -EIO;
                        goto Error_0;
                    }
                    char *data = sector;
                    do
                    {   switch( continue_from )
                        { case ~0:
                          case 0:
                                if( !~continue_from )
                                {   file_i++;
                                    continue_from = 0;
                                }
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
                                if( continue_from == 4 )
                                    char_i = 0;
                          case 4:
                                if( data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                                    break;
                                do
                                {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                                }while( *data
                                && ++data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post
                                );
                                if( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post )
                                {   data++;
                                    continue_from = ~0;
                                }
                        }
                    }while( data != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.post );
                    if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n
                          ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.sectors.n );
                            error = -EIO;
                            goto Error_0;
                        }
                    }
                }
            }else
            {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                    error = -EIO;
                    goto Error_0;
                }
                char *data = sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start;
                do
                {   switch( continue_from )
                    { case ~0:
                      case 0:
                            if( !~continue_from )
                            {   file_i++;
                                continue_from = 0;
                            }
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( uint64_t, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                            if( continue_from == 4 )
                                char_i = 0;
                      case 4:
                            if( data == sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            )
                                break;
                            do
                            {   *data = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ];
                            }while( *data
                            && ++data != sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            );
                            if( data != sector
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                              + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                            )
                            {   data++;
                                continue_from = ~0;
                            }
                    }
                }while( data != sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.start
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].location.in_sector.size
                );
                if( file_i >= H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from )
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start + file_table_i ].sector );
                        error = -EIO;
                        goto Error_0;
                    }
                }
            }
        }
        H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = ~0ULL;
    }
Error_0:
    kfree(sector);
    return error;
}
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_W
, unsigned, device_i
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid )
        {   pr_err( "file locked: file_i=%llu\n", file_i );
            error = -EBUSY;
            goto Error_0;
        }
    error = H_oux_E_fs_Q_device_I_save( device_i );
    if(error)
        goto Error_0;
    // Wyrzucenie z pamięci operacyjnej struktur systemu plików.
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
    kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
    filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
    else
    {   unsigned device_i_;
        for( device_i_ = H_oux_E_fs_Q_device_S_n - 2; ~device_i_; device_i_-- )
            if( H_oux_E_fs_Q_device_S[ device_i_ ].bdev_file )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i_, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
            error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i_;
    }
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_I_sync
, unsigned, device_i
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    error = H_oux_E_fs_Q_device_I_save( device_i );
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
, uint64_t __user *, uid
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    if( !~H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    if( ~parent )
    {   uint64_t parent_directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, parent, &parent_directory_i );
        if(error)
            goto Error_0;
    }
    char *name_ = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
    if( !name_ )
    {   error = -ENOMEM;
        goto Error_0;
    }
    long n = strncpy_from_user( name_, name, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
    if( n == -EFAULT )
    {   error = -EFAULT;
        kfree( name_ );
        goto Error_0;
    }
    if( !n )
    {   error = -EINVAL;
        kfree( name_ );
        goto Error_0;
    }
    if( n == H_oux_E_fs_Q_device_S[ device_i ].sector_size )
    {   error = -ENAMETOOLONG;
        kfree( name_ );
        goto Error_0;
    }
    if( n + 1 != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
    {   void *p = krealloc( name_, n + 1, E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   error = -ENOMEM;
            kfree( name_ );
            goto Error_0;
        }
        name_ = p;
    }
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, name_ )
        )
        {   error = -EEXIST;
            kfree( name_ );
            goto Error_0;
        }
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, name_ )
        )
        {   error = -EEXIST;
            kfree( name_ );
            goto Error_0;
        }
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, 2 * sizeof( uint64_t ) + n + 1
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
    , &H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from
    );
    if(error)
    {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
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
        if( ~uid_ )
            directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
        else
        {   uid_ = 0;
            for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
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
        memmove( H_oux_E_fs_Q_device_S[ device_i ].directory + directory_i + 1
        , H_oux_E_fs_Q_device_S[ device_i ].directory + directory_i
        , ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - directory_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory )
        );
    H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = uid_;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = parent;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = name_;
    if( H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from > directory_i )
        H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = directory_i;
    error = put_user( uid_, uid );
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_directory_W
, unsigned, device_i
, uint64_t, uid
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    uint64_t directory_i;
    error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    if( directory_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ], &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - ( directory_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, --H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
, uint64_t __user *, uid
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    if( !~H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    if( ~parent )
    {   uint64_t parent_directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, parent, &parent_directory_i );
        if(error)
            goto Error_0;
    }
    char *name_ = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
    if( !name_ )
    {   error = -ENOMEM;
        goto Error_0;
    }
    long n = strncpy_from_user( name_, name, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
    if( n == -EFAULT )
    {   error = -EFAULT;
        kfree( name_ );
        goto Error_0;
    }
    if( !n )
    {   error = -EINVAL;
        kfree( name_ );
        goto Error_0;
    }
    if( n == H_oux_E_fs_Q_device_S[ device_i ].sector_size )
    {   error = -ENAMETOOLONG;
        kfree( name_ );
        goto Error_0;
    }
    if( n + 1 != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
    {   void *p = krealloc( name_, n + 1, E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        name_ = p;
    }
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, name_ )
        )
        {   error = -EEXIST;
            kfree( name_ );
            goto Error_0;
        }
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, name_ )
        )
        {   error = -EEXIST;
            kfree( name_ );
            goto Error_0;
        }
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, 4 * sizeof( uint64_t ) + n + 1
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
    , &H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from
    );
    if(error)
    {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
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
        if( ~uid_ )
            file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n;
        else
        {   uid_ = 0;
            for( file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
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
        memmove( H_oux_E_fs_Q_device_S[ device_i ].file + file_i + 1
        , H_oux_E_fs_Q_device_S[ device_i ].file + file_i
        , ( H_oux_E_fs_Q_device_S[ device_i ].file_n - file_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file )
        );
    H_oux_E_fs_Q_device_S[ device_i ].file_n++;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = uid_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = parent;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = 0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = name_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
    if( H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from > file_i )
        H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = file_i;
    error = put_user( uid_, uid );
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_file_W
, unsigned, device_i
, uint64_t, uid
){  int error = 0;
    if( down_write_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    uint64_t file_i;
    error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    )
    {   error = -EPERM;
        goto Error_0;
    }
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start;
        for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
        {   error = H_oux_E_fs_Q_free_table_I_unite( device_i, &H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ] );
            if(error)
                goto Error_0;
        }
        memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
        , H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
        , ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table )
        );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start > H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start -= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start > H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start -= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        for( uint64_t file_i_ = 0; file_i_ != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i_++ )
            if( file_i_ != file_i
            && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start > H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
            )
                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].block_table.start -= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n -= H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    }
    if( file_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].file_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ], &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].file_n - ( file_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, --H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), E_oux_E_fs_S_alloc_flags );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
