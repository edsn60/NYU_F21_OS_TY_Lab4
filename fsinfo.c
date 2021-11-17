//
// Created by ysw on 11/17/21.
//
#include <stdio.h>
#include "fsinfo.h"

extern void* fs_image;


void print_fs_info(){
    BootEntry *boot_entry = fs_image;
    printf("Number of FATs = %d\n", boot_entry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", boot_entry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", boot_entry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", boot_entry->BPB_RsvdSecCnt);
}


void print_file_info(DirEntry *dir_entry){
    unsigned char *dir_name = dir_entry->DIR_Name;
    unsigned char c;
    for (int i = 0; i < 8; i++){
        c = *(dir_name+i);
        if (c != 32){
            printf("%c", c);
        }
    }
    if (dir_name[8] != 32) {
        printf(".");
        for (int i = 8; i < 11; i++) {
            c = *(dir_name + i);
            if (c != 32) {
                printf("%c", c);
            }
        }
    }
    unsigned int start_cluster = dir_entry->DIR_FstClusHI;
    start_cluster = start_cluster << 16;
    start_cluster += dir_entry->DIR_FstClusLO;
    printf(" (size = %u, starting cluster = %d)\n", dir_entry->DIR_FileSize, start_cluster);
}


void print_dir_info(DirEntry *dir_entry){
    unsigned char *dir_name = dir_entry->DIR_Name;
    unsigned char c;
    for (int i = 0; i < 11; i++){
        c = *(dir_name+i);
        if (c != 32){
            printf("%c", c);
        }
    }
    unsigned int start_cluster = dir_entry->DIR_FstClusHI;
    start_cluster = start_cluster << 16;
    start_cluster += dir_entry->DIR_FstClusLO;
    printf("/ (size = 0, starting cluster = %d)\n", start_cluster);
}


void list_root_dir(){
    BootEntry *boot_entry = fs_image;
    unsigned int root_location = boot_entry->BPB_RootClus;
    root_location -= 2;
    void *root_dir_addr = fs_image + (boot_entry->BPB_RsvdSecCnt + boot_entry->BPB_NumFATs * boot_entry->BPB_FATSz32 + root_location * boot_entry->BPB_SecPerClus) * boot_entry->BPB_BytsPerSec;
    DirEntry *root_entry = (DirEntry*) root_dir_addr;
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    size_t actual_entries_num = 0;

    for (size_t i = 0; i < max_entries_num; i++){
        unsigned char *dir_name = root_entry->DIR_Name;
        if (*dir_name != 0x00 && *dir_name != 0xe5){
            actual_entries_num++;
            if (root_entry->DIR_Attr & 0x10){
                print_dir_info(root_entry);
            } else{
                print_file_info(root_entry);
            }
        }
        root_entry++;
    }
    printf("Total number of entries = %lu\n", actual_entries_num);
}