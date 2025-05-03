/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         init
* ©overcq                on ‟Gentoo Linux 23.0” “x86_64”             2024‒6‒22 O
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
//==============================================================================
#include "../lang.h"
#include "fs.h"
//==============================================================================
struct rw_semaphore E_oux_E_fs_S_rw_lock;
void *H_oux_E_fs_Q_device_S_holder;
struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
static
int
__init
H_oux_E_fs_M( void
){  E_oux_E_fs_S_rw_lock = __RW_LOCK_UNLOCKED( E_oux_E_fs_S_rw_lock );
    H_oux_E_fs_Q_device_S_holder = kmalloc( 0, E_oux_E_fs_S_alloc_flags );
    if( !H_oux_E_fs_Q_device_S_holder )
        return -ENOMEM;
    H_oux_E_fs_Q_device_S_n = 0;
    H_oux_E_fs_Q_device_S = kmalloc_array( H_oux_E_fs_Q_device_S_n, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_alloc_flags );
    if( !H_oux_E_fs_Q_device_S )
        return -ENOMEM;
    return 0;
}
static
void
__exit
H_oux_E_fs_W( void
){  for( unsigned device_i = 0; device_i != H_oux_E_fs_Q_device_S_n; device_i++ )
    {   H_oux_E_fs_Q_device_I_save( device_i );
        for( uint64_t file_i = 0; file_i != H_oux_E_fs_Q_device_S[ device_i ].file_n; file_i++ )
            kfree( H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name );
        kfree( H_oux_E_fs_Q_device_S[ device_i ].file );
        for( uint64_t directory_i = 0; directory_i != H_oux_E_fs_Q_device_S[ device_i ].directory_n; directory_i++ )
            kfree( H_oux_E_fs_Q_device_S[ device_i ].directory[ directory_i ].name );
        kfree( H_oux_E_fs_Q_device_S[ device_i ].directory );
        kfree( H_oux_E_fs_Q_device_S[ device_i ].block_table );
        filp_close( H_oux_E_fs_Q_device_S[ device_i ].bdev_file, 0 );
    }
    kfree( H_oux_E_fs_Q_device_S );
    kfree( H_oux_E_fs_Q_device_S_holder );
}
//==============================================================================
MODULE_DESCRIPTION( "OUX filesystem" );
MODULE_AUTHOR( "Janusz Augustyński <overcq@int.pl>" );
MODULE_LICENSE( "GPL" );
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
