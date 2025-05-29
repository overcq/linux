/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         common
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/rwsem.h>
#include <linux/string.h>
#include <linux/syscalls.h>
//------------------------------------------------------------------------------
#include "../lang.h"
#include "fs.h"
//==============================================================================
extern struct rw_semaphore E_oux_E_fs_S_rw_lock;
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
bool
H_oux_E_fs_Q_block_T_cross( unsigned device_i
, const struct H_oux_E_fs_Z_block *block_1
, const struct H_oux_E_fs_Z_block *block_2
){  if( block_1->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        if( block_2->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if(( block_2->sector >= block_1->sector
              && block_2->sector < block_1->sector + block_1->location.sectors.n
            )
            || ( block_2->sector + block_2->location.sectors.n >= block_1->sector
              && block_2->sector + block_2->location.sectors.n
                < block_1->sector + block_1->location.sectors.n
            )
            || ( block_2->sector == block_1->sector - 1
              && block_2->location.sectors.post + block_1->location.sectors.pre > H_oux_E_fs_Q_device_S[ device_i ].sector_size
            )
            || ( block_1->sector == block_2->sector - 1
              && block_1->location.sectors.post + block_2->location.sectors.pre > H_oux_E_fs_Q_device_S[ device_i ].sector_size
            ))
                return yes;
        }else
        {   if(( block_2->sector == block_1->sector - 1
              && block_2->location.in_sector.start + block_2->location.in_sector.size
                > H_oux_E_fs_Q_device_S[ device_i ].sector_size - block_1->location.sectors.pre
            )
            || ( block_2->sector >= block_1->sector
              && block_2->sector < block_1->sector + block_1->location.sectors.n
            )
            || ( block_2->sector == block_1->sector + block_1->location.sectors.n
              && block_2->location.in_sector.start < block_1->location.sectors.post
            ))
                return yes;
        }
    else
        if( block_2->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if(( block_1->sector == block_2->sector - 1
              && block_1->location.in_sector.start + block_1->location.in_sector.size
                > H_oux_E_fs_Q_device_S[ device_i ].sector_size - block_2->location.sectors.pre
            )
            || ( block_1->sector >= block_2->sector
              && block_1->sector < block_2->sector + block_2->location.sectors.n
            )
            || ( block_1->sector == block_2->sector + block_2->location.sectors.n
              && block_1->location.in_sector.start < block_2->location.sectors.post
            ))
                return yes;
        }else
        {   if( block_2->sector == block_1->sector
            && (( block_2->location.in_sector.start >= block_1->location.in_sector.start
                && block_2->location.in_sector.start < block_1->location.in_sector.start + block_1->location.in_sector.size
              )
              || ( block_2->location.in_sector.start + block_2->location.in_sector.size > block_1->location.in_sector.start
                && block_2->location.in_sector.start + block_2->location.in_sector.size
                  <= block_1->location.in_sector.start + block_1->location.in_sector.size
            )))
                return yes;
        }
    return no;
}
//------------------------------------------------------------------------------
uint64_t
H_oux_E_fs_Z_start_n_R_size( unsigned device_i
, uint64_t block_start
, uint64_t block_n
){  uint64_t size = 0;
    for( uint64_t block_table_i = 0; block_table_i != block_n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = &H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_table_i ];
        size += block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
        ? block->location.sectors.pre + block->location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size + block->location.sectors.post
        : block->location.in_sector.size;
    }
    return size;
}
uint64_t
H_oux_E_fs_Q_free_table_R( unsigned device_i
, uint64_t sector
){  uint64_t min = 0;
    uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1;
    uint64_t free_table_i = min + ( max + 1 - min ) / 2;
    O{  if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == sector )
            break;
        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector > sector )
        {   if( free_table_i == min )
                break;
            max = free_table_i - 1;
            free_table_i = max - ( free_table_i - min ) / 2;
        }else
        {   if( free_table_i == max )
            {   free_table_i++; // Przesuń na “H_oux_E_fs_Q_device_S[ device_i ].free_table[ block_table_i ].sector > sector” lub poza zakres tablicy bloków.
                break;
            }
            min = free_table_i + 1;
            free_table_i = min + ( max - free_table_i ) / 2;
        }
    }
    return free_table_i;
}
int
H_oux_E_fs_Q_directory_R( unsigned device_i
, uint64_t uid
, uint64_t *directory_i
){  if( !H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        return -ENOENT;
    uint64_t min = 0;
    uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].directory_n - 1;
    uint64_t directory_i_ = ( max + 1 ) / 2;
    O{  if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i_ ].uid == uid )
            break;
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i_ ].uid > uid )
        {   if( directory_i_ == min )
                return -ENOENT;
            max = directory_i_ - 1;
            directory_i_ = max - ( directory_i_ - min ) / 2;
        }else
        {   if( directory_i_ == max )
                return -ENOENT;
            min = directory_i_ + 1;
            directory_i_ = min + ( max - directory_i_ ) / 2;
        }
    }
    *directory_i = directory_i_;
    return 0;
}
int
H_oux_E_fs_Q_file_R( unsigned device_i
, uint64_t uid
, uint64_t *file_i
){  if( !H_oux_E_fs_Q_device_S[ device_i ].file_n )
        return -ENOENT;
    uint64_t min = 0;
    uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].file_n - 1;
    uint64_t file_i_ = ( max + 1 ) / 2;
    O{  if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].uid == uid )
            break;
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i_ ].uid > uid )
        {   if( file_i_ == min )
                return -ENOENT;
            max = file_i_ - 1;
            file_i_ = max - ( file_i_ - min ) / 2;
        }else
        {   if( file_i_ == max )
                return -ENOENT;
            min = file_i_ + 1;
            file_i_ = min + ( max - file_i_ ) / 2;
        }
    }
    *file_i = file_i_;
    return 0;
}
//------------------------------------------------------------------------------
int
H_oux_E_fs_Q_block_table_I_unite( unsigned device_i
, uint64_t *block_start
, uint64_t *block_n
, uint64_t free_table_found_i
, uint64_t size_left
, int64_t *block_table_diff
){  bool free_table_found_fit = !size_left;
    struct H_oux_E_fs_Z_block block = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ];
    pr_info( "size_left: %llu\n", size_left );
    pr_info( "free block: type: %u, sector: %llu\n", block.location_type, block.sector );
    if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        pr_info( "n: %llu, pre: %hu, post: %hu\n", block.location.sectors.n, block.location.sectors.pre, block.location.sectors.post );
    else
        pr_info( "start: %hu, size: %hu\n", block.location.in_sector.start, block.location.in_sector.size );
    if( size_left )
        if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( block.location.sectors.post > size_left )
                block.location.sectors.post -= size_left;
            else
            {   size_left -= block.location.sectors.post;
                if( block.location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size > size_left )
                {   block.location.sectors.n -= size_left / H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    if( size_left % H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                    {   block.location.sectors.n--;
                        block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].sector_size - size_left % H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                        if( !block.location.sectors.n
                        && !block.location.sectors.pre
                        )
                        {   uint16_t post = block.location.sectors.post;
                            block.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            block.location.in_sector.start = 0;
                            block.location.in_sector.size = post;
                        }
                    }else
                    {   block.location.sectors.post = 0;
                        if( !block.location.sectors.n )
                        {   uint16_t pre = block.location.sectors.pre;
                            block.sector--;
                            block.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            block.location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                            block.location.in_sector.size = pre - size_left;
                        }
                    }
                }else
                {   size_left -= block.location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size;
                    uint16_t pre = block.location.sectors.pre;
                    block.sector--;
                    block.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    block.location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                    block.location.in_sector.size = pre - size_left;
                }
            }
        }else
            block.location.in_sector.size -= size_left;
    pr_info( "block 1: type: %u, sector: %llu\n", block.location_type, block.sector );
    if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        pr_info( "n: %llu, pre: %hu, post: %hu\n", block.location.sectors.n, block.location.sectors.pre, block.location.sectors.post );
    else
        pr_info( "start: %hu, size: %hu\n", block.location.in_sector.start, block.location.in_sector.size );
    uint64_t block_table_i;
    uint64_t upper_block_table_i = ~0ULL;
    bool realloc_subtract, realloc_add;
    if( *block_n )
    {   uint64_t block_start_ = block_start ? *block_start : 0;
        if( free_table_found_fit )
            for( uint64_t block_table_i = block_start_; block_table_i != block_start_ + *block_n; block_table_i++ )
                if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if(( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1
                          && block.location.sectors.post + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        )
                        || block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        )
                        {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1 )
                                block.location.sectors.n++;
                            block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                            upper_block_table_i = block_table_i;
                            break;
                        }
                    }else
                    {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        && block.location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                        )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                block.location.sectors.post += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            else
                            {   block.location.sectors.n++;
                                block.location.sectors.post = 0;
                            }
                            upper_block_table_i = block_table_i;
                            break;
                        }
                    }
                else
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1
                        && block.location.in_sector.start + block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        )
                        {   uint16_t start = block.location.in_sector.start;
                            uint16_t size = block.location.in_sector.size;
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
                            break;
                        }
                    }else
                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                        && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start == block.location.in_sector.start + block.location.in_sector.size
                        )
                        {   if( block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                                block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            else
                            {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                block.location.sectors.n = 1;
                                block.location.sectors.pre = block.location.sectors.post = 0;
                            }
                            upper_block_table_i = block_table_i;
                            break;
                        }
        for( block_table_i = block_start_; block_table_i != block_start_ + *block_n; block_table_i++ )
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        break;
                    }else if( block_table_i + 1 != block_start_ + *block_n
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location.sectors.n == block.sector
                    )
                    {   block_table_i++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        break;
                    }
                }else
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size + block.location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                            block.location.sectors.pre += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        else
                        {   block.sector--;
                            block.location.sectors.n++;
                            block.location.sectors.pre = 0;
                        }
                        break;
                    }
            }else
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    {   block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.post = 0;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post == block.location.in_sector.start
                    )
                    {   block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + size;
                        else
                        {   block.location.sectors.n++;
                            block.location.sectors.post = 0;
                        }
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        block.location.sectors.n = 0;
                        block.location.sectors.post = size;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == block.location.in_sector.start
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size + block.location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   block.location.in_sector.start -= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                            block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        }else
                        {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            block.location.sectors.n = 1;
                            block.location.sectors.pre = block.location.sectors.post = 0;
                        }
                        break;
                    }
                }
        pr_info( "block 2: type: %u, sector: %llu\n", block.location_type, block.sector );
        if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", block.location.sectors.n, block.location.sectors.pre, block.location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", block.location.in_sector.start, block.location.in_sector.size );
        if( ~upper_block_table_i
        && block_table_i != block_start_ + *block_n
        )
            realloc_subtract = yes;
        else if( ~upper_block_table_i
        || block_table_i != block_start_ + *block_n
        )
        {   if( ~upper_block_table_i )
                block_table_i = upper_block_table_i;
            realloc_subtract = no;
            realloc_add = no;
        }else
        {   realloc_subtract = no;
            realloc_add = yes;
        }
    }else
    {   realloc_subtract = no;
        realloc_add = yes;
        if( block_start )
            *block_start = block_table_i = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
        else
            block_table_i = 0;
    }
    int error = 0;
    if( realloc_subtract )
    {   *block_table_diff -= sizeof( uint64_t ) + 1
        + ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
          : sizeof( uint16_t ) + sizeof( uint16_t )
        );
        if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
        {   memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i
            , H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1
            , ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( block_table_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table )
            );
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start > block_table_i )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start--;
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start > block_table_i )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start--;
            for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
                if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start > block_table_i )
                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start--;
        }
        (*block_n)--;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n--;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > block_table_i )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = block_table_i;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
        if(p)
            H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        else
            error = ENOMEM;
        block_table_i = upper_block_table_i > block_table_i ? upper_block_table_i - 1 : upper_block_table_i;
        *block_table_diff -= sizeof( uint64_t ) + 1
        + ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
          : sizeof( uint16_t ) + sizeof( uint16_t )
        );
        *block_table_diff += sizeof( uint64_t ) + 1
        + ( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
          : sizeof( uint16_t ) + sizeof( uint16_t )
        );
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
        if( !p )
            return -ENOMEM;
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n++;
        (*block_n)++;
        if( block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n - 1 )
        {   memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1
            , H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i
            , ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - 1 - block_table_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table )
            );
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start >= block_table_i )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start++;
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start >= block_table_i )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start++;
            for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
                if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start >= block_table_i )
                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start++;
        }
        *block_table_diff += sizeof( uint64_t ) + 1
        + ( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
          : sizeof( uint16_t ) + sizeof( uint16_t )
        );
    }
    pr_info( "block 3: type: %u, sector: %llu\n", block.location_type, block.sector );
    if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        pr_info( "n: %llu, pre: %hu, post: %hu\n", block.location.sectors.n, block.location.sectors.pre, block.location.sectors.post );
    else
        pr_info( "start: %hu, size: %hu\n", block.location.in_sector.start, block.location.in_sector.size );
    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ] = block;
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > block_table_i )
        H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = block_table_i;
    if( free_table_found_fit )
    {   if( free_table_found_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i
            , H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i + 1
            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_found_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
            );
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
        if(p)
            H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
        else
            error = ENOMEM;
    }
    return error;
}
int
H_oux_E_fs_Q_free_table_I_unite( unsigned device_i
, const struct H_oux_E_fs_Z_block *block_p
){  struct H_oux_E_fs_Z_block block = *block_p;
    uint64_t free_table_i;
    bool realloc_subtract, realloc_add;
    if( H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
    {   uint64_t upper_free_table_i = ~0ULL;
        if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   uint64_t free_table_i = H_oux_E_fs_Q_free_table_R( device_i, block.sector + block.location.sectors.n );
            for( ; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
            {   if( block.sector + block.location.sectors.n > H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector )
                    break;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if(( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                      && block.location.sectors.post + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    || block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                    )
                    {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1 )
                            block.location.sectors.n++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post;
                        upper_free_table_i = free_table_i;
                        break;
                    }
                }else
                {   if( block.sector + block.location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                    && block.location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            block.location.sectors.post += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        else
                        {   block.location.sectors.n++;
                            block.location.sectors.post = 0;
                        }
                        upper_free_table_i = free_table_i;
                        break;
                    }
                }
            }
        }else
        {   uint64_t free_table_i = H_oux_E_fs_Q_free_table_R( device_i, block.sector );
            for( ; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
            {   if( block.sector > H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector )
                    break;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                    && block.location.in_sector.start + block.location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
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
                        upper_free_table_i = free_table_i;
                        break;
                    }
                }else
                {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                    && block.location.in_sector.start + block.location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    )
                    {   if( block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        else
                        {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            block.location.sectors.n = 1;
                            block.location.sectors.pre = block.location.sectors.post = 0;
                        }
                        upper_free_table_i = free_table_i;
                        break;
                    }
                    if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                    && block.location.in_sector.start + block.location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    )
                    {   uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.n = 0;
                        block.location.sectors.pre = size;
                        block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        upper_free_table_i = free_table_i;
                        break;
                    }
                }
            }
        }
        bool lower = no;
        uint64_t free_table_start, free_table_end;
        if( !~upper_free_table_i )
        {   free_table_start = H_oux_E_fs_Q_free_table_R( device_i, block.sector );
            if( free_table_start == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                free_table_start--;
            free_table_end = ~0ULL;
        }else
        {   free_table_start = free_table_end = upper_free_table_i - 1;
            if( upper_free_table_i )
                free_table_end--;
        }
        if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            for( free_table_i = free_table_start; free_table_i != free_table_end; free_table_i-- )
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n < block.sector - 1 )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    {   block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        lower = yes;
                        break;
                    }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector )
                    {   block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        lower = yes;
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector < block.sector - 1 )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size - block.location.sectors.pre
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start )
                            block.location.sectors.pre += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        else
                        {   block.sector--;
                            block.location.sectors.n++;
                            block.location.sectors.pre = 0;
                        }
                        lower = yes;
                        break;
                    }
                }
        else
        {   if( !~upper_free_table_i )
            {   while( free_table_start != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1
                && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_start ].sector == block.sector
                )
                    free_table_start++;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_start ].sector != block.sector )
                    free_table_start--;
            }
            for( free_table_i = free_table_start; free_table_i != free_table_end; free_table_i-- )
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n < block.sector )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post == block.location.in_sector.start
                    )
                    {   uint16_t size = block.location.in_sector.size;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                            block.location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + size;
                        else
                        {   block.location.sectors.n++;
                            block.location.sectors.post = 0;
                        }
                        lower = yes;
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector < block.sector - 1 )
                        break;
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == block.location.in_sector.start
                    )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size + block.location.in_sector.size != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   block.location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start;
                            block.location.in_sector.size += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        }else
                        {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                            block.location.sectors.n = 1;
                            block.location.sectors.pre = block.location.sectors.post = 0;
                        }
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint16_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.n = 0;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        block.location.sectors.post = size;
                        lower = yes;
                        break;
                    }
                }
        }
        if( ~upper_free_table_i
        && lower
        )
            realloc_subtract = yes;
        else if( ~upper_free_table_i
        || lower
        )
        {   if( ~upper_free_table_i )
                free_table_i = upper_free_table_i;
            realloc_add = no;
            realloc_subtract = no;
        }else
        {   realloc_add = yes;
            realloc_subtract = no;
            free_table_i = H_oux_E_fs_Q_free_table_R( device_i, block.sector );
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_in_sector )
            {   while( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                && free_table_i
                )
                    free_table_i--;
                for( ; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector > block.sector
                    || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                      && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_in_sector
                      && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start > block.location.in_sector.start + block.location.in_sector.size
                    ))
                            break;
            }
        }
    }else
    {   realloc_add = yes;
        realloc_subtract = no;
        free_table_i = 0;
    }
    int error = 0;
    if( realloc_subtract )
    {   if( free_table_i + 2 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1
            , H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 2
            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_i + 2 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
            );
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
        if(p)
            H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
        else
            error = ENOMEM;
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_alloc_flags );
        if( !p )
            return -ENOMEM;
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
        if( free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1
            , H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i
            , ( H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - free_table_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table )
            );
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ] = block;
    return error;
}
//------------------------------------------------------------------------------
int
H_oux_E_fs_Z_start_n_I_block_append( unsigned device_i
, uint64_t n
, uint64_t *block_start
, uint64_t *block_n
, uint64_t *changed_from
, int64_t *block_table_diff
, uint64_t internal_table_element_size
, uint64_t *count
){  uint64_t n_0 = n;
    int error = 0;
    *block_table_diff = 0;
    *count = 0;
    uint64_t size;
    uint64_t free_table_found_i;
    if( *block_n
    && H_oux_E_fs_Q_device_S[ device_i ].free_table_n
    )
    {   uint64_t block_start_ = block_start ? *block_start : 0;
        // Szukaj wolnego bloku na całą tablicę.
        uint64_t table_size = H_oux_E_fs_Z_start_n_R_size( device_i, block_start_, *block_n ) + n + internal_table_element_size;
        uint64_t lowest_size = ~0ULL;
        for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
        {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
              ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
              : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
            if( size >= table_size
            && lowest_size > size
            )
            {   lowest_size = size;
                free_table_found_i = free_table_i;
                if( size == table_size )
                    break;
            }
        }
        if( ~lowest_size )
        {   for( uint64_t block_table_i = 0; block_table_i != *block_n; block_table_i++ )
            {   if( error >= 0 )
                    error = H_oux_E_fs_Q_free_table_I_unite( device_i, &H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + block_table_i ] );
                *block_table_diff -= sizeof( uint64_t ) + 1
                + ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                  ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
                  : sizeof( uint16_t ) + sizeof( uint16_t )
                );
            }
            if( error < 0 )
            {   pr_crit( "some free blocks not counted, remount filesystem: device_i=%u\n", device_i );
                error = -error;
            }
            memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_start_
            , H_oux_E_fs_Q_device_S[ device_i ].block_table + block_start_ + *block_n
            , ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( block_start_ + *block_n )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table )
            );
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start > block_start_ )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start -= *block_n;
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start > block_start_ )
                H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start -= *block_n;
            for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
                if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start > block_start_ )
                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start -= *block_n;
            H_oux_E_fs_Q_device_S[ device_i ].block_table_n -= *block_n;
            *block_n = 0;
            *changed_from = 0;
            void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
            if(p)
                H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
            else
                error = ENOMEM;
            uint64_t lowest_size = ~0ULL;
            for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
            {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                  ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                  : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                if( size >= table_size
                && lowest_size > size
                )
                {   lowest_size = size;
                    free_table_found_i = free_table_i;
                    if( size == table_size )
                        break;
                }
            }
            n = table_size;
            size = lowest_size;
            goto Compute;
        }
        // Szukaj wolnego bloku przyległego od góry do ostatniego przydzielonego.
        uint64_t free_table_i = H_oux_E_fs_Q_free_table_R( device_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector + 1 );
        if( free_table_i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            free_table_i--;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            for( ; ~free_table_i; free_table_i-- )
            {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                  ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                  : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                  < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector
                  + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.n
                )
                    break;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.n
                        == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                      && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.post
                        + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                        == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    )
                    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.n
                        == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                      && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.post
                      && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    ))
                    {   free_table_found_i = free_table_i;
                        (*count)--;
                        goto Compute;
                    }
                }else
                {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.n
                        == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                      && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    )
                    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.n
                        == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                      && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.sectors.post
                      && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    ))
                    {   free_table_found_i = free_table_i;
                        (*count)--;
                        goto Compute;
                    }
                }
            }
        else
        {   while( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector + 1
            && free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1
            )
                free_table_i++;
            for( ; ~free_table_i; free_table_i-- )
            {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
                  ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                    + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post
                  : H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector )
                    break;
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                    && ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.start
                      + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.size
                      == H_oux_E_fs_Q_device_S[ device_i ].sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
                    ))
                    {   free_table_found_i = free_table_i;
                        (*count)--;
                        goto Compute;
                    }
                }else
                {   if(( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector
                      && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.start
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.size
                        == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    )
                    || ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                      && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.start
                        + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start_ + *block_n - 1 ].location.in_sector.size
                        == H_oux_E_fs_Q_device_S[ device_i ].sector_size
                      && !H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start
                    ))
                    {   free_table_found_i = free_table_i;
                        (*count)--;
                        goto Compute;
                    }
                }
            }
        }
    }
    n += internal_table_element_size;
    // Szukaj wolnego bloku na całe żądane dopisywane dane.
    uint64_t lowest_size = ~0ULL;
    for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
    {   size = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre
            + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size
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
    {   size = lowest_size;
        goto Compute;
    }
    O{  if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "no space left on device: %llu\n", n );
            return -ENOSPC;
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
            {   if( size >= n )
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
                if( size >= n
                && lowest_size > size
                )
                {   lowest_size = size;
                    free_table_found_i = free_table_i;
                    if( size == n )
                        break;
                }
            }
            size = lowest_size;
        }
