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
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_read
, unsigned, device_i
, uint64_t, uid
, uint64_t, pos
, uint64_t __user *, n
, char __user *, data
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
                        goto Loop_end;
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
Loop_end:
    if(pos)
        error = -EINVAL;
    put_user( data_p, n );
Error_1:
    kfree(sector);
Error_0:
    read_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
static
int
H_oux_E_fs_Q_block_table_I_unite( unsigned device_i
, uint64_t file_i
, uint64_t free_table_found_i
, uint64_t size_left
){  int error = 0;
    bool free_table_found_fit = !size_left;
    struct H_oux_E_fs_Z_block block = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ];
    if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
    {   if( block.location.sectors.post > size_left )
            block.location.sectors.post -= size_left;
        else
        {   size_left -= block.location.sectors.post;
            if( block.location.sectors.n * H_oux_E_fs_S_sector_size > size_left )
            {   block.location.sectors.post = size_left % H_oux_E_fs_S_sector_size;
                block.location.sectors.n -= size_left / H_oux_E_fs_S_sector_size;
                if( !block.location.sectors.n )
                {   uint64_t pre = block.location.sectors.pre;
                    block.sector--;
                    block.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    block.location.in_sector.start = H_oux_E_fs_S_sector_size - pre;
                    block.location.in_sector.size = pre;
                }
            }else
            {   size_left -= block.location.sectors.n * H_oux_E_fs_S_sector_size;
                uint64_t pre = block.location.sectors.pre;
                block.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                block.location.in_sector.start = H_oux_E_fs_S_sector_size - pre;
                block.location.in_sector.size = pre - size_left;
            }
        }
    }else
        block.location.in_sector.size -= size_left;
    uint64_t block_table_i = H_oux_E_fs_Q_block_table_R( device_i, file_i, block.sector );
    bool realloc_subtract, realloc_add;
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   uint64_t upper_block_table_i = ~0;
        if( free_table_found_fit )
        {   while( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
            )
                block_table_i++;
            if( block_table_i == H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
                block_table_i--;
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if(( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1
                      && block.location.sectors.post + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    || block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    )
                    {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1 )
                            block.location.sectors.n++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        upper_block_table_i = block_table_i;
                    }
                }else
                {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                    && block.location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size )
                            block.location.sectors.post += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        else
                        {   block.location.sectors.n++;
                            block.location.sectors.post = 0;
                        }
                        upper_block_table_i = block_table_i;
                    }
                }
            else
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1
                    && block.location.in_sector.start + block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   uint64_t start = block.location.in_sector.start;
                        uint64_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        if(start)
                        {   block.sector++;
                            block.location.sectors.pre = size + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        }else
                        {   block.location.sectors.n++;
                            block.location.sectors.pre = 0;
                        }
                        block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                        upper_block_table_i = block_table_i;
                    }
                }else
                {   while( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i - 1 ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i - 1 ].location.in_sector.start > block.location.in_sector.start + block.location.in_sector.size
                    )
                        block_table_i--;
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start == block.location.in_sector.start + block.location.in_sector.size
                    )
                    {   if( block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size )
                            block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        else
                        {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            block.location.sectors.n = 1;
                            block.location.sectors.pre = block.location.sectors.post = 0;
                        }
                        upper_block_table_i = block_table_i;
                    }
                }
        }
        bool lower = no;
        while( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start - 1
        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector >= block.sector
        )
            block_table_i--;
        if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start - 1 )
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                    }else if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location.sectors.n == block.sector
                    )
                    {   block_table_i++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                    }
                }else
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                            block.location.sectors.pre += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        else
                        {   block.sector--;
                            block.location.sectors.n++;
                            block.location.sectors.pre = 0;
                        }
                        lower = yes;
                    }
        }else
            if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start - 1
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_S_sector_size
            )
            {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                block.location.sectors.post = 0;
                lower = yes;
            }else if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start - 1
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector
            && !block.location.in_sector.start
            )
            {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                block.location.sectors.post = 0;
                lower = yes;
            }else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].sector == block.sector
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location.sectors.post == block.location.in_sector.start
            )
            {   block_table_i++;
                uint64_t size = block.location.in_sector.size;
                block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + size != H_oux_E_fs_S_sector_size )
                {   block.location.sectors.n = 0;
                    block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + size;
                }else
                {   block.location.sectors.n = 1;
                    block.location.sectors.post = 0;
                }
                lower = yes;
            }else if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start - 1
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_in_sector
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
            && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
            && !block.location.in_sector.start
            )
            {   uint64_t size = block.location.in_sector.size;
                block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                block.location.sectors.n = 0;
                block.location.sectors.post = size;
                lower = yes;
            }else
            {   while( ++block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size < block.location.in_sector.start
                ){}
                if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == block.location.in_sector.start
                )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size + block.location.in_sector.size != H_oux_E_fs_S_sector_size )
                    {   block.location.in_sector.start -= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                    }else
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.n = 1;
                        block.location.sectors.pre = block.location.sectors.post = 0;
                    }
                    lower = yes;
                }
            }
        if( ~upper_block_table_i )
            block_table_i = upper_block_table_i;
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
                block_table_i++;
        }
    }else
    {   realloc_add = yes;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start = block_table_i = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
    }
    if( realloc_subtract )
    {   if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
        {   for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
                if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start > block_table_i )
                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start--;
            memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( block_table_i + 1 ));
        }
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n--;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), GFP_KERNEL );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        block_table_i--;
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), GFP_KERNEL );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n++;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n++;
        if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
        {   memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table_n - 1 - block_table_i );
            for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
                if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start >= block_table_i )
                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start++;
        }
    }
    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ] = block;
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > block_table_i )
        H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = block_table_i;
    if( free_table_found_fit )
    {   if( free_table_found_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i, H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_found_i + 1 ));
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    }
    return error;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write
