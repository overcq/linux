/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         header
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
#define H_oux_E_fs_Q_device_S_ident "OUXFS"
#define H_oux_E_fs_S_sector_size    4096
//==============================================================================
#define H_oux_J_min(a,b)            ( (a) > (b) ? (b) : (a) )
#define H_oux_J_max(a,b)            ( (a) < (b) ? (b) : (a) )
#define H_oux_J_align_up_p(p,t)     (( void * )((( uint64_t )(p) + sizeof(t) - 1 ) % sizeof(t) ))
//==============================================================================
struct H_oux_E_fs_Z_block
{ uint64_t sector;
  uint64_t n;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Z_file
{ uint64_t uid;
  uint64_t parent;
  struct
  { uint64_t start;
    uint64_t n;
  }block_table;
  uint64_t size;
  char *name;
  int lock_pid;
  unsigned lock_read    :1;
};
struct H_oux_E_fs_Z_directory
{ uint64_t uid;
  uint64_t parent;
  char *name;
};
/******************************************************************************/
