/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         main
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include "fs.h"
//==============================================================================
struct H_oux_E_fs_Q_device_Z
{ struct bdev_handle *bdev_handle;
  struct H_oux_E_fs_Z_block *block_table;
  uint64_t block_table_n;
  struct H_oux_E_fs_Z_file *file;
  uint64_t file_n;
  struct H_oux_E_fs_Z_directory *directory;
  uint64_t directory_n;
} *H_oux_E_fs_Q_device_S;
unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
static
int
__init
H_oux_E_fs_M( void
){  H_oux_E_fs_Q_device_S = kmalloc_array( 0, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
    return H_oux_E_fs_Q_device_S ? 0 : -ENOMEM;
}
static
void
__exit
H_oux_E_fs_W( void
){  kfree( H_oux_E_fs_Q_device_S );
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int
H_oux_E_fs_Q_device_I_read_sector( uint64_t sector
, struct page *page
){  struct bio *bio = bio_alloc( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle->bdev, 1, REQ_OP_READ, GFP_KERNEL );
    bio->bi_iter.bi_sector = 4 * 2 * sector;
    if( bio_add_page( bio, page, H_oux_E_fs_S_sector_size, 0 ))
    {   bdev_release( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle );
        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle = 0; // Zamiast realokacji.
        return -ENOMEM;
    }
    struct completion event;
    init_completion( &event );
    bio->bi_private = &event;
    submit_bio(bio);
    wait_for_completion( &event );
    bio_put(bio);
    return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_M, char __user *, pathname
){  struct bdev_handle *bdev_handle = bdev_open_by_path( pathname, BLK_OPEN_READ | BLK_OPEN_WRITE | BLK_OPEN_EXCL, 0, 0 );
    if( IS_ERR( bdev_handle ))
        return PTR_ERR( bdev_handle );
    unsigned device_i;
    for( device_i = 0; device_i != H_oux_E_fs_Q_device_S_n; device_i++ )
        if( !H_oux_E_fs_Q_device_S[ device_i ].bdev_handle )
        {   H_oux_E_fs_Q_device_S[ device_i ].bdev_handle = bdev_handle;
            return device_i;
        }
    void *p = krealloc_array( H_oux_E_fs_Q_device_S, H_oux_E_fs_Q_device_S_n, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
    if( !p )
        return -ENOMEM;
    H_oux_E_fs_Q_device_S = p;
    H_oux_E_fs_Q_device_S_n++;
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle = bdev_handle;
    int error;
    struct page *page = alloc_page( GFP_KERNEL );
    if( !page )
    {   error = -ENOMEM;
        goto Error;
    }
    error = H_oux_E_fs_Q_device_I_read_sector( 0, page );
    if(error)
        goto Error;
    char *sector = page_address(page);
    if( strncmp( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident )))
    {   error = -ENODEV;
        goto Error;
    }
    // Odczyt tablicy bloków do pamięci operacyjnej.
    uint64_t *block_table_n = H_oux_J_align_p( sector, uint64_t );
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n = *block_table_n;
    struct H_oux_E_fs_Z_block *block_table = H_oux_J_align_p(( char * )block_table_n + sizeof( H_oux_E_fs_Q_device_S_ident ), struct H_oux_E_fs_Z_block );
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error;
    }
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table = p;
    uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n, ( H_oux_E_fs_S_sector_size - ( uint64_t )block_table ) / sizeof( *block_table ));
    for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[i] = block_table[i];
    if( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n > n )
    {   struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
        for( uint64_t block_table_i = n; block_table_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n; ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i ].n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   error = H_oux_E_fs_Q_device_I_read_sector( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i ].sector + sector_i, page );
                if(error)
                    goto Error;
                uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n - block_table_i, H_oux_E_fs_S_sector_size / sizeof( *block_table ));
                for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w bieżącym sektorze.
                    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i++ ] = block_table[i];
            }
    }
    
    free_page(( unsigned long )sector );
    return H_oux_E_fs_Q_device_S_n - 1;
Error:
    bdev_release( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_handle = 0;
    else
    {   unsigned device_i;
        for( device_i = H_oux_E_fs_Q_device_S_n - 2; device_i != ~0; device_i-- )
            if( H_oux_E_fs_Q_device_S[ device_i ].bdev_handle )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
        if( !p )
            return ENOMEM;
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i;
    }
    return error;
}
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_W, unsigned, device_i
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    
    bdev_release( H_oux_E_fs_Q_device_S[ device_i ].bdev_handle );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_handle = 0;
    else
    {   unsigned device_i;
        for( device_i = H_oux_E_fs_Q_device_S_n - 2; device_i != ~0; device_i-- )
            if( H_oux_E_fs_Q_device_S[ device_i ].bdev_handle )
                break;
        void *p = krealloc_array( H_oux_E_fs_Q_device_S, ++device_i, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
        if( !p )
            return ENOMEM;
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n = device_i;
    }
    return 0;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_lock, unsigned, dev, uint64_t, uid, int, operation
){  
    return 0;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_read, unsigned, dev, uint64_t, uid, uint64_t, sector, uint64_t, count, char __user *, data
){  
    return 0;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write, unsigned, dev, uint64_t, uid, uint64_t, sector, uint64_t, count, const char __user *, data
){  
    return 0;
}
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
