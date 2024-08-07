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
//TODO Dodać “syscalle” “truncate” i “move”.
uint64_t
H_oux_E_fs_Q_free_table_R_with_max( unsigned device_i
, uint64_t sector
, uint64_t max
){  uint64_t min = 0;
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
uint64_t
H_oux_E_fs_Q_free_table_R( unsigned device_i
, uint64_t sector
){  return H_oux_E_fs_Q_free_table_R_with_max( device_i, sector, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 );
}
int
H_oux_E_fs_Q_directory_R( unsigned device_i
, uint64_t uid
, uint64_t *directory_i
){  uint64_t min = 0;
    uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
    uint64_t directory_i_ = max / 2;
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
){  uint64_t min = 0;
    uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].file_n;
    uint64_t file_i_ = max / 2;
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
H_oux_E_fs_Q_file_Q_block_table_I_unite( unsigned device_i
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
    uint64_t block_table_i;
    bool realloc_subtract, realloc_add;
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   uint64_t upper_block_table_i = ~0;
        if( free_table_found_fit )
            for( uint64_t block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
            ; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
            ; block_table_i++
            )
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
                            break;
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
                            break;
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
                            break;
                        }
                    }else
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
                            break;
                        }
        bool lower = no;
        for( block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
        ; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
        ; block_table_i++
        )
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                        break;
                    }else if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location.sectors.n == block.sector
                    )
                    {   block_table_i++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                        break;
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
                        break;
                    }
            }else
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_S_sector_size
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector
                    && !block.location.in_sector.start
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post == block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
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
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        block.location.sectors.n = 0;
                        block.location.sectors.post = size;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
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
                        break;
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
            block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n;
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
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        block_table_i--;
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
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
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    }
    return error;
}
int
H_oux_E_fs_Q_block_table_I_unite( unsigned device_i
, uint64_t block_start
, uint64_t *block_n
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
    uint64_t block_table_i;
    bool realloc_subtract, realloc_add;
    if( *block_n )
    {   uint64_t upper_block_table_i = ~0;
        if( free_table_found_fit )
            for( uint64_t block_table_i = block_start
            ; block_table_i != block_start + *block_n
            ; block_table_i++
            )
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
                            break;
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
                            break;
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
                            break;
                        }
                    }else
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
                            break;
                        }
        bool lower = no;
        for( uint64_t block_table_i = block_start
        ; block_table_i != block_start + *block_n
        ; block_table_i++
        )
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                        break;
                    }else if( block_table_i + 1 != block_start + *block_n
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i + 1 ].location.sectors.n == block.sector
                    )
                    {   block_table_i++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                        lower = yes;
                        break;
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
                        break;
                    }
            }else
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_S_sector_size
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n + 1;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n == block.sector
                    && !block.location.in_sector.start
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post == block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
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
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                        block.location.sectors.n = 0;
                        block.location.sectors.post = size;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == block.sector
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
                        break;
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
            block_table_i = block_start + *block_n;
        }
    }else
    {   realloc_add = yes;
        block_start = block_table_i = H_oux_E_fs_Q_device_S[ device_i ].block_table_n;
    }
    if( realloc_subtract )
    {   if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].block_table_n - ( block_table_i + 1 ));
        (*block_n)--;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        block_table_i--;
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
        H_oux_E_fs_Q_device_S[ device_i ].block_table_n++;
        (*block_n)++;
        if( block_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].block_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].block_table + block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table_n - 1 - block_table_i );
    }
    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ] = block;
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from > block_table_i )
        H_oux_E_fs_Q_device_S[ device_i ].block_table_changed_from = block_table_i;
    if( free_table_found_fit )
    {   if( free_table_found_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i, H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_found_i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_found_i + 1 ));
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    }
    return error;
}
int
H_oux_E_fs_Q_free_table_I_unite( unsigned device_i
, const struct H_oux_E_fs_Z_block *block_p
){  int error = 0;
    struct H_oux_E_fs_Z_block block = *block_p;
    uint64_t free_table_i;
    bool realloc_subtract, realloc_add;
    if( H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
    {   uint64_t upper_free_table_i = ~0;
        for( uint64_t free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
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
                        upper_free_table_i = free_table_i;
                        break;
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
                        upper_free_table_i = free_table_i;
                        break;
                    }
                }
            else
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( block.sector == H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector - 1
                    && block.location.in_sector.start + block.location.in_sector.size + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   uint64_t start = block.location.in_sector.start;
                        uint64_t size = block.location.in_sector.size;
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
                        upper_free_table_i = free_table_i;
                        break;
                    }
        bool lower = no;
        for( free_table_i = 0; free_table_i != H_oux_E_fs_Q_device_S[ device_i ].free_table_n; free_table_i++ )
            if( block.location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + block.location.sectors.pre == H_oux_E_fs_S_sector_size
                    )
                    {   block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n + 1;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                        lower = yes;
                        break;
                    }else if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i + 1 ].location.sectors.n == block.sector
                    )
                    {   free_table_i++;
                        block.location.sectors.n += H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector;
                        lower = yes;
                        break;
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
                        break;
                    }
            }else
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post + block.location.in_sector.size == H_oux_E_fs_S_sector_size
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n + 1;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n == block.sector
                    && !block.location.in_sector.start
                    )
                    {   block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.pre;
                        block.location.sectors.n = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.n;
                        block.location.sectors.post = 0;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.sectors.post == block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
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
                        break;
                    }
                }else
                {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector - 1
                    && H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size == H_oux_E_fs_S_sector_size
                    && !block.location.in_sector.start
                    )
                    {   uint64_t size = block.location.in_sector.size;
                        block.location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                        block.location.sectors.pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].location.in_sector.size;
                        block.location.sectors.n = 0;
                        block.location.sectors.post = size;
                        lower = yes;
                        break;
                    }
                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ].sector == block.sector
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
                        break;
                    }
                }
        if( ~upper_free_table_i )
            free_table_i = upper_free_table_i;
        if( ~upper_free_table_i
        && lower
        )
            realloc_subtract = yes;
        else if( ~upper_free_table_i
        || lower
        )
        {   realloc_add = no;
            realloc_subtract = no;
        }else
        {   realloc_add = yes;
            realloc_subtract = no;
            free_table_i = H_oux_E_fs_Q_device_S[ device_i ].free_table_n;
        }
    }else
        realloc_add = yes;
    if( realloc_subtract )
    {   if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i, H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( free_table_i + 1 ));
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
        free_table_i--;
    }else if( realloc_add )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), E_oux_E_fs_S_kmalloc_flags );
        if( !p )
        {   error = -ENOMEM;
            return error;
        }
        H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
        H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
        if( free_table_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
            memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + free_table_i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - free_table_i );
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_i ] = block;
    return error;
}
//------------------------------------------------------------------------------
int
H_oux_E_fs_Q_directory_file_I_block_append( unsigned device_i
, long n
, uint64_t block_start
, uint64_t *block_n
){  if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
    {   pr_err( "no space left on device: %lu\n", n );
        return -ENOSPC;
    }
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
    {   int error = H_oux_E_fs_Q_block_table_I_unite( device_i
        , block_start
        , block_n
        , free_table_found_i
        , lowest_size - size
        );
        if(error)
            return error;
        goto Comp;
    }
    O{  if( !H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
        {   pr_err( "no space left on device: %lu\n", n );
            return -ENOSPC;
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
            {   if( size >= n )
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
Comp:   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre )
            {   uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre );
                int error = H_oux_E_fs_Q_block_table_I_unite( device_i
                , block_start
                , block_n
                , free_table_found_i
                , size - n__
                );
                if(error)
                    return error;
                n -= n__;
                if( !n )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre -= n__;
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
            {   uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_S_sector_size );
                uint64_t size_left = size
                  - ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.pre
                    + sector_i * H_oux_E_fs_S_sector_size
                    + n__
                    );
                int error = H_oux_E_fs_Q_block_table_I_unite( device_i
                , block_start
                , block_n
                , free_table_found_i
                , size_left
                );
                if(error)
                    return error;
                n -= n__;
                if( !n )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.n -= sector_i + 1;
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
            {   uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post );
                uint64_t post = H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.sectors.post;
                int error = H_oux_E_fs_Q_block_table_I_unite( device_i
                , block_start
                , block_n
                , free_table_found_i
                , post - n__
                );
                if(error)
                    return error;
                n -= n__;
                if( !n )
                {   if( post != n__ )
                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start = 0;
                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size = post;
                    }
                    break;
                }
            }
        }else
        {   uint64_t n__ = H_oux_J_min( n, size );
            int error = H_oux_E_fs_Q_block_table_I_unite( device_i
            , block_start
            , block_n
            , free_table_found_i
            , size - n__
            );
            if(error)
                return error;
            n -= n__;
            if( !n )
            {   if( size != n__ )
                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.start += size;
                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ free_table_found_i ].location.in_sector.size -= size;
                }
                break;
            }
        }
    }
