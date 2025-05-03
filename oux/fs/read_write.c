/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         read and write
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
extern rwlock_t E_oux_E_fs_S_rw_lock;
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_lock
, unsigned, device_i
, uint64_t, uid
, int, operation
){  if( operation != LOCK_SH
    && operation != LOCK_EX
    && operation != LOCK_UN
    )
        return -EINVAL;
    int error = 0;
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
    if( operation == LOCK_SH )
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = current->pid;
    else if( operation == LOCK_EX )
    {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = current->pid;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = yes;
    }else
    {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
    }
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_read
, unsigned, device_i
, uint64_t, uid
, uint64_t, pos
, uint64_t __user *, n
, char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t n_;
    int error = get_user( n_, n );
    if(error)
        goto Error_0;
    if( !n_
    || !access_ok( data, n_ )
    )
        return -EINVAL;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    uint64_t file_i;
    error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read
    )
    {   error = -EPERM;
        goto Error_0;
    }
    char *sector = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    uint64_t data_p = 0;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i;
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( pos < block->location.sectors.pre )
            {   loff_t offset = ( block->sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.pre );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - block->location.sectors.pre ) + pos, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.pre;
            for( uint64_t sector_i = 0; sector_i != block->location.sectors.n; sector_i++ )
            {   if( pos < H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   loff_t offset = ( block->sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", block->sector + sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    uint64_t n__ = H_oux_J_min( n_, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    if( n__ > pos )
                        n__ -= pos;
                    if( copy_to_user( data + data_p, sector + pos, n__ ))
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                        goto Loop_end;
                    pos = 0;
                }else
                    pos -= H_oux_E_fs_Q_device_S[ device_i ].sector_size;
            }
            if( pos < block->location.sectors.post )
            {   loff_t offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.post );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + pos, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.post;
        }else
            if( pos < block->location.in_sector.size )
            {   loff_t offset = block->sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.in_sector.size );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + block->location.in_sector.start + pos, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.in_sector.size;
    }
Loop_end:
    if(pos)
        error = -EINVAL;
    error = put_user( data_p, n );
