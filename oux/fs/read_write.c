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
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_lock, unsigned, device_i, uint64_t, uid, int, operation
){  if( device_i >= H_oux_E_fs_Q_device_S_n
    || ( operation != LOCK_SH
      && operation != LOCK_EX
      && operation != LOCK_UN
    ))
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
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_read, unsigned, device_i, uint64_t, uid, uint64_t, pos, uint64_t __user *, n, char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t n_;
    get_user( n_, n );
    if( !n_
    || !access_ok( data, n_ )
    )
        return -EINVAL;
    read_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read
    )
    {   error = -EPERM;
        goto Error_0;
    }
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, GFP_KERNEL );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    uint64_t data_p = 0;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i;
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( pos < block->location.sectors.pre )
            {   loff_t offset = ( block->sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.pre );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + ( H_oux_E_fs_S_sector_size - block->location.sectors.pre ) + pos, n__ ) != n__ )
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
            {   if( pos < H_oux_E_fs_S_sector_size )
                {   loff_t offset = ( block->sector + sector_i ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", block->sector + sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    uint64_t n__ = H_oux_J_min( n_, H_oux_E_fs_S_sector_size );
                    if( n__ > pos )
                        n__ -= pos;
                    if( copy_to_user( data + data_p, sector + pos, n__ ) != n__ )
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    data_p += n__;
                    n_ -= n__;
                    if( !n_ )
                        break;
                    pos = 0;
                }else
                    pos -= H_oux_E_fs_S_sector_size;
            }
            if( pos < block->location.sectors.post )
            {   loff_t offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.sectors.post );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + pos, n__ ) != n__ )
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
            {   loff_t offset = block->sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n_, block->location.in_sector.size );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_to_user( data + data_p, sector + block->location.in_sector.start + pos, n__ ) != n__ )
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
    put_user( data_p, n );
Error_1:
    kfree(sector);
Error_0:
    read_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write, unsigned, device_i, uint64_t, uid, uint64_t, pos, uint64_t, n, const char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n
    || !n
    || !access_ok( data, n )
    )
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
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, GFP_KERNEL );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    uint64_t data_p = 0;
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = H_oux_E_fs_Q_device_S[ device_i ].block_table + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i;
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( pos < block->location.sectors.pre )
            {   loff_t offset = ( block->sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n, block->location.sectors.pre );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_from_user( sector + ( H_oux_E_fs_S_sector_size - block->location.sectors.pre ) + pos, data + data_p, n__ ) != n__ )
                {   error = -EPERM;
                    goto Error_1;
                }
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", block->sector - 1 );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n -= n__;
                if( !n )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.pre;
            for( uint64_t sector_i = 0; sector_i != block->location.sectors.n; sector_i++ )
            {   if( pos < H_oux_E_fs_S_sector_size )
                {   loff_t offset = ( block->sector + sector_i ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", block->sector + sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_S_sector_size );
                    if( n__ > pos )
                        n__ -= pos;
                    if( copy_from_user( sector + pos, data + data_p, n__ ) != n__ )
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", block->sector - sector_i );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n -= n__;
                    if( !n )
                        break;
                    pos = 0;
                }else
                    pos -= H_oux_E_fs_S_sector_size;
            }
            if( pos < block->location.sectors.post )
            {   loff_t offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n, block->location.sectors.post );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_from_user( sector + pos, data + data_p, n__ ) != n__ )
                {   error = -EPERM;
                    goto Error_1;
                }
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", block->sector + block->location.sectors.n );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n -= n__;
                if( !n )
                    break;
                pos = 0;
            }else
                pos -= block->location.sectors.post;
        }else
            if( pos < block->location.in_sector.size )
            {   loff_t offset = block->sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                uint64_t n__ = H_oux_J_min( n, block->location.in_sector.size );
                if( n__ > pos )
                    n__ -= pos;
                if( copy_from_user( sector + block->location.in_sector.start + pos, data + data_p, n__ ) != n__ )
                {   error = -EPERM;
                    goto Error_1;
                }
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", block->sector );
                    error = -EIO;
                    goto Error_1;
                }
                data_p += n__;
                n -= n__;
                if( !n )
                    break;
                pos = 0;
            }else
                pos -= block->location.in_sector.size;
    }
    if(n)
    {   //TODO Wyszukać wolny blok i dopisać na końcu pliku.
        
    }
Error_1:
    kfree(sector);
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
