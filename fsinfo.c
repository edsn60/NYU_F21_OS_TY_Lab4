//
// Created by ysw on 11/17/21.
//
#include <stdio.h>
#include <stdlib.h>
#include "fsinfo.h"
#include "fat_manipulate.h"

extern void* fs_image;
extern BootEntry *boot_entry;


unsigned int *get_root_clusters(){
    unsigned int num_root_clusters = 1;
    unsigned int *root_clusters = (unsigned int *) malloc(sizeof(unsigned int) * (num_root_clusters + 1));
    unsigned int *fat_start_addr = get_fat_start_addr();
    unsigned int *root_cluster_in_fat = fat_start_addr + boot_entry->BPB_RootClus;
    root_clusters[0] = boot_entry->BPB_RootClus;
    while (*root_cluster_in_fat < 0x0ffffff8){
        root_clusters[num_root_clusters] = *root_cluster_in_fat;
        num_root_clusters++;
        root_clusters = realloc(root_clusters, sizeof(unsigned int) * (num_root_clusters + 1));
        root_cluster_in_fat = fat_start_addr + *root_cluster_in_fat;
    }
    root_clusters[num_root_clusters] = 0;
    return root_clusters;
}


void *get_data_start_addr(){
    unsigned int root_location = boot_entry->BPB_RootClus;
    root_location -= 2;
    return (unsigned char*)fs_image + (boot_entry->BPB_RsvdSecCnt + boot_entry->BPB_NumFATs * boot_entry->BPB_FATSz32 + root_location * boot_entry->BPB_SecPerClus) * boot_entry->BPB_BytsPerSec;
}


unsigned int get_cluster_id(DirEntry *dir_entry){
    unsigned int cluster_id = dir_entry->DIR_FstClusHI;
    cluster_id = cluster_id << 16;
    cluster_id += dir_entry->DIR_FstClusLO;
    return cluster_id;
}


void print_fs_info(){
    printf("Number of FATs = %d\n", boot_entry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", boot_entry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", boot_entry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", boot_entry->BPB_RsvdSecCnt);
    printf("FAT size = %u sectors\n", boot_entry->BPB_FATSz32);
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
    unsigned int start_cluster = get_cluster_id(dir_entry);
    printf("/ (size = 0, starting cluster = %d)\n", start_cluster);
}


void list_root_dir(){
    unsigned int *root_clusters = get_root_clusters();
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    unsigned char *data_start_addr = get_data_start_addr();
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    size_t actual_entries_num = 0;
    unsigned int *root_cluster = root_clusters;
    while (*root_cluster != 0){
        DirEntry *root_entry = (DirEntry*) (data_start_addr + (*root_cluster - 2) * cluster_size);
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
        root_cluster++;
    }
    printf("Total number of entries = %lu\n", actual_entries_num);
}