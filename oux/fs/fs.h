/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         header
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
struct H_oux_E_fs_Z_block
{ uint64_t sector;
  uint64_t size;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Z_file
{ uint64_t uid;
  uint64_t parent;
  struct
  { uint64_t start;
    uint64_t count;
  }block_table;
  uint64_t size;
  char *name;
  unsigned lock_write   :1;
  unsigned lock_read    :1;
};
struct H_oux_E_fs_Z_directory
{ uint64_t uid;
  uint64_t parent;
  char *name;
};
/******************************************************************************/