Compute:(*count)++;
        uint64_t n__;
        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
            {   n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre );
                n -= n__;
                if( !n )
                {   int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                    , block_start, block_n
                    , free_table_found_i, size - n__
                    , block_table_diff
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
                        , n_0 - ( n + n__ )
                        , *block_start, block_n
                        , block_table_diff
                        );
                        if( error_ )
                            error = error_;
                    }
                    break;
                }
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n; sector_i++ )
            {   n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                n -= n__;
                if( !n )
                {   uint64_t size_left = size
                      - ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                        + sector_i * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                        + n__
                        );
                    int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                    , block_start, block_n
                    , free_table_found_i, size_left
                    , block_table_diff
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
                        , n_0 - ( n + n__ )
                        , *block_start, block_n
                        , block_table_diff
                        );
                        if( error_ )
                            error = error_;
                    }
                    goto Loop_end;
                }
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post )
            {   n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post );
                n -= n__;
                if( !n )
                {   uint16_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                    int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                    , block_start, block_n
                    , free_table_found_i, post - n__
                    , block_table_diff
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
                        , n_0 - ( n + n__ )
                        , *block_start, block_n
                        , block_table_diff
                        );
                        if( error_ )
                            error = error_;
                    }
                    break;
                }
            }
        }else
        {   n__ = H_oux_J_min( n, size );
            n -= n__;
            if( !n )
            {   int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
                , block_start, block_n
                , free_table_found_i, size - n__
                , block_table_diff
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
                    , n_0 - ( n + n__ )
                    , *block_start, block_n
                    , block_table_diff
                    );
                    if( error_ )
                        error = error_;
                }
                break;
            }
        }
        int error_ = H_oux_E_fs_Q_block_table_I_unite( device_i
        , block_start, block_n
        , free_table_found_i, 0
        , block_table_diff
        );
        if( error_ )
            error = error_;
        if( error_ < 0 )
        {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
            , n_0 - ( n + n__ )
            , *block_start, block_n
            , block_table_diff
            );
            if( error_ )
                error = error_;
            break;
        }
        n += internal_table_element_size;
    }
