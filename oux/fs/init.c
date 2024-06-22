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
struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
static
int
__init
H_oux_E_fs_M( void
){  H_oux_E_fs_Q_device_S_n = 0;
    H_oux_E_fs_Q_device_S = kmalloc_array( H_oux_E_fs_Q_device_S_n, sizeof( *H_oux_E_fs_Q_device_S ), GFP_KERNEL );
    return H_oux_E_fs_Q_device_S ? 0 : -ENOMEM;
}
static
void
__exit
H_oux_E_fs_W( void
){  kfree( H_oux_E_fs_Q_device_S );
}
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
