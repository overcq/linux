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
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_I_list, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, uint64_t __user *, list
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
SYSCALL_DEFINE4( H_oux_E_fs_Q_directory_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
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
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_R_name, unsigned, device_i, uint64_t, uid, uint64_t __user *, n, char __user *, name
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
SYSCALL_DEFINE3( H_oux_E_fs_Q_directory_M, unsigned, device_i, uint64_t, parent, const char __user *, name
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
SYSCALL_DEFINE2( H_oux_E_fs_Q_directory_W, unsigned, device_i, uint64_t, uid
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
SYSCALL_DEFINE3( H_oux_E_fs_Q_file_M, unsigned, device_i, uint64_t, parent, const char __user *, name
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
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.start = H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].block_table.n = 0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].name = name_;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_pid = ~0;
    H_oux_E_fs_Q_device_S[ device_i ].file[ file_i ].lock_read = no;
Error_1:
    kfree( name_ );
Error_0:
    write_unlock( &E_oux_E_fs_S_rw_lock );
    return error;
}
SYSCALL_DEFINE2( H_oux_E_fs_Q_file_W, unsigned, device_i, uint64_t, uid
){  write_lock( &E_oux_E_fs_S_rw_lock );
    uint64_t file_i;
    int error = H_oux_E_fs_Q_file_R( device_i, uid, &file_i );
    if(error)
        goto Error_0;
    //TODO Zwolnić bloki, dopisać do tablicy wolnych bloków.
    
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