Loop_end:
    if( error > 0 )
        error = -error;
    return error;
}
int
H_oux_E_fs_Z_start_n_I_block_truncate( unsigned device_i
, uint64_t n
, uint64_t block_start
, uint64_t *block_n
, int64_t *block_table_diff
){  *block_table_diff = 0;
    uint64_t block_table_i;
    bool block_first_use = no;
    struct H_oux_E_fs_Z_block block_first;
    uint64_t block_delete_start;
    for( block_table_i = *block_n - 1; ~block_table_i; block_table_i-- )
    {   struct H_oux_E_fs_Z_block *block = &H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_table_i ];
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   uint64_t n__ = H_oux_J_min( n, block->location.sectors.post );
            n -= n__;
            if( !n )
            {   block_delete_start = block_table_i;
                uint16_t post = block->location.sectors.post;
                block->location.sectors.post -= n__;
                if( n__ != post )
                {   block_first_use = yes;
                    block_first = *block;
                    block_first.sector++;
                    block_first.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    block_first.location.in_sector.start = block->location.sectors.post;
                    block_first.location.in_sector.size = post - block_first.location.in_sector.start;
                }else
                {   block_delete_start++;
                    if( !block->location.sectors.n )
                    {   uint16_t pre = block->location.sectors.pre;
                        block->sector--;
                        block->location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        block->location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                        block->location.in_sector.size = pre;
                        *block_table_diff -= sizeof( uint64_t );
                    }
                }
                break;
            }
            for( uint64_t sector_i = 0; sector_i != block->location.sectors.n; sector_i++ )
            {   uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].sector_size );
                n -= n__;
                if( !n )
                {   block_delete_start = block_table_i;
                    uint64_t size = block->location.sectors.pre + block->location.sectors.n * H_oux_E_fs_Q_device_S[ device_i ].sector_size + block->location.sectors.post;
                    uint64_t size_left = size
                    - ( block->location.sectors.post
                      + sector_i * H_oux_E_fs_Q_device_S[ device_i ].sector_size
                      + n__
                    );
                    if( size_left )
                    {   block_first_use = yes;
                        block_first = *block;
                        block->location.sectors.n -= sector_i + 1;
                        block_first.sector += block->location.sectors.n;
                        block_first.location.sectors.n = sector_i;
                        if( n__ != H_oux_E_fs_Q_device_S[ device_i ].sector_size )
                        {   block->location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].sector_size - n__;
                            block_first.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].sector_size - block->location.sectors.post;
                            if( !block->location.sectors.n
                            && !block->location.sectors.pre
                            )
                            {   uint16_t post = block->location.sectors.post;
                                block->location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                block->location.in_sector.start = 0;
                                block->location.in_sector.size = post;
                                *block_table_diff -= sizeof( uint64_t );
                            }
                            if( !block_first.location.sectors.n
                            && !block_first.location.sectors.post
                            )
                            {   uint16_t pre = block_first.location.sectors.pre;
                                block_first.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                block_first.sector--;
                                block_first.location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                                block_first.location.in_sector.size = pre;
                            }
                        }else
                        {   block->location.sectors.n++;
                            block->location.sectors.post = 0;
                            block_first.sector--;
                            block_first.location.sectors.n++;
                            block_first.location.sectors.pre = 0;
                        }
                    }
                    goto Loop_end;
                }
            }
            n__ = H_oux_J_min( n, block->location.sectors.pre );
            n -= n__;
            if( !n )
            {   block_delete_start = block_table_i;
                if( n__ != block->location.sectors.pre )
                {   block_first_use = yes;
                    block_first = *block;
                    uint16_t pre = block->location.sectors.pre;
                    block->sector--;
                    block->location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    block->location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].sector_size - pre;
                    block->location.in_sector.size = pre - n__;
                    block_first.location.sectors.pre -= block->location.in_sector.size;
                    *block_table_diff -= sizeof( uint64_t );
                }
                break;
            }
        }else
        {   uint64_t n__ = H_oux_J_min( n, block->location.in_sector.size );
            n -= n__;
            if( !n )
            {   block_delete_start = block_table_i;
                if( n__ != block->location.in_sector.size )
                {   block_first_use = yes;
                    block_first = *block;
                    block->location.in_sector.size -= n__;
                    block_first.location.in_sector.start += block->location.in_sector.size;
                    block_first.location.in_sector.size = n__;
                }
                break;
            }
        }
    }
    if(n) // Sprawdzenie na czas testów.
        return -EINVAL;
