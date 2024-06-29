/*******************************************************************************
*   ___   public
*  ¦OUX¦  C
*  ¦/C+¦  Linux kernel subsystem
*   ---   OUX
*         language
* ©overcq                on ‟Gentoo Linux 17.1” “x86_64”             2024‒3‒23 P
*******************************************************************************/
#define no                          false
#define yes                         true
#define O                           while(yes)
//==============================================================================
#define H_oux_J_min(a,b)            ( (a) > (b) ? (b) : (a) )
#define H_oux_J_max(a,b)            ( (a) < (b) ? (b) : (a) )
#define H_oux_J_align_up_p(p,t)     (( void * )(( uint64_t )(p) + sizeof(t) - 1 - (( uint64_t )(p) + sizeof(t) - 1 ) % sizeof(t) ))
/******************************************************************************/