Loop_end:
    return 0;
}
static
int
H_oux_E_fs_Q_directory_file_I_block_append_truncate( unsigned device_i
, long n_prev
, const char *name
, uint64_t block_start
, uint64_t *block_n
){  int error = 0;
    uint64_t n = strlen(name);
    if( n > n_prev )
    {   error = H_oux_E_fs_Q_directory_file_I_block_append( device_i, n - n_prev, block_start, block_n );
        if(error)
            return error;
    }
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    if( !sector )
        return -ENOMEM;
    uint64_t block_table_i;
    bool block_first_use = no;
    struct H_oux_E_fs_Z_block block_first;
    for( block_table_i = 0; block_table_i != *block_n; block_table_i++ )
    {   struct H_oux_E_fs_Z_block *block = H_oux_E_fs_Q_device_S[ device_i ].block_table + block_start + block_table_i;
        if( block->location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   loff_t offset = ( block->sector - 1 ) * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", block->sector - 1 );
                error = -EIO;
                goto Error_0;
            }
            uint64_t n__ = H_oux_J_min( n, block->location.sectors.pre );
            memcpy( sector + ( H_oux_E_fs_S_sector_size - block->location.sectors.pre ), name, n__ );
            size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "write sector: %llu\n", block->sector - 1 );
                error = -EIO;
                goto Error_0;
            }
            name += n__;
            n -= n__;
            if( !n )
            {   uint64_t size = block->location.sectors.pre + block->location.sectors.n * H_oux_E_fs_S_sector_size + block->location.sectors.post;
                if( size != n__ )
                {   block_first_use = yes;
                    block_first = *block;
                    block_first.location.sectors.pre -= n__;
                    if( !block_first.location.sectors.pre
                    && !block_first.location.sectors.n
                    )
                    {   uint64_t post = block_first.location.sectors.post;
                        block_first.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                        block_first.location.in_sector.start = 0;
                        block_first.location.in_sector.size = post;
                    }
                }
                break;
            }
            for( uint64_t sector_i = 0; sector_i != block->location.sectors.n; sector_i++ )
            {   loff_t offset = ( block->sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "read sector: %llu\n", block->sector + sector_i );
                    error = -EIO;
                    goto Error_0;
                }
                uint64_t n__ = H_oux_J_min( n, H_oux_E_fs_S_sector_size );
                memcpy( sector, name, n__ );
                size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "write sector: %llu\n", block->sector - sector_i );
                    error = -EIO;
                    goto Error_0;
                }
                name += n__;
                n -= n__;
                if( !n )
                {   uint64_t size_left = size
                      - ( block->location.sectors.pre
                        + sector_i * H_oux_E_fs_S_sector_size
                        + n__
                        );
                    if( size_left )
                    {   block_first_use = yes;
                        block_first = *block;
                        if( block->location.sectors.post )
                        {   block_first.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                            block_first.location.in_sector.start = 0;
                            block_first.location.in_sector.size = block->location.sectors.post;
                        }else
                            block_first.location.sectors.pre = 0;
                    }
                    break;
                }
            }
            offset = ( block->sector + block->location.sectors.n ) * H_oux_E_fs_S_sector_size;
            size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", block->sector + block->location.sectors.n );
                error = -EIO;
                goto Error_0;
            }
            n__ = H_oux_J_min( n, block->location.sectors.post );
            memcpy( sector, name, n__ );
            size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "write sector: %llu\n", block->sector + block->location.sectors.n );
                error = -EIO;
                goto Error_0;
            }
            name += n__;
            n -= n__;
            if( !n )
            {   if( block->location.sectors.post != n__ )
                {   block_first_use = yes;
                    block_first = *block;
                    block_first.location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                    block_first.location.in_sector.start = 0;
                    block_first.location.in_sector.size = block->location.sectors.post;
                }
                break;
            }
        }else
        {   loff_t offset = block->sector * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "read sector: %llu\n", block->sector );
                error = -EIO;
                goto Error_0;
            }
            uint64_t n__ = H_oux_J_min( n, block->location.in_sector.size );
            memcpy( sector + block->location.in_sector.start, name, n__ );
            size = kernel_write( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "write sector: %llu\n", block->sector );
                error = -EIO;
                goto Error_0;
            }
            name += n__;
            n -= n__;
            if( !n )
            {   if( block->location.in_sector.size != n__ )
                {   block_first_use = yes;
                    block_first = *block;
                    block_first.location.in_sector.start += size;
                    block_first.location.in_sector.size -= size;
                }
                break;
            }
        }
    }
    if( n < n_prev )
    {   if( block_first_use )
        {   error = H_oux_E_fs_Q_free_table_I_unite( device_i, &block_first );
            if(error)
                goto Error_0;
        }
        for( block_table_i++; block_table_i != *block_n; block_table_i++ )
        {   error = H_oux_E_fs_Q_free_table_I_unite( device_i, &H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ] );
            if(error)
                goto Error_0;
        }
    }
