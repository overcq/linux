/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         main
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/syscalls.h>
#include "fs.h"
//==============================================================================
uint64_t H_oux_E_fs_S_file_table_start;
//------------------------------------------------------------------------------
struct H_oux_E_fs_Z_block *H_oux_E_fs_S_block_table;
uint64_t H_oux_E_fs_S_block_table_n;
//------------------------------------------------------------------------------
struct H_oux_E_fs_Z_file *H_oux_E_fs_S_file;
uint64_t H_oux_E_fs_S_file_n;
struct H_oux_E_fs_Z_directory *H_oux_E_fs_S_directory;
uint64_t H_oux_E_fs_S_directory_n;
//==============================================================================
static
int
H_oux_E_fs_Q_fs_context_I_fill_super( struct super_block *sb
, struct fs_context *fc
){  
    return 0;
}
static
int
H_oux_E_fs_Q_fs_context_I_get_tree( struct fs_context *fc
){  return get_tree_bdev( fc, H_oux_E_fs_Q_fs_context_I_fill_super );
}
static
void
H_oux_E_fs_Q_fs_context_I_free( struct fs_context *fc
){
}
//------------------------------------------------------------------------------
static const
struct fs_context_operations
H_oux_E_fs_S_context_ops =
{ .get_tree = H_oux_E_fs_Q_fs_context_I_get_tree
, .free = H_oux_E_fs_Q_fs_context_I_free
};
//------------------------------------------------------------------------------
static
int
H_oux_E_fs_Q_type_I_init_fs_context( struct fs_context *fc
){  fc->ops = &H_oux_E_fs_S_context_ops;
    return 0;
}
static
void
H_oux_E_fs_Q_type_I_kill_sb( struct super_block *sb
){  
    kill_block_super(sb);
}
//------------------------------------------------------------------------------
static
struct file_system_type
H_oux_E_fs_S_type =
{ .owner = THIS_MODULE
, .name = "ouxfs"
, .init_fs_context = H_oux_E_fs_Q_type_I_init_fs_context
, .kill_sb = H_oux_E_fs_Q_type_I_kill_sb
};
//------------------------------------------------------------------------------
static
int
__init
H_oux_E_fs_M( void
){  return register_filesystem( &H_oux_E_fs_S_type );
}
static
void
__exit
H_oux_E_fs_W( void
){  unregister_filesystem( &H_oux_E_fs_S_type );
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SYSCALL_DEFINE2( H_oux_E_fs_Q_file_I_lock, uint64_t, uid, int, operation
){  
    return 0;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_I_read, uint64_t, uid, uint64_t, sector, uint64_t, count, char __user *, data
){  
    return 0;
}
SYSCALL_DEFINE4( H_oux_E_fs_Q_file_I_write, uint64_t, uid, uint64_t, sector, uint64_t, count, const char __user *, data
){  
    return 0;
}
//==============================================================================
module_init( H_oux_E_fs_M )
module_exit( H_oux_E_fs_W )
/******************************************************************************/
