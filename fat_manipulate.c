//
// Created by ysw on 11/17/21.
//
#include <stdio.h>
#include "fsinfo.h"


extern void* fs_image;
extern BootEntry *boot_entry;

unsigned int *get_fat_start_addr(){
    return (unsigned int*)fs_image + (boot_entry->BPB_RsvdSecCnt * boot_entry->BPB_BytsPerSec) / sizeof(unsigned int);
}


void recover_fats(unsigned int *clusters, unsigned int cluster_num){
    unsigned int *fat_start_addr = get_fat_start_addr();
    unsigned int fat_num = boot_entry->BPB_NumFATs;
    unsigned int fat_size = boot_entry->BPB_FATSz32 * boot_entry->BPB_BytsPerSec;
    for (unsigned int fat_id = 0; fat_id < fat_num; fat_id++){
        for (unsigned cluster_idx = 0; cluster_idx < cluster_num - 1; cluster_idx++){
            *(fat_start_addr + clusters[cluster_idx]) = clusters[cluster_idx + 1];
        }
        *(fat_start_addr + clusters[cluster_num - 1]) = EOF;
        fat_start_addr += fat_size / sizeof(unsigned int);
    }
}