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
//TODO Co do plików zwykłych pozostaje decyzja, jak ograniczać fagmentację, a nie zużywać nośnika na zapis danych.
//TODO Postarać się zrobić ‘rollback’ operacji, których niepowodzenie obecnie powoduje “inconsistent”.
//==============================================================================
#define H_oux_E_fs_Q_device_S_ident     "OUXFS"
#define E_oux_E_fs_S_alloc_flags        GFP_KERNEL
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
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
      uint16_t pre, post; // Wartości od 0 do “H_oux_E_fs_Q_device_S[ device_i ].sector_size - 1”.
    }sectors;
    struct
    { uint16_t start; // Wartości od 0 do “H_oux_E_fs_Q_device_S[ device_i ].sector_size - 1”.
      uint16_t size; // Wartości od 0 do “H_oux_E_fs_Q_device_S[ device_i ].sector_size - 1”.
    }in_sector;
  }location; //DFN Jeśli “!sectors.n” oraz “!sectors.pre || !sectors.post”, to ustawić “in_sector”.
  enum H_oux_E_Fs_Z_block_Z_location location_type;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Z_file
{ uint64_t uid; // Wartości oprócz “~0”. Klucz sortowania rosnąco.
  uint64_t parent; // “~0” oznacza katalog główny.
  struct
  { uint64_t start;
    uint64_t n;
  }block_table;
  char *name;
  int lock_pid; //DFN Jeśli różny od “~0”, to zablokowany do zapisu.
  unsigned lock_read    :1;
};
struct H_oux_E_fs_Z_directory
{ uint64_t uid; // Wartości oprócz “~0”. Klucz sortowania rosnąco.
  uint64_t parent; // “~0” oznacza katalog główny.
  char *name;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct H_oux_E_fs_Q_device_Z
{ void *holder;
  struct file *bdev_file;
  uint64_t block_table_size;
  struct H_oux_E_fs_Z_block *block_table;
  uint64_t block_table_n;
  uint64_t block_table_changed_from;
  uint64_t block_table_block_table_n;
  struct H_oux_E_fs_Z_directory *directory;
  uint64_t directory_n;
  uint64_t directory_table_changed_from;
  uint64_t block_table_directory_table_start, block_table_directory_table_n;
  struct H_oux_E_fs_Z_file *file;
  uint64_t file_n;
  uint64_t file_table_changed_from;
  uint64_t block_table_file_table_start, block_table_file_table_n;
  struct H_oux_E_fs_Z_block *free_table;
  uint64_t free_table_n;
  uint16_t sector_size;
  uint16_t first_sector_max_size;
  bool inconsistent;
};
//==============================================================================
int H_oux_E_fs_Q_device_I_save(unsigned);
bool H_oux_E_fs_Q_block_T_cross( unsigned, const struct H_oux_E_fs_Z_block *, const struct H_oux_E_fs_Z_block * );
uint64_t H_oux_E_fs_Z_start_n_R_size( unsigned, uint64_t, uint64_t );
uint64_t H_oux_E_fs_Q_free_table_R( unsigned, uint64_t );
int H_oux_E_fs_Q_directory_R( unsigned, uint64_t, uint64_t * );
int H_oux_E_fs_Q_file_R( unsigned, uint64_t, uint64_t * );
int H_oux_E_fs_Q_file_Q_block_table_I_unite( unsigned, uint64_t, uint64_t, uint64_t );
int H_oux_E_fs_Q_block_table_I_unite( unsigned, uint64_t *, uint64_t *, uint64_t, uint64_t, int64_t * );
int H_oux_E_fs_Q_free_table_I_unite( unsigned, const struct H_oux_E_fs_Z_block * );
int H_oux_E_fs_Q_directory_file_I_block_append( unsigned, uint64_t, uint64_t *, uint64_t *, uint64_t * );
int H_oux_E_fs_Q_block_table_I_append_truncate( unsigned, int64_t );
int H_oux_E_fs_Z_start_n_I_block_append( unsigned, uint64_t, uint64_t *, uint64_t *, uint64_t *, int64_t *, uint64_t, uint64_t * );
int H_oux_E_fs_Z_start_n_I_block_truncate( unsigned, uint64_t, uint64_t, uint64_t *, int64_t * );
/******************************************************************************/
