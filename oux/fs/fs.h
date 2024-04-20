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
enum H_oux_E_Fs_Z_block_Z_location
{ H_oux_E_Fs_Z_block_Z_location_S_sectors
, H_oux_E_Fs_Z_block_Z_location_S_in_sector
};
struct H_oux_E_fs_Z_block
{ uint64_t sector;
  union
  { struct
    { uint64_t n;
      uint16_t pre, post;
    }sectors;
    struct
    { uint16_t start;
      uint16_t size;
    }in_sector;
  }location; //DFN Jeśli “!sectors.n” oraz “!sectors.pre || !sectors.post”, to ustawić “in_sector”.
  enum H_oux_E_Fs_Z_block_Z_location location_type;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Z_file
{ uint64_t uid;
  uint64_t parent;
  struct
  { uint64_t start;
    uint64_t n;
  }block_table;
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
