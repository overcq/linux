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
extern rwlock_t E_oux_E_fs_S_rw_lock;
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
//TODO Dodać “syscalle” “truncate” i “move”/“rename”.
uint64_t
H_oux_E_fs_Q_block_table_R( unsigned device_i
, uint64_t file_i
, uint64_t sector
){  if( !H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
        return H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start;
    uint64_t min = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start;
    uint64_t max = min + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n - 1;
    uint64_t block_table_i = min + ( max + 1 - min ) / 2;
    O{  if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == sector )
            break;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector > sector )
        {   if( block_table_i == min )
                break;
            max = block_table_i - 1;
            block_table_i = max - ( block_table_i - min ) / 2;
        }else
        {   if( block_table_i == max )
            {   block_table_i++; // Przesuń na “H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector > sector” lub poza zakres podtablicy bloków.
                break;
            }
            min = block_table_i + 1;
            block_table_i = min + ( max - block_table_i ) / 2;
        }
    }
    return block_table_i;
}
uint64_t
H_oux_E_fs_Q_free_table_R( unsigned device_i
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
    uint64_t *list_ = kmalloc_array( 0, sizeof( uint64_t ), GFP_KERNEL );
    uint64_t n__ = 0;
    for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == uid )
        {   void *p = krealloc_array( list_, n__ + 1, sizeof( uint64_t ), GFP_KERNEL );
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
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
){  write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t parent_directory_i;
    error = H_oux_E_fs_Q_directory_R( device_i, parent, &parent_directory_i );
    if(error)
        goto Error_0;
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, GFP_KERNEL );
    long count = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( count == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        goto Error_1;
    }
    void *p = krealloc( name_, count, GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    name_ = p;
    //TODO Powiększyć listę bloków przydzielonych na tablicę katalogów.
    
    uint64_t uid;
    uint64_t directory_i;
    if( H_oux_E_fs_Q_device_S[ device_i ].directory_n )
    {   uid = H_oux_E_fs_Q_device_S[ device_i ].directory[ H_oux_E_fs_Q_device_S[ device_i ].directory_n - 1 ].uid + 1;
        if(uid)
            directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
        else
        {   for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
            {   if( uid != H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid )
                    break;
                uid++;
            }
        }
    }else
    {   uid = 0;
        directory_i = H_oux_E_fs_Q_device_S[ device_i ].directory_n;
    }
    p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, H_oux_E_fs_Q_device_S[ device_i ].directory_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    if( directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        memmove( &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i + 1 ], &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ], ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - directory_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ));
    H_oux_E_fs_Q_device_S[ device_i ].directory_n++;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = uid;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = H_oux_E_fs_Q_device_S[ device_i ].directory[ parent_directory_i ].uid;
    H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = name_;
Error_1:
    kfree( name_ );
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_directory_W
, unsigned, device_i
, uint64_t, uid
){  write_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        goto Error_0;
    if( directory_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].directory_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ], &H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].directory_n - ( directory_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory, --H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), GFP_KERNEL );
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
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_M
, unsigned, device_i
, uint64_t, parent
, const char __user *, name
){  write_lock( &E_oux_E_fs_S_rw_lock );
    int error = 0;
    if( !~H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   error = -ENFILE;
        goto Error_0;
    }
    uint64_t directory_i;
    error = H_oux_E_fs_Q_directory_R( device_i, parent, &directory_i );
    if(error)
        goto Error_0;
    char *name_ = kmalloc( H_oux_E_fs_S_sector_size, GFP_KERNEL );
    long count = strncpy_from_user( name_, name, H_oux_E_fs_S_sector_size );
    if( count == H_oux_E_fs_S_sector_size )
    {   error = -ENAMETOOLONG;
        goto Error_1;
    }
    void *p = krealloc( name_, count, GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    name_ = p;
    //TODO Powiększyć listę bloków przydzielonych na tablicę plików.
    
    uint64_t uid;
    uint64_t file_i;
    if( H_oux_E_fs_Q_device_S[ device_i ].file_n )
    {   uid = H_oux_E_fs_Q_device_S[ device_i ].file[ H_oux_E_fs_Q_device_S[ device_i ].file_n - 1 ].uid + 1;
        if(uid)
            file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n;
        else
        {   for( file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
            {   if( uid != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid )
                    break;
                uid++;
            }
        }
    }else
    {   uid = 0;
        file_i = H_oux_E_fs_Q_device_S[ device_i ].file_n;
    }
    p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, H_oux_E_fs_Q_device_S[ device_i ].file_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    if( file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n )
        memmove( &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i + 1 ], &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ], ( H_oux_E_fs_Q_device_S[ device_i ].file_n - file_i ) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ));
    H_oux_E_fs_Q_device_S[ device_i ].file_n++;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = uid;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = 0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = name_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
Error_1:
    kfree( name_ );
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_file_W
, unsigned, device_i
, uint64_t, uid
){  write_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    if( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n )
    {   uint64_t free_table_i = H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1;
        for( uint64_t block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n - 1; ~block_table_i; block_table_i-- )
        {   bool realloc_subtract, realloc_add;
            struct H_oux_E_fs_Z_block block = H_oux_E_fs_Q_device_S[ device_i ].block_table[ H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + block_table_i ];
            //TODO Można zamiast tego tworzyć indeksy bloków wstawianych do tablicy wolnych bloków i na końcu ‘realokować’ i przenosić dane. Tylko ze względu na “memmove”.
            if( H_oux_E_fs_Q_device_S[ device_i ].free_table_n
            && free_table_i
            )
            {   free_table_i = H_oux_E_fs_Q_free_table_R( device_i, block.sector, free_table_i - 1 );
                uint64_t upper_block_table_i = ~0;
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
                        uint64_t size = block.location.in_sector.size;
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
                    {   uint64_t size = block.location.in_sector.size;
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
                void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                if( !p )
                {   error = -ENOMEM;
                    goto Error_0;
                }
                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                free_table_i--;
            }else if( realloc_add )
            {   void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
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
        void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table, H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), GFP_KERNEL );
        if( !p )
        {   error = -ENOMEM;
            goto Error_0;
        }
        H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    }
    if( file_i + 1 != H_oux_E_fs_Q_device_S[ device_i ].file_n )
        memcpy( &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ], &H_oux_E_fs_Q_device_S[ device_i ].file[ file_i + 1 ], ( H_oux_E_fs_Q_device_S[ device_i ].file_n - ( file_i + 1 )) * sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ));
    void *p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].file, --H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), GFP_KERNEL );
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
