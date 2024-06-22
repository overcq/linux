/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         read and write
* ©overcq                on ‟Gentoo Linux 23.0” “x86_64”             2024‒6‒22 P
*******************************************************************************/
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
//------------------------------------------------------------------------------
#include "../lang.h"
#include "fs.h"
//==============================================================================
extern struct H_oux_E_fs_Q_device_Z *H_oux_E_fs_Q_device_S;
extern unsigned H_oux_E_fs_Q_device_S_n;
//==============================================================================
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
//------------------------------------------------------------------------------
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
/******************************************************************************/