Error_0:
    kfree(sector);
    return error;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_I_list
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, uint64_t __user *, list
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    read_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    uint64_t *list_ = kmalloc_array( 0, sizeof( uint64_t ), E_oux_E_fs_S_kmalloc_flags );
    uint64_t n__ = 0;
    for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == uid )
        {   void *p = krealloc_array( list_, n__ + 1, sizeof( uint64_t ), E_oux_E_fs_S_kmalloc_flags );
            if( !p )
            {   kfree( list_ );
                error = -ENOMEM;
                goto Error_0;
            }
            list_ = p;
            n__++;
            list_[ n__ - 1 ] = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
        }
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        if( copy_to_user( list, list_, n__ * sizeof( uint64_t )) != n__ * sizeof( uint64_t ))
        {   error = -EPERM;
            goto Error_0;
        }
    kfree( list_ );
    put_user( n__, n );
Error_0:
    read_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_R_name
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, char __user *, name
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    read_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name ) + 1;
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, n__ ) != n__ )
        {   error = -EPERM;
            goto Error_0;
        }
    put_user( n__, n );
Error_0:
    read_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_P_name
, unsigned, device_i
, uint64_t, uid
, const char __user *, name
){  write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t directory_i;
    error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    long n = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( n == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        goto Error_0;
    }
    void *p = krealloc( name_, n, E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    name_ = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append_truncate( device_i
    , strlen( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name )
    , H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name
    , H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_directory_table_n
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
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_R_name
, unsigned, device_i
, uint64_t, uid
, uint64_t __user *, n
, char __user *, name
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    read_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name ) + 1;
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, n__ ) != n__ )
        {   error = -EPERM;
            goto Error_0;
        }
    put_user( n__, n );
Error_0:
    read_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_P_name
, unsigned, device_i
, uint64_t, uid
, const char __user *, name
){  write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t file_i;
    error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, E_oux_E_fs_S_kmalloc_flags );
    long n = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( n == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        goto Error_0;
    }
    void *p = krealloc( name_, n, E_oux_E_fs_S_kmalloc_flags );
    if( !p )
    {   error = -ENOMEM;
        kfree( name_ );
        goto Error_0;
    }
    name_ = p;
    error = H_oux_E_fs_Q_directory_file_I_block_append_truncate( device_i
    , strlen( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name )
    , H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name
    , H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_start
    , &H_oux_E_fs_Q_device_S[ device_i ].block_table_file_table_n
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
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
/******************************************************************************/