, unsigned, device_i
, uint64_t, uid
, uint64_t, pos
, uint64_t, n
, const char __user *, data
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
                uint64_t n__ = H_oux_J_min( n, block->location.sectors.pre - pos );
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
                    uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_S_sector_size - pos );
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
                uint64_t n__ = H_oux_J_min( n, block->location.sectors.post - pos );
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
                uint64_t n__ = H_oux_J_min( n, block->location.in_sector.size - pos );
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
    if(pos)
    {   pr_err( "H_oux_E_fs_Q_file_I_write: write past end of file: %llu\n", pos );
        error = -EIO;
        goto Error_1;
    }
    if(n)
    {   if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "H_oux_E_fs_Q_file_I_write: no space left on device: %llu\n", n );
            error = -ENOSPC;
            goto Error_1;
        }
        // Szukaj wolnego bloku na całe żądane dopisywane dane.
        uint64_t lowest_size = ~0;
        uint64_t size;
        uint64_t free_table_found_i;
        for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
        {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
              ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_S_sector_size
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
              : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
            if( size >= n
            && lowest_size > size
            )
            {   lowest_size = size;
                free_table_found_i = free_table_i;
                if( size == n )
                    break;
            }
        }
        if( ~lowest_size )
        {   error = H_oux_E_fs_Q_block_table_I_unite( device_i, file_i, free_table_found_i, lowest_size - size );
            if(error)
                goto Error_1;
            if( lowest_size != size )
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre > size )
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre -= size;
                    else
                    {   size -= H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector += size / H_oux_E_fs_S_sector_size;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n * H_oux_E_fs_S_sector_size > size )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre = size % H_oux_E_fs_S_sector_size ? H_oux_E_fs_S_sector_size - size % H_oux_E_fs_S_sector_size : 0;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n -= size / H_oux_E_fs_S_sector_size;
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector++;
                            if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n )
                            {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                            }
                        }else
                        {   size -= H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n * H_oux_E_fs_S_sector_size;
                            uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post - size;
                        }
                    }
                else
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start += size;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size -= size;
                }
        }else
        {   // Szukaj największych wolnych bloków mniejszych od żądanych dopisywanych danych.
            O{  if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                {   pr_err( "H_oux_E_fs_Q_file_I_write: no space left on device: %llu\n", n );
                    error = -ENOSPC;
                    goto Error_1;
                }
        		uint64_t greatest_size = 0;
                uint64_t size;
                uint64_t free_table_i;
                for( free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
                {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                      ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_S_sector_size
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                      : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                    if( greatest_size < size )
                    {   if( size > n )
                            break;
                        greatest_size = size;
                        free_table_found_i = free_table_i;
                    }
                }
                if( free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                {   uint64_t lowest_size = ~0;
                    for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
                    {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                          ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                            + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_S_sector_size
                            + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                          : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        if( size >= n
                        && lowest_size > size
                        )
                        {   lowest_size = size;
                            free_table_found_i = free_table_i;
                            if( size == n )
                                break;
                        }
                    }
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                        ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size_ != H_oux_E_fs_S_sector_size )
                        {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 );
                            error = -EIO;
                            goto Error_1;
                        }
                        uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre );
                        if( copy_from_user( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre ), data + data_p, n__ ) != n__ )
                        {   error = -EPERM;
                            goto Error_1;
                        }
                        size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size_ != H_oux_E_fs_S_sector_size )
                        {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - 1 );
                            error = -EIO;
                            goto Error_1;
                        }
                        data_p += n__;
                        n -= n__;
                        if( !n )
                        {   error = H_oux_E_fs_Q_block_table_I_unite( device_i, file_i, free_table_found_i, size - n__ );
                            if(error)
                                goto Error_1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre -= n__;
                            if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                            && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n
                            )
                            {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                            }
                            break;
                        }
                    }
                    for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n; sector_i++ )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                        uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_S_sector_size );
                        if( n__ != H_oux_E_fs_S_sector_size )
                        {   ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                            if( size != H_oux_E_fs_S_sector_size )
                            {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + sector_i );
                                error = -EIO;
                                goto Error_1;
                            }
                        }
                        if( copy_from_user( sector, data + data_p, n__ ) != n__ )
                        {   error = -EPERM;
                            goto Error_1;
                        }
                        ssize_t size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size_ != H_oux_E_fs_S_sector_size )
                        {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector - sector_i );
                            error = -EIO;
                            goto Error_1;
                        }
                        data_p += n__;
                        n -= n__;
                        if( !n )
                        {   uint64_t size_left = size
                              - ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                                + sector_i * H_oux_E_fs_S_sector_size
                                + n__
                                );
                            error = H_oux_E_fs_Q_block_table_I_unite( device_i, file_i, free_table_found_i, size_left );
                            if(error)
                                goto Error_1;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n -= sector_i + 1;
                            if( !size_left )
                                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post )
                                {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                                }else
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre = 0;
                            goto Loop_end;
                        }
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post )
                    {   loff_t offset = ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                        ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size_ != H_oux_E_fs_S_sector_size )
                        {   pr_err( "H_oux_E_fs_Q_file_I_write: read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n );
                            error = -EIO;
                            goto Error_1;
                        }
                        uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post );
                        if( copy_from_user( sector, data + data_p, n__ ) != n__ )
                        {   error = -EPERM;
                            goto Error_1;
                        }
                        size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                        if( size_ != H_oux_E_fs_S_sector_size )
                        {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n );
                            error = -EIO;
                            goto Error_1;
                        }
                        data_p += n__;
                        n -= n__;
                        if( !n )
                        {   uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                            error = H_oux_E_fs_Q_block_table_I_unite( device_i, file_i, free_table_found_i, post - n__ );
                            if(error)
                                goto Error_1;
                            if( post != n__ )
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                            }
                            break;
                        }
                    }
                }else
                {   loff_t offset = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector * H_oux_E_fs_S_sector_size;
                    ssize_t size_ = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size_ != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_file_I_read: read sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector );
                        error = -EIO;
                        goto Error_1;
                    }
                    uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size );
                    if( copy_from_user( sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start, data + data_p, n__ ) != n__ )
                    {   error = -EPERM;
                        goto Error_1;
                    }
                    size_ = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size_ != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_file_I_write: write sector: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].sector );
                        error = -EIO;
                        goto Error_1;
                    }
                    data_p += n__;
                    n -= n__;
                    if( !n )
                    {   error = H_oux_E_fs_Q_block_table_I_unite( device_i, file_i, free_table_found_i, size - n__ );
                        if(error)
                            goto Error_1;
                        if( size != n__ )
                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start += size;
                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size -= size;
                        }
                        break;
                    }
                }
            }
Loop_end:   ;
        }
    }
Error_1:
    kfree(sector);
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
