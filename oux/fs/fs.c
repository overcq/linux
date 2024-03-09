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
    *directory_i = directory_i;
    return 0;
}
int
H_oux_E_fs_Q_file_T( unsigned device_i
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
H_oux_E_fs_Q_device_I_read_sector( uint64_t sector
, struct page *page
){  struct bio *bio = bio_alloc( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].bdev_handle->bdev, 1, REQ_OP_READ, GFP_KERNEL );
    bio->bi_iter.bi_sector = 4 * 2 * sector;
    bio->bi_end_io = 0;
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
        goto Error_0;
    }
    error = H_oux_E_fs_Q_device_I_read_sector( 0, page );
    if(error)
        goto Error_0;
    char *sector = page_address(page);
    if( strncmp( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident )))
    {   error = -ENODEV;
        goto Error_0;
    }
    uint64_t *block_table_n = H_oux_J_align_up_p( sector, uint64_t );
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n = *block_table_n;
    uint64_t file_table_start = block_table_n[1];
    uint64_t file_table_n = block_table_n[2];
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n = block_table_n[3];
    uint64_t directory_table_start = block_table_n[4];
    uint64_t directory_table_n = block_table_n[5];
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory_n = block_table_n[6];
    struct H_oux_E_fs_Z_block *block_table = H_oux_J_align_up_p( sizeof( H_oux_E_fs_Q_device_S_ident ) + ( char * )&block_table_n[7], struct H_oux_E_fs_Z_block );
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_0;
    }
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_2;
    }
    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory = p;
    // Odczyt tablicy bloków do pamięci operacyjnej.
    uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n, ( H_oux_E_fs_S_sector_size - ( uint64_t )block_table ) / sizeof( *block_table ));
    for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[i] = block_table[i];
    if( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n > n )
        for( uint64_t block_table_i = n; block_table_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n; ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i ].n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   error = H_oux_E_fs_Q_device_I_read_sector( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i ].sector + sector_i, page );
                if(error)
                    goto Error_3;
                struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
                uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table_n - block_table_i, H_oux_E_fs_S_sector_size / sizeof( *block_table ));
                for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w bieżącym sektorze.
                    H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ block_table_i++ ] = block_table[i]; // Założenie, że dane są upakowane w pamięci.
            }
    // Oczyt tablicy plików do pamięci operacyjnej.
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n; )
    {   unsigned continue_from = 0;
        uint64_t char_i;
        for( uint64_t file_table_i = 0; file_table_i != file_table_n; file_table_i++ )
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ file_table_start + file_table_i ].n; sector_i++ )
            {   error = H_oux_E_fs_Q_device_I_read_sector( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ file_table_start + file_table_i ].sector + sector_i, page );
                if(error)
                    goto Error_3;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        char_i = 0;
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].block_table.start = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 3:
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].block_table.n = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 4:
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].size = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 5:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 5 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_3;
                            }
                            H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].name = p;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_3;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                            continue;
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].lock_pid = ~0;
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].lock_write = false;
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].lock_read = false;
                        continue_from = 0;
                    }
                }
                if( ++file_i == H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n )
                    break;
            }
    }
    // Odczyt tablicy katalogów do pamięci operacyjnej.
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory_n; )
    {   unsigned continue_from = 0;
        uint64_t char_i;
        for( uint64_t directory_table_i = 0; directory_table_i != directory_table_n; directory_table_i++ )
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ directory_table_start + directory_table_i ].n; sector_i++ )
            {   error = H_oux_E_fs_Q_device_I_read_sector( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table[ directory_table_start + directory_table_i ].sector + sector_i, page );
                if(error)
                    goto Error_3;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        char_i = 0;
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 2 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_3;
                            }
                            H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].name = p;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_3;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                            continue;
                        continue_from = 0;
                    }
                }
                if( ++directory_i == H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n )
                    break;
            }
    }
    free_page(( unsigned long )sector );
    return H_oux_E_fs_Q_device_S_n - 1;
Error_3:
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory );
Error_2:
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file );
Error_1:
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table );
Error_0:
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
    
    // Wyrzucenie z pamięci operacyjne struktur systemu plików.
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory_n; directory_i++ )
        kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory[ directory_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].directory );
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file_n; file_i++ )
        kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file[ file_i ].name );
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].file );
    kfree( H_oux_E_fs_Q_device_S[ H_oux_E_fs_Q_device_S_n - 1 ].block_table );
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
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_I_list, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, uint64_t __user *, list
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        return error;
    uint64_t *list_ = kmalloc_array( 0, sizeof( uint64_t ), GFP_KERNEL );
    uint64_t n__ = 0;
    for( directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
        if( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent == uid )
        {   void *p = krealloc_array( list_, n__ + 1, sizeof( uint64_t ), GFP_KERNEL );
            if( !p )
            {   kfree( list_ );
                return -ENOMEM;
            }
            list_ = p;
            n__++;
            list_[ n__ - 1 ] = H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid;
        }
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        copy_to_user( list, list_, n__ * sizeof( uint64_t ));
    kfree( list_ );
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t directory_i;
    int error = H_oux_E_fs_Q_directory_R( device_i, uid, &directory_i );
    if(error)
        return error;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name ) + 1;
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, n__ );
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        return error;
    uint64_t n__ = strlen( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name ) + 1;
    uint64_t n_;
    get_user( n_, n );
    if( n_ >= n__ )
        copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, n__ );
    put_user( n__, n );
    return 0;
}
//------------------------------------------------------------------------------
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_I_lock, unsigned, device_i, uint64_t, uid, int, operation
){  if( device_i >= H_oux_E_fs_Q_device_S_n
    || ( operation != LOCK_SH
      && operation != LOCK_EX
      && operation != LOCK_UN
    ))
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_T( device_i, uid, &file_i );
    if(error)
        return error;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    )
        return -EPERM;
    if( operation == LOCK_SH )
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = current->pid;
    else if( operation == LOCK_EX )
    {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = current->pid;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = true;
    }else
    {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = false;
    }
    return 0;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_read, unsigned, device_i, uint64_t, uid, uint64_t, sector, uint64_t __user *, n, char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n )
        return -EINVAL;
    uint64_t n_;
    get_user( n_, n );
    if( !n_ )
        return -EINVAL;
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_T( device_i, uid, &file_i );
    if(error)
        return error;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read
    )
        return -EPERM;
    char *data_;
    uint64_t n__;
    
    if( n_ >= n__ )
        copy_to_user( data, data_, n__ );
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write, unsigned, device_i, uint64_t, uid, uint64_t, sector, uint64_t, n, const char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n
    || !n
    )
        return -EINVAL;
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_T( device_i, uid, &file_i );
    if(error)
        return error;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    )
        return -EPERM;
    char *data_ = kmalloc( n, GFP_KERNEL );
    if( !data_ )
        return -ENOMEM;
    copy_from_user( data_, data, n );
    
    kfree( data_ );
    return 0;
}
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
