/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         main
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
//------------------------------------------------------------------------------
#include "../lang.h"
#include "fs.h"
//==============================================================================
struct H_oux_E_fs_Q_device_Z
{ struct file *bdev_file;
  struct H_oux_E_fs_Z_block *block_table;
  uint64_t block_table_n;
  struct H_oux_E_fs_Z_file *file;
  uint64_t file_n;
  struct H_oux_E_fs_Z_directory *directory;
  uint64_t directory_n;
  struct H_oux_E_fs_Z_block *free_table;
  uint64_t free_table_n;
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
SYSCALL_DEFINE1( H_oux_E_fs_Q_device_M, char __user *, pathname
){  struct file *bdev_file = bdev_file_open_by_path( pathname, BLK_OPEN_READ | BLK_OPEN_WRITE | BLK_OPEN_EXCL, 0, 0 );
    if( !bdev_file )
        return -ENOENT;
    unsigned device_i;
    for( device_i = 0; device_i != H_oux_E_fs_Q_device_S_n; device_i++ )
        if( !H_oux_E_fs_Q_device_S[ device_i ].bdev_file )
            break;
    if( device_i == H_oux_E_fs_Q_device_S_n )
    {   void *p = krealloc_array( H_oux_E_fs_Q_device_S, H_oux_E_fs_Q_device_S_n + 1, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
        if( !p )
            return -ENOMEM;
        H_oux_E_fs_Q_device_S = p;
        H_oux_E_fs_Q_device_S_n++;
    }
    H_oux_E_fs_Q_device_S[ device_i ].bdev_file = bdev_file;
    int error;
    char *sector = kmalloc( H_oux_E_fs_S_sector_size, GFP_KERNEL );
    if( !sector )
    {   error = -ENOMEM;
        goto Error_0;
    }
    loff_t offset = 0;
    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
    if( size != H_oux_E_fs_S_sector_size )
        goto Error_1;
    if( strncmp( sector, H_oux_E_fs_Q_device_S_ident, sizeof( H_oux_E_fs_Q_device_S_ident )))
    {   error = -ENODEV;
        goto Error_1;
    }
    uint64_t *block_table_n = H_oux_J_align_up_p( sector, uint64_t );
    H_oux_E_fs_Q_device_S[ device_i ].block_table_n = *block_table_n;
    uint64_t file_table_start = block_table_n[1];
    uint64_t file_table_n = block_table_n[2];
    H_oux_E_fs_Q_device_S[ device_i ].file_n = block_table_n[3];
    uint64_t directory_table_start = block_table_n[4];
    uint64_t directory_table_n = block_table_n[5];
    H_oux_E_fs_Q_device_S[ device_i ].directory_n = block_table_n[6];
    struct H_oux_E_fs_Z_block *block_table = H_oux_J_align_up_p( sizeof( H_oux_E_fs_Q_device_S_ident ) + ( char * )&block_table_n[7], struct H_oux_E_fs_Z_block );
    void *p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].block_table_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].block_table ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_1;
    }
    H_oux_E_fs_Q_device_S[ device_i ].block_table = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].file_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].file ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_2;
    }
    H_oux_E_fs_Q_device_S[ device_i ].file = p;
    p = kmalloc_array( H_oux_E_fs_Q_device_S[ device_i ].directory_n, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].directory ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_3;
    }
    H_oux_E_fs_Q_device_S[ device_i ].directory = p;
    // Odczyt tablicy bloków do pamięci operacyjnej.
    uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ device_i ].block_table_n, ( H_oux_E_fs_S_sector_size - ( uint64_t )block_table ) / sizeof( *block_table ));
    for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
        H_oux_E_fs_Q_device_S[ device_i ].block_table[i] = block_table[i];
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_n > n )
        for( uint64_t block_table_i = n; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].pre )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                struct H_oux_E_fs_Z_block *block_table = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].pre ));
                uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].pre / sizeof( *block_table ));
                for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w początkowym, niepełnym sektorze.
                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i++ ] = block_table[i];
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
                uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - block_table_i, H_oux_E_fs_S_sector_size / sizeof( *block_table ));
                for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w bieżącym sektorze.
                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i++ ] = block_table[i];
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].post )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
                uint64_t n = H_oux_J_min( H_oux_E_fs_Q_device_S[ device_i ].block_table_n - block_table_i, H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].post / sizeof( *block_table ));
                for( uint64_t i = 0; i != n; i++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w końcowym, niepełnym sektorze.
                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i++ ] = block_table[i];
            }
        }
    // Oczyt tablicy plików do pamięci operacyjnej.
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; )
    {   unsigned continue_from = 0;
        uint64_t char_i;
        for( uint64_t file_table_i = 0; file_table_i != file_table_n; file_table_i++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].pre )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].pre ));
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 3:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 4:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 4 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = false;
                            continue_from = 0;
                        }
                    }
                }
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].n; sector_i++ )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 3:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 4:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 4 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = false;
                            continue_from = 0;
                        }
                    }
                }
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
                            continue;
                  case 2:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
                            continue;
                  case 3:
                        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
                            continue;
                  case 4:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 4 )
                        {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post );
                        if( data_c == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].post )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                        }else
                        {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                            H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = false;
                            continue_from = 0;
                        }
                    }
                }
            }
        }
    }
    // Odczyt tablicy katalogów do pamięci operacyjnej.
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; )
    {   unsigned continue_from = 0;
        uint64_t char_i;
        for( uint64_t directory_table_i = 0; directory_table_i != directory_table_n; directory_table_i++ )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].pre )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].pre ));
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 2 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        }else
                            continue_from = 0;
                    }
                }
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].n; sector_i++ )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_S_sector_size )
                            continue;
                  case 2:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 2 )
                        {   p = kmalloc( sector + H_oux_E_fs_S_sector_size - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        }else
                            continue_from = 0;
                    }
                }
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].post )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                    goto Error_4;
                uint64_t *data = ( void * )sector;
                switch( continue_from )
                { case 0:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].post )
                            continue;
                  case 1:
                        H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent = *data;
                        continue_from++;
                        if(( char * )++data == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].post )
                            continue;
                  case 2:
                    {   char *data_c = ( void * )data;
                        if( continue_from != 2 )
                        {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].post - data_c, GFP_KERNEL );
                            if( !p )
                            {   error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                            char_i = 0;
                        }
                        do
                        {   H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name[ char_i++ ] = *data_c;
                            if( !*data_c )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                break;
                            }
                        }while( ++data_c != sector + H_oux_E_fs_S_sector_size );
                        if( data_c == sector + H_oux_E_fs_S_sector_size )
                        {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                            if( !p )
                            {   kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
                                error = -ENOMEM;
                                goto Error_4;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name = p;
                        }else
                            continue_from = 0;
                    }
                }
            }
        }
    }
    kfree(sector);
    // Utworzenie tablicy wolnych bloków.
    p = kmalloc_array( 1, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_4;
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].sector = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].n = i_size_read( file_inode( H_oux_E_fs_Q_device_S[ device_i ].bdev_file ));
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].pre = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].post = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table_n = 1;
    
    return device_i;
