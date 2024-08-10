/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         init
* ©overcq                on ‟Gentoo Linux 23.0” “x86_64”             2024‒6‒22 O
*******************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
//==============================================================================
#include "../lang.h"
#include "fs.h"
//==============================================================================
const gfp_t E_oux_E_fs_S_kmalloc_flags = GFP_KERNEL;
rwlock_t E_oux_E_fs_S_rw_lock;
struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
unsigned H_oux_E_fs_Q_device_S_n;
uint16_t H_oux_E_fs_Q_block_table_S_first_sector_max_size;
//==============================================================================
static
int
__init
H_oux_E_fs_M( void
){  E_oux_E_fs_S_rw_lock = __RW_LOCK_UNLOCKED( E_oux_E_fs_S_rw_lock );
    H_oux_E_fs_Q_device_S_n = 0;
    H_oux_E_fs_Q_device_S = kmalloc_array( H_oux_E_fs_Q_device_S_n, sizeof( *H_oux_E_fs_Q_device_S ), E_oux_E_fs_S_kmalloc_flags );
    if( !H_oux_E_fs_Q_device_S )
        return -ENOMEM;
    uint64_t *block_table_n = H_oux_J_align_up_p(( char * )sizeof( H_oux_E_fs_Q_device_S_ident ) - 1, uint64_t );
    H_oux_E_fs_Q_block_table_S_first_sector_max_size = ( uint64_t )&block_table_n[8];
    return 0;
}
static
void
__exit
H_oux_E_fs_W( void
){  kfree( H_oux_E_fs_Q_device_S );
}
//==============================================================================
MODULE_DESCRIPTION( "OUX filesystem" );
MODULE_AUTHOR( "Janusz Augustyński <overcq@linux.pl>" );
MODULE_LICENSE( "GPL" );
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