Loop_end:
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > block_table_i )
        H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = block_table_i;
    int error = 0;
    if( block_first_use )
    {   error = H_oux_E_fs_Q_free_table_I_unite( device_i, &block_first );
        block_delete_start++;
    }
    for( uint64_t block_table_i = block_delete_start; block_table_i != *block_n; block_table_i++ )
    {   if( error >= 0 )
            error = H_oux_E_fs_Q_free_table_I_unite( device_i, &H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_table_i ] );
        *block_table_diff -= sizeof( uint64_t ) + 1
        + ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_start + block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors
          ? sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t )
          : sizeof( uint16_t ) + sizeof( uint16_t )
        );
    }
    if( error < 0 )
    {   pr_crit( "some free blocks not counted, remount filesystem: device_i=%u\n", device_i );
        error = -error;
    }
    if( block_delete_start != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
    {   memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_start + block_delete_start
        , H_oux_E_fs_Q_device_S[ device_i ].block_table + block_start + *block_n
        , ( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( block_start + *block_n )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table )
        );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start > block_start + block_delete_start )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start -= *block_n - block_delete_start;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start > block_start + block_delete_start )
            H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start -= *block_n - block_delete_start;
        for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
            if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start > block_start + block_delete_start )
                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start -= *block_n - block_delete_start;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n -= *block_n - block_delete_start;
        *block_n = block_delete_start;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_alloc_flags );
        if(p)
            H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        else
            error = ENOMEM;
    }
    return error;
}
int
H_oux_E_fs_Q_block_table_I_append_truncate( unsigned device_i
, int64_t block_table_diff
){  int error = 0;
    pr_info( "block_table_diff: %lld\n", block_table_diff );
    if( block_table_diff > 0 )
    {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size )
        {   int64_t block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
            ? block_table_diff
            : H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
            int64_t block_table_diff_;
            uint64_t count;
            uint64_t internal_table_element_size = sizeof( uint64_t ) + 1 + sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint16_t );
            error = H_oux_E_fs_Z_start_n_I_block_append( device_i
            , block_table_diff_above
            , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
            , &H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from
            , &block_table_diff_
            , internal_table_element_size
            , &count
            );
            if(error)
                return error;
            pr_info( "block_table_diff_above: %lld\n", block_table_diff_above );
            pr_info( "block_table_diff_: %lld\n", block_table_diff_ );
            block_table_diff_ = count * internal_table_element_size - block_table_diff_;
            pr_info( "block_table_diff_: %lld\n", block_table_diff_ );
            if( block_table_diff_ )
            {   int64_t block_table_diff__;
                block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff + count * internal_table_element_size - block_table_diff_ > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
                ? block_table_diff_
                : H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff + count * internal_table_element_size - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
                pr_info( "block_table_diff_above: %lld\n", block_table_diff_above );
                int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                , block_table_diff_above
                , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
                , &block_table_diff__
                );
                if( error_ )
                    error = error_;
                while( block_table_diff__ )
                {   block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff - block_table_diff_ + block_table_diff__ > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
                    ? -block_table_diff__
                    : H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff - block_table_diff_ - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
                    H_oux_E_fs_Q_device_S[ device_i ].block_table_size += block_table_diff__;
                    pr_info( "block_table_diff_above: %lld\n", block_table_diff_above );
                    if( block_table_diff_above <= 0 )
                        break;
                    int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
                    , block_table_diff_above
                    , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
                    , &block_table_diff__
                    );
                    if( error_ )
                        error = error_;
                }
            }
            H_oux_E_fs_Q_device_S[ device_i ].block_table_size -= block_table_diff_;
        }
    }else if( block_table_diff < 0 )
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table_size > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size )
        {   int64_t block_table_diff__;
            int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
            , H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
              ? -block_table_diff
              : H_oux_E_fs_Q_device_S[ device_i ].block_table_size - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
            , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
            , &block_table_diff__
            );
            if( error_ )
                error = error_;
            while( block_table_diff__ )
            {   int64_t block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff + block_table_diff__ > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
                ? -block_table_diff__
                : H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
                H_oux_E_fs_Q_device_S[ device_i ].block_table_size += block_table_diff__;
                if( block_table_diff_above <= 0 )
                    break;
                int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i, block_table_diff_above
                , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
                , &block_table_diff__
                );
                if( error_ )
                    error = error_;
            }
        }
    H_oux_E_fs_Q_device_S[ device_i ].block_table_size += block_table_diff;
    pr_info( "block_table_size: %llu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table_size );
    return error;
}
int
H_oux_E_fs_Q_directory_file_I_block_append( unsigned device_i
, uint64_t n
, uint64_t *block_start
, uint64_t *block_n
, uint64_t *changed_from
){  int64_t block_table_diff;
    uint64_t count;
    /*pr_info( "1\n" );
    for( uint64_t i = 0; i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; i++ )
    {   pr_info( "%llu. type: %u, sector: %llu\n", i, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size );
    }
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
    {   pr_info( "block_table_i: %llu, sector: %llu\n", block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
    }*/
    int error = H_oux_E_fs_Z_start_n_I_block_append( device_i
    , n
    , block_start, block_n
    , changed_from
    , &block_table_diff
    , 0, &count
    );
    if(error)
        return error;
    int error_ = H_oux_E_fs_Q_block_table_I_append_truncate( device_i, block_table_diff );
    if( error_ )
        error = error_;
    if( error_ < 0 )
    {   error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
        , n
        , *block_start, block_n
        , &block_table_diff
        );
        if( error_ )
            error = error_;
    }
    /*pr_info( "2\n" );
    for( uint64_t i = 0; i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; i++ )
    {   pr_info( "%llu. type: %u, sector: %llu\n", i, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size );
    }
    for( uint64_t block_table_i = 0; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i++ )
    {   pr_info( "block_table_i: %llu, sector: %llu\n", block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector );
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            pr_info( "n: %llu, pre: %hu, post: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post );
        else
            pr_info( "start: %hu, size: %hu\n", H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
    }*/
    if( error > 0 )
        error = -error;
    return error;
}
static
int
H_oux_E_fs_Q_directory_file_I_block_append_truncate( unsigned device_i
, long n_prev
, const char *name
, uint64_t *block_start
, uint64_t *block_n
, uint64_t *changed_from
){  int error = 0;
    uint64_t n = strlen(name) + 1;
    if( n > n_prev )
        error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, n - n_prev, block_start, block_n, changed_from );
    else if( n < n_prev )
    {   int64_t block_table_diff__;
        error = H_oux_E_fs_Z_start_n_I_block_truncate( device_i, n_prev - n, *block_start, block_n, &block_table_diff__ );
        while( block_table_diff__ )
        {   uint64_t block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff__ > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
            ? -block_table_diff__
            : H_oux_E_fs_Q_device_S[ device_i ].block_table_size - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += block_table_diff__;
            if( block_table_diff_above <= 0 )
                break;
            int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i
            , block_table_diff_above
            , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
            , &block_table_diff__
            );
            if( error_ )
                error = error_;
        }
        error = -error;
    }
    return error;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_I_list_directory
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, uint64_t __user *, list
){  int error = 0;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    if( ~uid )
    {   uint64_t directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
        if(error)
            goto Error_0;
    }
    uint64_t n_;
    error = get_user( n_, n );
    if(error)
        goto Error_0;
    uint64_t *list_ = kmalloc_array( 0, sizeof( *list_ ), E_oux_E_fs_S_alloc_flags );
    uint64_t n__ = 0;
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == uid )
        {   if( n__ < n_ )
            {   void *p = krealloc_array( list_, n__ + 1, sizeof( *list_ ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_1;
                }
                list_ = p;
                list_[ n__ ] = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
            }
            n__++;
        }
    error = put_user( n__, n );
    if(error)
        goto Error_1;
    if( n__ > n_ )
        n__ = n_;
    if( n__ )
        if( copy_to_user( list, list_, n__ * sizeof( *list_ )))
        {   error = -EPERM;
            goto Error_1;
        }
Error_1:
    kfree( list_ );
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_I_list_file
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, uint64_t __user *, list
){  int error = 0;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    if( ~uid )
    {   uint64_t directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
        if(error)
            goto Error_0;
    }
    uint64_t n_;
    error = get_user( n_, n );
    if(error)
        goto Error_0;
    uint64_t *list_ = kmalloc_array( 0, sizeof( *list_ ), E_oux_E_fs_S_alloc_flags );
    uint64_t n__ = 0;
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; file_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == uid )
        {   if( n__ < n_ )
            {   void *p = krealloc_array( list_, n__ + 1, sizeof( *list_ ), E_oux_E_fs_S_alloc_flags );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_1;
                }
                list_ = p;
                list_[ n__ ] = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid;
            }
            n__++;
        }
    error = put_user( n__, n );
    if(error)
        goto Error_1;
    if( n__ > n_ )
        n__ = n_;
    if( n__ )
        if( copy_to_user( list, list_, n__ * sizeof( *list_ )))
        {   error = -EPERM;
            goto Error_1;
        }