Error_1:
    kfree(sector);
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write
, unsigned, device_i
, uint64_t, uid
, uint64_t, pos
, uint64_t __user *, n
, const char __user *, data
){  uint64_t n_;
    int error = get_user( n_, n );
    if(error)
        return -EPERM;
    if( !n_
    || !access_ok( data, n_ )
    )
        return -EINVAL;
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
    char *sector = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    uint64_t data_p = 0;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i;
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( pos < block->location.sectors.pre )
            {   loff_t offset = ( block->sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.pre - pos );
                if( copy_from_user( sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - block->location.sectors.pre ) + pos, data + data_p, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                offset = ( block->sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "write sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.pre;
            for( uint64_t sector_i = 0; sector_i != block->location.sectors.n; sector_i++ )
            {   if( pos < H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   loff_t offset = ( block->sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", block->sector + sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    uint64_t n__ = H_oux_J_min( n_, H_oux_E_fs_Q_device_S[ device_i ].sector_size - pos );
                    if( copy_from_user( sector + pos, data + data_p, n__ ))
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    offset = ( block->sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", block->sector - sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                        break;
                    pos = 0;
                }else
                    pos -= H_oux_E_fs_Q_device_S[ device_i ].sector_size;
            }
            if( pos < block->location.sectors.post )
            {   loff_t offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.post - pos );
                if( copy_from_user( sector + pos, data + data_p, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "write sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.post;
        }else
            if( pos < block->location.in_sector.size )
            {   loff_t offset = block->sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.in_sector.size - pos );
                if( copy_from_user( sector + block->location.in_sector.start + pos, data + data_p, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                offset = block->sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "write sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                    break;
                pos = 0;
            }else
                pos -= block->location.in_sector.size;
    }
    if(pos)
    {   pr_err( "write past end of file: %llu\n", pos );
        error = -EIO;
        goto Error_1;
    }
    if( n_ )
    {   if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "no space left on device: %llu\n", n_ );
            error = -ENOSPC;
            goto Error_1;
        }
        uint64_t n_0 = n_;
        int64_t block_table_diff = 0;
        uint64_t size;
        uint64_t free_table_found_i;
        uint64_t block_n = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
        if( block_n )
        {   // Szukaj wolnego bloku przyległego od góry do ostatniego przydzielonego.
            uint64_t block_start = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start;
            uint64_t free_table_i = H_oux_E_fs_Q_free_table_R( device_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector + 1 );
            if( free_table_i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                free_table_i--;
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                for( ; ~free_table_i; free_table_i-- )
                {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                      ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                      : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                      < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.n
                    )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.n
                            == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                          && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.post
                            + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                            == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        )
                        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.n
                            == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                          && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.post
                          && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        ))
                        {   free_table_found_i = free_table_i;
                            goto Write;
                        }
                    }else
                    {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.n
                            == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                          && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                        )
                        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.n
                            == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                          && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.sectors.post
                          && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                        ))
                        {   free_table_found_i = free_table_i;
                            goto Write;
                        }
                    }
                }
            else
            {   while( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector + 1
                && free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1
                )
                    free_table_i++;
                for( ; ~free_table_i; free_table_i-- )
                {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                      ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                      : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                        && ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.start
                          + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.size
                          == H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        ))
                        {   free_table_found_i = free_table_i;
                            goto Write;
                        }
                    }else
                    {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                          && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.start
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.size
                            == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                        )
                        || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                          && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.start
                            + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_n - 1 ].location.in_sector.size
                            == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                          && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                        ))
                        {   free_table_found_i = free_table_i;
                            goto Write;
                        }
                    }
                }
            }
        }
        // Szukaj wolnego bloku na całe żądane dopisywane dane.
        uint64_t lowest_size = ~0ULL;
        for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
        {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
              ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
              : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
            if( size >= n_
            && lowest_size > size
            )
            {   lowest_size = size;
                free_table_found_i = free_table_i;
                if( size == n_ )
                    break;
            }
        }
        if( ~lowest_size )
        {   size = lowest_size;
            goto Write;
        }
        O{  if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            {   pr_err( "no space left on device: %llu\n", n_ );
                error = -ENOSPC;
                goto Error_1;
            }
            // Szukaj największego wolnego bloku mniejszego od żądanych dopisywanych danych.
            uint64_t greatest_size = 0;
            uint64_t free_table_i;
            for( free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
            {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                  ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                  : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                if( greatest_size < size )
                {   if( size >= n_ )
                        break;
                    greatest_size = size;
                    free_table_found_i = free_table_i;
                }
            }
            if( free_table_i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                size = greatest_size;
            else
            {   // Szukaj wolnego bloku na całe żądane dopisywane dane.
                uint64_t lowest_size = ~0ULL;
                for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
                {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                      ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                      : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                    if( size >= n_
                    && lowest_size > size
                    )
                    {   lowest_size = size;
                        free_table_found_i = free_table_i;
                        if( size == n_ )
                            break;
                    }
                }
                size = lowest_size;
            }
Write:      uint64_t n__;
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
                {   n__ = H_oux_J_min( n_, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre );
                    loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 );
                        error = -EIO;
                        goto Error_1;
                    }
                    if( copy_from_user( sector + ( H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre ), data + data_p, n__ ))
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                    {   int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , free_table_found_i, size - n__
                        , &block_table_diff
                        );
                        if( error_ >= 0 )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre -= n__;
                            if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                            && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n
                            )
                            {   uint16_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                            }
                        }
                        if( error_ )
                            error = error_;
                        if( error_ < 0 )
                        {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                            , n_0 - ( n_ + n__ )
                            , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , &block_table_diff
                            );
                            if( error_ )
                                error = error_;
                        }
                        break;
                    }
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n; sector_i++ )
                {   n__ = H_oux_J_min( n_, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                    loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    if( n__ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                        if( size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + sector_i );
                            error = -EIO;
                            goto Error_1;
                        }
                    }
                    if( copy_from_user( sector, data + data_p, n__ ))
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + sector_i ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                    {   uint64_t size_left = size
                          - ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                            + sector_i * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                            + n__
                            );
                        int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , free_table_found_i, size_left
                        , &block_table_diff
                        );
                        if( error_ >= 0
                        && size_left
                        )
                        {   uint16_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector += sector_i + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n -= sector_i + 1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - n__;
                            if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n )
                                if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post )
                                {   uint16_t pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector--;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = pre;
                                }else if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                                }
                        }
                        if( error_ )
                            error = error_;
                        if( error_ < 0 )
                        {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                            , n_0 - ( n_ + n__ )
                            , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , &block_table_diff
                            );
                            if( error_ )
                                error = error_;
                        }
                        goto Loop_end;
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post )
                {   n__ = H_oux_J_min( n_, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post );
                    uint16_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                    loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n );
                        error = -EIO;
                        goto Error_1;
                    }
                    if( copy_from_user( sector, data + data_p, n__ ))
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n ) * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                    if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                    {   int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , free_table_found_i, post - n__
                        , &block_table_diff
                        );
                        if( error_ >= 0
                        && post != n__
                        )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = n__;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start;
                        }
                        if( error_ )
                            error = error_;
                        if( error_ < 0 )
                        {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                            , n_0 - ( n_ + n__ )
                            , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , &block_table_diff
                            );
                            if( error_ )
                                error = error_;
                        }
                        break;
                    }
                }
            }else
            {   n__ = H_oux_J_min( n_, size );
                loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector );
                    error = -EIO;
                    goto Error_1;
                }
                if( copy_from_user( sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start, data + data_p, n__ ))
                {   error = -EPERM;
                    goto Error_1;
                }
                offset = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_Q_device_S[ device_i ].sector_size, &offset );
                if( size_ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                {   pr_err( "write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n_ -= n__;
                if( !n_ )
                {   int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                    , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                    , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                    , free_table_found_i, size - n__
                    , &block_table_diff
                    );
                    if( error_ >= 0
                    && size != n__
                    )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start += n__;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size -= n__;
                    }
                    if( error_ )
                        error = error_;
                    if( error_ < 0 )
                    {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                        , n_0 - ( n_ + n__ )
                        , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                        , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                        , &block_table_diff
                        );
                        if( error_ )
                            error = error_;
                    }
                    break;
                }
            }
            int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
            , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
            , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
            , free_table_found_i, 0, &block_table_diff
            );
            if( error_ )
                error = error_;
            if( error_ < 0 )
            {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                , n_0 - ( n_ + n__ )
                , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                , &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                , &block_table_diff
                );
                if( error_ )
                    error = error_;
                break;
            }
        }
Loop_end:
        int error_ = H_oux_E_fs_Q_block_table_I_append_truncate( device_i, block_table_diff );
        if( error_ )
            error = error_;
        if( error > 0 )
            error = -error;
        if(error)
            goto Error_1;
    }
    error = put_user( data_p, n );
Error_1:
    kfree(sector);
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
