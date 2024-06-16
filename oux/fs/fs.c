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
#define H_oux_E_fs_Q_device_I_switch_item( item, end ) \
    if( data_i ) \
    {   char *data_c = ( void * )data; \
        while( data_i-- ) \
        {   item <<= 8; \
            item |= *data_c++; \
        } \
        data = ( void * )data_c; \
    }else \
    {   if(( char * )( data + 1 ) > end ) \
        {   char *data_c = ( void * )data; \
            data_i = sizeof( *data ); \
            while( data_c != end ) \
            {   item <<= 8; \
                item |= *data_c++; \
                data_i--; \
            } \
            data = ( void * )data_c; \
            break; \
        } \
        item = *data++; \
    } \
    continue_from++
//------------------------------------------------------------------------------
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
    {   pr_err( "H_oux_E_fs_Q_device_M: no filesystem identification string" );
        error = -EIO;
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
    if( H_oux_E_fs_Q_device_S[ device_i ].block_table_n < file_table_start + file_table_n
    || H_oux_E_fs_Q_device_S[ device_i ].block_table_n < directory_table_start + directory_table_n
    || ( directory_table_start >= file_table_start
      && directory_table_start < file_table_start + file_table_n
    )
    || ( directory_table_start + directory_table_n >= file_table_start
      && directory_table_start + directory_table_n < file_table_start + file_table_n
    )
    || ( file_table_start >= directory_table_start
      && file_table_start < directory_table_start + directory_table_n
    )
    || ( file_table_start + file_table_n >= directory_table_start
      && file_table_start + file_table_n < directory_table_start + directory_table_n
    ))
    {   pr_err( "H_oux_E_fs_Q_device_M: filesystem header inconsistent" );
        error = -EIO;
        goto Error_1;
    }
    char *block_table = H_oux_J_align_up_p( sizeof( H_oux_E_fs_Q_device_S_ident ) + ( char * )&block_table_n[7], struct H_oux_E_fs_Z_block );
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
    unsigned continue_from = 0;
    uint64_t block_table_i_write;
    for( block_table_i_write = 0; block_table_i_write != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; block_table_i_write++ ) // Czyta wpisy pliku tablicy bloków znajdujące się w pierwszym sektorze.
    {   if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
            break;
        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector = *( uint64_t * )block_table;
        block_table += sizeof( uint64_t );
        continue_from++;
        if(( char * )block_table + sizeof( enum H_oux_E_Fs_Z_block_Z_location ) > sector + H_oux_E_fs_S_sector_size )
            break;
        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type = *( enum H_oux_E_Fs_Z_block_Z_location * )block_table;
        block_table += sizeof( enum H_oux_E_Fs_Z_block_Z_location );
        continue_from++;
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
                break;
            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n = *( uint64_t * )block_table;
            block_table += sizeof( uint64_t );
            continue_from++;
            if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                break;
            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre = *( uint16_t * )block_table;
            block_table += sizeof( uint16_t );
            continue_from++;
            if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                break;
            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post = *( uint16_t * )block_table;
            block_table += sizeof( uint16_t );
        }else
        {   if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                break;
            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start = *( uint16_t * )block_table;
            block_table += sizeof( uint16_t );
            continue_from++;
            if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                break;
            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size = *( uint16_t * )block_table;
            block_table += sizeof( uint16_t );
        }
        continue_from = 0;
    }
    for( uint64_t block_table_i_read = 0; block_table_i_read != H_oux_E_fs_Q_device_S[ device_i ].block_table_n; ) // Czyta wszystkie pozostałe wpisy pliku tablicy bloków.
        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                    goto Error_4;
                }
                struct H_oux_E_fs_Z_block *block_table = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.pre ));
                bool sector_over = no;
                do // Czyta wpisy pliku tablicy bloków znajdujące się w początkowym, niepełnym sektorze.
                {   switch( continue_from )
                    { case 0:
                            if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
                            {   sector_over = yes;
                                break;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector = *( uint64_t * )block_table;
                            block_table += sizeof( uint64_t );
                            continue_from++;
                      default:
                            if( continue_from == 1 )
                            {   if(( char * )block_table + sizeof( enum H_oux_E_Fs_Z_block_Z_location ) > sector + H_oux_E_fs_S_sector_size )
                                {   sector_over = yes;
                                    break;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type = *( enum H_oux_E_Fs_Z_block_Z_location * )block_table;
                                block_table += sizeof( enum H_oux_E_Fs_Z_block_Z_location );
                                continue_from++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n = *( uint64_t * )block_table;
                                        block_table += sizeof( uint64_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 4:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                    }
                }while( !sector_over );
            }
            for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n; sector_i++ ) // Czyta kolejne sektory z szeregu ciągłych.
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                    goto Error_4;
                }
                struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
                bool sector_over = no;
                do // Czyta wpisy pliku tablicy bloków znajdujące się w bieżącym sektorze.
                {   switch( continue_from )
                    { case 0:
                            if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
                            {   sector_over = yes;
                                break;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector = *( uint64_t * )block_table;
                            block_table += sizeof( uint64_t );
                            continue_from++;
                      default:
                            if( continue_from == 1 )
                            {   if(( char * )block_table + sizeof( enum H_oux_E_Fs_Z_block_Z_location ) > sector + H_oux_E_fs_S_sector_size )
                                {   sector_over = yes;
                                    break;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type = *( enum H_oux_E_Fs_Z_block_Z_location * )block_table;
                                block_table += sizeof( enum H_oux_E_Fs_Z_block_Z_location );
                                continue_from++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n = *( uint64_t * )block_table;
                                        block_table += sizeof( uint64_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 4:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_S_sector_size )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                    }
                }while( !sector_over );
            }
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
            {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                    goto Error_4;
                }
                struct H_oux_E_fs_Z_block *block_table = ( void * )sector;
                bool sector_over = no;
                do // Czyta wpisy pliku tablicy bloków znajdujące się w końcowym, niepełnym sektorze.
                {   switch( continue_from )
                    { case 0:
                            if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                            {   sector_over = yes;
                                break;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector = *( uint64_t * )block_table;
                            block_table += sizeof( uint64_t );
                            continue_from++;
                      default:
                            if( continue_from == 1 )
                            {   if(( char * )block_table + sizeof( enum H_oux_E_Fs_Z_block_Z_location ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                {   sector_over = yes;
                                    break;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type = *( enum H_oux_E_Fs_Z_block_Z_location * )block_table;
                                block_table += sizeof( enum H_oux_E_Fs_Z_block_Z_location );
                                continue_from++;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n = *( uint64_t * )block_table;
                                        block_table += sizeof( uint64_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 4:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                            else
                                switch( continue_from )
                                { case 2:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from++;
                                  case 3:
                                        if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.sectors.post )
                                        {   sector_over = yes;
                                            break;
                                        }
                                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size = *( uint16_t * )block_table;
                                        block_table += sizeof( uint16_t );
                                        continue_from = 0;
                                        block_table_i_write++;
                                }
                    }
                }while( !sector_over );
            }
        }else
        {   offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].sector * H_oux_E_fs_S_sector_size;
            ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
            if( size != H_oux_E_fs_S_sector_size )
            {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                goto Error_4;
            }
            struct H_oux_E_fs_Z_block *block_table = ( void * )( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start );
            bool sector_over = no;
            do // Czyta wpisy pliku tablicy bloków znajdujące się we fragmencie sektora.
            {   switch( continue_from )
                { case 0:
                        if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                        {   sector_over = yes;
                            break;
                        }
                        H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].sector = *( uint64_t * )block_table;
                        block_table += sizeof( uint64_t );
                        continue_from++;
                  default:
                        if( continue_from == 1 )
                        {   if(( char * )block_table + sizeof( enum H_oux_E_Fs_Z_block_Z_location ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                            {   sector_over = yes;
                                break;
                            }
                            H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type = *( enum H_oux_E_Fs_Z_block_Z_location * )block_table;
                            block_table += sizeof( enum H_oux_E_Fs_Z_block_Z_location );
                            continue_from++;
                        }
                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                            switch( continue_from )
                            { case 2:
                                    if(( char * )block_table + sizeof( uint64_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                                    {   sector_over = yes;
                                        break;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.n = *( uint64_t * )block_table;
                                    block_table += sizeof( uint64_t );
                                    continue_from++;
                              case 3:
                                    if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                                    {   sector_over = yes;
                                        break;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.pre = *( uint16_t * )block_table;
                                    block_table += sizeof( uint16_t );
                                    continue_from++;
                              case 4:
                                    if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                                    {   sector_over = yes;
                                        break;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.sectors.post = *( uint16_t * )block_table;
                                    block_table += sizeof( uint16_t );
                                    continue_from = 0;
                                    block_table_i_write++;
                            }
                        else
                            switch( continue_from )
                            { case 2:
                                    if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                                    {   sector_over = yes;
                                        break;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.start = *( uint16_t * )block_table;
                                    block_table += sizeof( uint16_t );
                                    continue_from++;
                              case 3:
                                    if(( char * )block_table + sizeof( uint16_t ) > sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_read ].location.in_sector.size )
                                    {   sector_over = yes;
                                        break;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i_write ].location.in_sector.size = *( uint16_t * )block_table;
                                    block_table += sizeof( uint16_t );
                                    continue_from = 0;
                                    block_table_i_write++;
                            }
                }
            }while( !sector_over );
        }
    // Odczyt tablicy plików do pamięci operacyjnej.
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; )
    {   unsigned continue_from = 0;
        unsigned data_i = 0;
        uint64_t char_i;
        for( uint64_t file_table_i = 0; file_table_i != file_table_n; file_table_i++ )
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.pre )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.pre ));
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_S_sector_size
                                );
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
                                data = ( void * )data_c;
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
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                            }
                        }
                    }while(( char * )data < sector + H_oux_E_fs_S_sector_size );
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.n; sector_i++ )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_S_sector_size
                                );
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
                                data = ( void * )data_c;
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
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                            }
                        }
                    }while(( char * )data < sector + H_oux_E_fs_S_sector_size );
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post
                                );
                          case 2:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post
                                );
                          case 3:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post
                                );
                          case 4:
                            {   char *data_c = ( void * )data;
                                if( continue_from != 4 )
                                {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post - data_c, GFP_KERNEL );
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
                                }while( ++data_c != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post );
                                data = ( void * )data_c;
                                if( data_c == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post )
                                {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_S_sector_size, GFP_KERNEL );
                                    if( !p )
                                    {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                        error = -ENOMEM;
                                        goto Error_4;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                                }else
                                {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                    continue_from = 0;
                                }
                            }
                        }
                    }while(( char * )data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.sectors.post );
                }
            }else
            {   offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                    goto Error_4;
                }
                uint64_t *data = ( void * )( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start );
                do
                {   switch( continue_from )
                    { case 0:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 2:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 3:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size
                            );
                      case 4:
                        {   char *data_c = ( void * )data;
                            if( continue_from != 4 )
                            {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size - data_c, GFP_KERNEL );
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
                            }while( ++data_c != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size );
                            data = ( void * )data_c;
                            if( data_c == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size )
                            {   p = krealloc( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name, char_i + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size, GFP_KERNEL );
                                if( !p )
                                {   kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
                                    error = -ENOMEM;
                                    goto Error_4;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = p;
                            }else
                            {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
                                H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
                                continue_from = 0;
                            }
                        }
                    }
                }while(( char * )data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ file_table_start + file_table_i ].location.in_sector.size );
            }
    }
    // Odczyt tablicy katalogów do pamięci operacyjnej.
    for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; )
    {   unsigned continue_from = 0;
        unsigned data_i = 0;
        uint64_t char_i;
        for( uint64_t directory_table_i = 0; directory_table_i != directory_table_n; directory_table_i++ )
            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.pre )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector - 1 ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )( sector + ( H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.pre ));
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
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
                                data = ( void * )data_c;
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
                    }while(( char * )data < sector + H_oux_E_fs_S_sector_size );
                }
                for( uint64_t sector_i = 0; sector_i != H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.n; sector_i++ )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector + sector_i ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_S_sector_size
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_S_sector_size
                                );
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
                                data = ( void * )data_c;
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
                    }while(( char * )data < sector + H_oux_E_fs_S_sector_size );
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.post )
                {   offset = ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.n ) * H_oux_E_fs_S_sector_size;
                    ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                    if( size != H_oux_E_fs_S_sector_size )
                    {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                        goto Error_4;
                    }
                    uint64_t *data = ( void * )sector;
                    do
                    {   switch( continue_from )
                        { case 0:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.post
                                );
                          case 1:
                                H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                                , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.post
                                );
                          case 2:
                            {   char *data_c = ( void * )data;
                                if( continue_from != 2 )
                                {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.post - data_c, GFP_KERNEL );
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
                                data = ( void * )data_c;
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
                    }while(( char * )data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.sectors.post );
                }
            }else
            {   offset = H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].sector * H_oux_E_fs_S_sector_size;
                ssize_t size = kernel_read( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, sector, H_oux_E_fs_S_sector_size, &offset );
                if( size != H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: read sector" );
                    goto Error_4;
                }
                uint64_t *data = ( void * )( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start );
                do
                {   switch( continue_from )
                    { case 0:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].uid
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size
                            );
                      case 1:
                            H_oux_E_fs_Q_device_I_switch_item( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].parent
                            , sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size
                            );
                      case 2:
                        {   char *data_c = ( void * )data;
                            if( continue_from != 2 )
                            {   p = kmalloc( sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size - data_c, GFP_KERNEL );
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
                            }while( ++data_c != sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size );
                            data = ( void * )data_c;
                            if( data_c == sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size )
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
                }while(( char * )data < sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ directory_table_start + directory_table_i ].location.in_sector.size );
            }
    }
    kfree(sector);
    // Utworzenie tablicy wolnych bloków.
    p = kmalloc_array( 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
    if( !p )
    {   error = -ENOMEM;
        goto Error_4;
    }
    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].sector = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.n = i_size_read( file_inode( H_oux_E_fs_Q_device_S[ device_i ].bdev_file )) / H_oux_E_fs_S_sector_size;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.pre = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table[0].location.sectors.post = 0;
    H_oux_E_fs_Q_device_S[ device_i ].free_table_n = 1;
    for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
    {   for( uint64_t block_table_i = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start
        ; block_table_i != H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start + H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n
        ; block_table_i++
        )
        {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
            {   if( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                && ( !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                  || !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                ))
                {   pr_err( "H_oux_E_fs_Q_device_M: inconsistent block: block_table_i=%llu", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre >= H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: \"pre\" greater than sector size: block_table_i=%llu", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post >= H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: \"post\" greater than sector size: block_table_i=%llu", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
            }else
            {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type != H_oux_E_fs_Z_block_Z_location_S_in_sector )
                {   pr_err( "H_oux_E_fs_Q_device_M: location_type unknown: block_table_i=%llu", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size >= H_oux_E_fs_S_sector_size )
                {   pr_err( "H_oux_E_fs_Q_device_M: \"start\" or/and \"size\" greater than sector size: block_table_i=%llu", block_table_i );
                    error = -EIO;
                    goto Error_5;
                }
            }
            uint64_t min = 0;
            uint64_t max = H_oux_E_fs_Q_device_S[ device_i ].free_table_n;
            uint64_t i = max / 2;
            O{  if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                    {   while( ~--i
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                        ){} // Mogą być wpisy we fragmentach sektora powyżej w sektorze startowym, jeśli dla wyszukanego wpisu jest “!H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n”.
                        i++;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors )
                        {   pr_err( "H_oux_E_fs_Q_device_M: no free \"sectors\" block for allocated: block_table_i=%llu", block_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n ) // Centralne sektory znikają.
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            ) // Fragmenty poprzedniego (“pre”) i ostatniego (“post”) sektora pozostają.
                            {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - i );
                                uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                if( free_pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"pre\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                if( free_post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"post\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            && ( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            )) // Fragment poprzedniego sektora (“pre”) pozostaje.
                            {   uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                if( free_pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"pre\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                            }else if(( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                              || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            )
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            ) // Fragment ostatniego sektora (“post”) pozostaje.
                            {   uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector += H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                if( free_post < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"post\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                            }else // Wolny blok równy blokowi lokacji pliku.
                            {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                                p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                            }
                        }else // Ostatnie sektory centralnych sektorów pozostają.
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n )
                            {   pr_err( "H_oux_E_fs_Q_device_M: free sectors less than allocated: block_table_i=%llu", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre
                            ) // Fragment poprzedniego (“pre”) sektora z ostatnimi sektorami centralnych sektorów oraz ewentualnie fragment ostatniego (“post”) sektora pozostają.
                            {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - i );
                                uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                if( free_pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"pre\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sectors_n - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector++;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n--;
                                }else
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = 0;
                            }else // Ostatnie sektory centralnych sektorów z ewentualnym fragmentem ostatniego (“post”) sektora pozostają.
                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector += H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n -= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                {   pr_err( "H_oux_E_fs_Q_device_M: free \"pre\" less than allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post -= H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post )
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector++;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                                }else
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = 0;
                            }
                        }
                    }else
                    {   while( ~--i
                        && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                        ){} // Przesuwanie na początkową pozycję do wyszukiwania wolnego bloku zawierającego blok pliku.
                        i++;
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        {   if( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            )
                            {   do
                                {   i++;
                                    if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                                    || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                    )
                                    {   pr_err( "H_oux_E_fs_Q_device_M: no free in_sector block for allocated: block_table_i=%llu", block_table_i );
                                        error = -EIO;
                                        goto Error_5;
                                    }
                                }while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
                                  < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                );
                                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                                {   pr_err( "H_oux_E_fs_Q_device_M: no free in_sector block for allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                            }
                        }else
                        {   i--;
                            do
                            {   i++;
                                if( i == H_oux_E_fs_Q_device_S[ device_i ].free_table_n
                                || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                )
                                {   pr_err( "H_oux_E_fs_Q_device_M: no free in_sector block for allocated: block_table_i=%llu", block_table_i );
                                    error = -EIO;
                                    goto Error_5;
                                }
                            }while( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
                              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            );
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                            {   pr_err( "H_oux_E_fs_Q_device_M: no free in_sector block for allocated: block_table_i=%llu", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                        }
                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n )
                                if(( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                                  || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                )
                                && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1
                                  || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                                  || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                    != H_oux_E_fs_S_sector_size
                                )) // Ewentualny fragment poprzedniego (“pre”) sektora z ewentualnym fragmentem początkowym pierwszego sektora centralnych sektorów oraz ewentualny fragment końcowy ostatniego sektora centralnych sektorów z ewentualnym fragmentem ostatniego (“post”) sektora pozostają.
                                {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                    if( !p )
                                    {   error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                    if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                        memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - i );
                                    uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                    uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                    uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                    uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                    if( free_pre )
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = 0;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                                        }else
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                                        }
                                    else
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                                    }
                                    if( free_sectors_n > 1 )
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector + 1;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sectors_n - 1;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                                    }else
                                        if( free_post )
                                            if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size != H_oux_E_fs_S_sector_size )
                                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector + 1;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sectors_n - 1;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                                            }else
                                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector + 1;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = 0;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post;
                                            }
                                        else
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size;
                                        }
                                }else if((( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                                    || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                  )
                                  && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n == 1
                                  && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                                  && H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                    == H_oux_E_fs_S_sector_size
                                )
                                || ( !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                                  && !H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                                  && ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1
                                    || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                                    || H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                      != H_oux_E_fs_S_sector_size
                                ))) // Ewentualny fragment poprzedniego (“pre”) sektora z ewentualnym fragmentem początkowym pierwszego sektora centralnych sektorów albo ewentualny fragment końcowy ostatniego sektora centralnych sektorów z ewentualnym fragmentem ostatniego (“post”) sektora pozostają.
                                {   uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                    uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                    if( free_pre )
                                        if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n = 0;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                                        }else
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                                        }
                                    else if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                                    }else
                                        if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n > 1 )
                                        {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector++;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                            H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                                        }else
                                            if( free_post )
                                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                                  != H_oux_E_fs_S_sector_size
                                                )
                                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector++;
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = free_post;
                                                }else
                                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector++;
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = 0;
                                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_post;
                                                }
                                            else
                                            {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size );
                                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size;
                                            }
                                }else
                                {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                        memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                                    p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                    if( !p )
                                    {   error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                                }
                            else
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                                  != H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                                ) // Poprzedni (“pre”) i fragment ostatniego (“post”) sektora pozostają.
                                {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                    if( !p )
                                    {   error = -ENOMEM;
                                        goto Error_5;
                                    }
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                    if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                        memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - i );
                                    uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                    uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                    uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = free_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                                }else // Poprzedni (“pre”) sektor pozostaje.
                                {   uint64_t free_pre = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector--;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - free_pre;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_pre;
                                }
                        else
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
                              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            ) // Krańcowe fragmenty bloku pozostają.
                            {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - i );
                                uint64_t free_start = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                                uint64_t free_size = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - free_start;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_start + free_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                            }else if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start
                            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size
                              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            ) // Jeden końcowy fragment bloku pozostaje.
                            {   uint64_t free_start = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                                uint64_t free_size = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size;
                                if( free_start != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start )
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start - free_start;
                                else
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = free_start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = free_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                }
                            }else // Wolny blok równy blokowi lokacji pliku.
                            {   if( i + 1 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - ( i + 1 ));
                                p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n--;
                            }
                    }
                    break;
                }
                if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector > H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector )
                {   if( i == min )
                    {   i--;
                        if( !~i
                        || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors
                        )
                        {   pr_err( "H_oux_E_fs_Q_device_M: no free \"sectors\" block for allocated: block_table_i=%llu", block_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        // Początkowy fragment pozostaje.
Subtract_sectors:       if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location_type == H_oux_E_fs_Z_block_Z_location_S_sectors )
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                            )
                            {   pr_err( "H_oux_E_fs_Q_device_M: free sectors less than allocated: block_table_i=%llu", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                            || ( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post
                            )) // Ostatnie sektory centralnych sektorów lub ewentualnie fragment ostatniego (“post”) sektora pozostają.
                            {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                if( i + 2 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 ));
                                uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == 1
                                    && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                                    )
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                                    }else
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                    }
                                else
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                                if( free_sector + free_sectors_n
                                  == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n
                                )
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = 0;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.post;
                                }else
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.n;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = 0;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                                }
                            }else // Ostatni fragment znika.
                                if( H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre )
                                    if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector == 1
                                    && !H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.pre
                                    )
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.size = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.in_sector.start;
                                    }else
                                    {   H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n--;
                                        H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_S_sector_size - H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.sectors.pre;
                                    }
                                else
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = 0;
                        }else
                        {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                              == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            && H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              < H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            )
                            {   pr_err( "H_oux_E_fs_Q_device_M: free \"post\" less than allocated: block_table_i=%llu", block_table_i );
                                error = -EIO;
                                goto Error_5;
                            }
                            if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector + H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n
                              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                            || H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post
                              != H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size
                            ) // Ostatnie sektory centralnych sektorów lub ewentualnie fragment ostatniego (“post”) sektora pozostają.
                            {   p = krealloc_array( H_oux_E_fs_Q_device_S[ device_i ].free_table, H_oux_E_fs_Q_device_S[ device_i ].free_table_n + 1, sizeof( *H_oux_E_fs_Q_device_S[ device_i ].free_table ), GFP_KERNEL );
                                if( !p )
                                {   error = -ENOMEM;
                                    goto Error_5;
                                }
                                H_oux_E_fs_Q_device_S[ device_i ].free_table = p;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table_n++;
                                if( i + 2 != H_oux_E_fs_Q_device_S[ device_i ].free_table_n )
                                    memmove( H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 2, H_oux_E_fs_Q_device_S[ device_i ].free_table + i + 1, H_oux_E_fs_Q_device_S[ device_i ].free_table_n - 1 - ( i + 1 ));
                                uint64_t free_sector = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].sector;
                                uint64_t free_sectors_n = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.n;
                                uint64_t free_post = H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post;
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                                if( free_sector + free_sectors_n
                                  == H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector
                                )
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_in_sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.size;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size = free_post - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start;
                                }else
                                {   H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location_type = H_oux_E_fs_Z_block_Z_location_S_sectors;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.n = free_sector + free_sectors_n - H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].sector;
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.pre = H_oux_E_fs_S_sector_size - ( H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.start + H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.in_sector.size );
                                    H_oux_E_fs_Q_device_S[ device_i ].free_table[ i + 1 ].location.sectors.post = free_post;
                                }
                            }else // Ostatni fragment znika.
                                H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location.sectors.post = H_oux_E_fs_Q_device_S[ device_i ].block_table[ block_table_i ].location.in_sector.start;
                        }
                    }
                    max = i - 1;
                    i = max - ( i - min ) / 2;
                }else
                {   if( i == max )
                    {   if( H_oux_E_fs_Q_device_S[ device_i ].free_table[i].location_type != H_oux_E_fs_Z_block_Z_location_S_sectors )
                        {   pr_err( "H_oux_E_fs_Q_device_M: no free block for allocated: block_table_i=%llu", block_table_i );
                            error = -EIO;
                            goto Error_5;
                        }
                        goto Subtract_sectors;
                    }
                    min = i + 1;
                    i = min + ( max - i ) / 2;
                }
            }
        }
    }
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
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = yes;
    }else
    {   H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
        H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
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
