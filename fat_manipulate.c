//
// Created by ysw on 11/17/21.
//

#include "fat_manipulate.h"
#include "fsinfo.h"


extern void* fs_image;


unsigned int get_fat_index_start_addr(){
    BootEntry *boot_entry = (BootEntry*)fs_image;
    return *(unsigned int*)(fs_image + boot_entry->BPB_RsvdSecCnt * boot_entry->BPB_BytsPerSec);
}