Error_5:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].free_table );
Error_4:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
Error_3:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
Error_2:
    kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
Error_1:
    kfree(sector);
Error_0:
    filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
    if( device_i != H_oux_E_fs_Q_device_S_n - 1 )
        H_oux_E_fs_Q_device_S[ device_i ].bdev_file = 0;
    else
    {   unsigned device_i;
        for( device_i = H_oux_E_fs_Q_device_S_n - 2; ~device_i; device_i-- )
            if( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )
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
        if( copy_to_user( list, list_, n__ * sizeof( uint64_t )) != n__ * sizeof( uint64_t ))
            return -EPERM;
    kfree( list_ );
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
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
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name, n__ ) != n__ )
            return -EPERM;
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
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
        if( copy_to_user( name, H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, n__ ) != n__ )
            return -EPERM;
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
        return -EINVAL;
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
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
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
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
        if( copy_to_user( data, data_, n__ ) != n__ )
            return -EPERM;
    put_user( n__, n );
    return 0;
}
SYSCALL_DEFINE5( H_oux_E_fs_Q_file_I_write, unsigned, device_i, uint64_t, uid, uint64_t, sector, uint64_t, n, const char __user *, data
){  if( device_i >= H_oux_E_fs_Q_device_S_n
    || !n
    )
        return -EINVAL;
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        return error;
    if( ~H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid
    && H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid != current->pid
    )
        return -EPERM;
    char *data_ = kmalloc( n, GFP_KERNEL );
    if( !data_ )
        return -ENOMEM;
    if( copy_from_user( data_, data, n ) != n )
        return -EPERM;
    
    kfree( data_ );
    return 0;
}
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
