  /*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX filesystem
*         header
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒1‒29 L
*******************************************************************************/
//DFN Rozmiary fragmentów tablicy bloków na dysku zawsze są ustawione tak, że wpisy wypełniają je całe.
//DFN Tablice bloków na dysku są posortowane według sektora, a następnie według lokalizacji w sektorze. Tablice plików i katalogów – według ‘uid’.
//DFN Katalog główny w “parent” ma wartość “~0”.
//==============================================================================
#define H_oux_E_fs_Q_device_S_ident "OUXFS"
#define H_oux_E_fs_S_sector_size    4096
//==============================================================================
enum H_oux_E_Fs_Z_block_Z_location
{ H_oux_E_fs_Z_block_Z_location_S_sectors
, H_oux_E_fs_Z_block_Z_location_S_in_sector
};
//DFN Wpis na dysku ma kolejność:
// • sector
// • location_type
//   • n, pre, post
// albo
//   • start, size
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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Q_device_Z
{ struct file *bdev_file;
  struct H_oux_E_fs_Z_block *block_table;
  uint64_t block_table_n;
  uint64_t block_table_changed_from;
  uint64_t block_table_first_sector_size;
  struct H_oux_E_fs_Z_file *file;
  uint64_t file_n;
  uint64_t block_table_file_table_start, block_table_file_table_n;
  uint64_t block_table_file_table_changed_from;
  struct H_oux_E_fs_Z_directory *directory;
  uint64_t directory_n;
  uint64_t block_table_directory_table_start, block_table_directory_table_n;
  uint64_t block_table_directory_table_changed_from;
  struct H_oux_E_fs_Z_block *free_table;
  uint64_t free_table_n;
};
//==============================================================================
int H_oux_E_fs_Q_directory_R( unsigned, uint64_t, uint64_t * );
int H_oux_E_fs_Q_file_R( unsigned, uint64_t, uint64_t * );
/******************************************************************************/