Error_1:
    kfree( list_ );
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_R_name
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, char __user *, name
){  int error = 0;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    uint64_t directory_i;
    error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name ) + 1;
    uint64_t n_;
    error = get_user( n_, n );
    if(error)
        goto Error_0;
    if( n_ >= n__ )
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, n__ ))
        {   error = -EPERM;
            goto Error_0;
        }
    error = put_user( n__, n );
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_P_name
, unsigned, device_i
, uint64_t, uid
, const char __user *, name
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
    char *name_ = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
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
    uint64_t parent = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent;
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
    error = H_oux_E_fs_Q_directory_file_I_block_append_truncate( device_i
    , strlen( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name ) + 1
    , name_
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
    , &H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from
    );
    if(error)
    {   kfree( name_ );
        goto Error_0;
    }
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = name_;
    if( H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from > directory_i )
        H_oux_E_fs_Q_device_S[ device_i ].directory_table_changed_from = directory_i;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_R_name
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, char __user *, name
){  int error = 0;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    uint64_t file_i;
    error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name ) + 1;
    uint64_t n_;
    error = get_user( n_, n );
    if(error)
        goto Error_0;
    if( n_ >= n__ )
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, n__ ))
        {   error = -EPERM;
            goto Error_0;
        }
    error = put_user( n__, n );
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_P_name
, unsigned, device_i
, uint64_t, uid
, const char __user *, name
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
    char *name_ = kmalloc( H_oux_E_fs_Q_device_S[ device_i ].sector_size, E_oux_E_fs_S_alloc_flags );
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
    uint64_t parent = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent;
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
    error = H_oux_E_fs_Q_directory_file_I_block_append_truncate( device_i
    , strlen( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name ) + 1
    , name_
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
    , &H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from
    );
    if(error)
    {   kfree( name_ );
        goto Error_0;
    }
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = name_;
    if( H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from > file_i )
        H_oux_E_fs_Q_device_S[ device_i ].file_table_changed_from = file_i;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_R_size
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
){  int error = 0;
    if( down_read_killable( &E_oux_E_fs_S_rw_lock ))
        return -ERESTARTSYS;
    if( device_i >= H_oux_E_fs_Q_device_S_n )
    {   error = -EINVAL;
        goto Error_0;
    }
    uint64_t file_i;
    error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    uint64_t n_ = H_oux_E_fs_Z_start_n_R_size( device_i, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n );
    error = put_user( n_, n );
Error_0:
    up_read( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_I_move
, unsigned, device_i
, uint64_t, uid
, uint64_t, parent
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
    if( ~parent )
    {   uint64_t directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, parent, &directory_i );
        if(error)
            goto Error_0;
    }
    const char *name_ = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name;
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, name_ )
        )
        {   error = -EEXIST;
            goto Error_0;
        }
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, name_ )
        )
        {   error = -EEXIST;
            goto Error_0;
        }
    if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == parent )
    {   error = -EINVAL;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = parent;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_move
, unsigned, device_i
, uint64_t, uid
, uint64_t, parent
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
    if( ~parent )
    {   uint64_t directory_i;
        error = H_oux_E_fs_Q_directory_R( device_i, parent, &directory_i );
        if(error)
            goto Error_0;
    }
    const char *name_ = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name;
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, name_ )
        )
        {   error = -EEXIST;
            goto Error_0;
        }
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == parent
        && !strcmp( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, name_ )
        )
        {   error = -EEXIST;
            goto Error_0;
        }
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent == parent )
    {   error = -EINVAL;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = parent;
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_truncate
, unsigned, device_i
, uint64_t, uid
, uint64_t, size
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
    uint64_t size_orig = H_oux_E_fs_Z_start_n_R_size( device_i, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n );
    if( size > size_orig )
    {   error = -EINVAL;
        goto Error_0;
    }
    if( size_orig - size )
    {   int64_t block_table_diff__;
        error = H_oux_E_fs_Z_start_n_I_block_truncate( device_i, size_orig - size
        , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start, &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
        , &block_table_diff__
        );
        while( block_table_diff__ )
        {   int64_t block_table_diff_above = H_oux_E_fs_Q_device_S[ device_i ].block_table_size + block_table_diff__ > H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size
            ? -block_table_diff__
            : H_oux_E_fs_Q_device_S[ device_i ].block_table_size - H_oux_E_fs_Q_device_S[ device_i ].first_sector_max_size;
            H_oux_E_fs_Q_device_S[ device_i ].block_table_size += block_table_diff__;
            if( block_table_diff_above <= 0 )
                break;
            int error_ = H_oux_E_fs_Z_start_n_I_block_truncate( device_i, block_table_diff_above
            , 0, &H_oux_E_fs_Q_device_S[ device_i ].block_table_block_table_n
            , &block_table_diff__
            );
            if( error_ )
                error = error_;
        }
        if( error > 0 )
            error = -error;
    }
Error_0:
    up_write( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
