//
// Created by ysw on 11/17/21.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <openssl/sha.h>
#include "file_recovery.h"
#include "fsinfo.h"

#define SHA_DIGEST_LENGTH 20

extern void* fs_image;
extern BootEntry *boot_entry;
extern size_t image_size;


DirEntry *locate_entry(unsigned char dirname[11], char *filename){
    void *root_dir_addr = get_root_dir_addr();
    DirEntry *root_entry = (DirEntry*) root_dir_addr;
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    int deleted_file_count = 0;
    DirEntry *target;
    for (size_t i = 0; i < max_entries_num; i++){
        unsigned char *dir_name = root_entry->DIR_Name;
        if (memcmp(dirname, dir_name, sizeof(unsigned char) * 11) == 0){
            if (deleted_file_count == 0){
                target = root_entry;
                deleted_file_count++;
            }
            else{
                printf("%s: multiple candidates found\n", filename);
                abort();
            }
        }
        root_entry++;
    }
    if (deleted_file_count == 0){
        printf("%s: file not found\n", filename);
        return NULL;
    }
    return target;
}


char* reformat_deleted_dirname(char *filename){
    char *new_filename = (char*) malloc(sizeof(char) * 11);
    strcpy(new_filename, filename);
    char *dirname = (char*) malloc(sizeof(char) * 11);
    memset(dirname, ' ', 11);
    char *filename_prefix;
    char *file_extension = "";
    filename_prefix = strtok_r(new_filename, ".", &file_extension);
    strcpy(dirname, filename_prefix);
    dirname[strlen(filename_prefix)] = 32;
    strcpy(dirname+8, file_extension);
    size_t file_extension_len = strlen(file_extension);
    if (file_extension_len != 3){
        dirname[8 + file_extension_len] = 32;
    }
    return dirname;
}


void contiguous_file(DirEntry *dir_entry){
    unsigned int file_size = dir_entry->DIR_FileSize;
    unsigned int file_start_cluster = get_cluster_id(dir_entry);
    unsigned int fat_num = boot_entry->BPB_NumFATs;
    unsigned int fat_size = boot_entry->BPB_FATSz32 * boot_entry->BPB_BytsPerSec;
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;

    unsigned int *fat_addr = (unsigned int*)(fs_image + boot_entry->BPB_RsvdSecCnt * boot_entry->BPB_BytsPerSec);
    for (unsigned int i = 0; i < fat_num; i++) {
        printf("sizeof void*: %lu\n", sizeof(void*));
        printf("sizeof unsigned int: %lu\n", sizeof(unsigned int*));
        unsigned int *file_start_cluster_in_fat = fat_addr + i * fat_size / sizeof(unsigned int);
        printf("%p\n", file_start_cluster_in_fat);
        file_start_cluster_in_fat += file_start_cluster;
        unsigned int tmp_file_start_cluster = file_start_cluster;
        unsigned int tmp_file_size = file_size;
        while (tmp_file_size > cluster_size) {
            *file_start_cluster_in_fat = ++tmp_file_start_cluster;
            file_start_cluster_in_fat++;
            tmp_file_size -= cluster_size;
        }
        *file_start_cluster_in_fat = EOF;
    }
}


void recover_file_with_name(char* filename){
    unsigned char* dirname = (unsigned char*)reformat_deleted_dirname(filename);
    dirname[0] = 0xe5;
    DirEntry * target = locate_entry(dirname, filename);
    if (target){
        *(target->DIR_Name) = *filename;
        if (target->DIR_FileSize > boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus){
            contiguous_file(target);
        }
        munmap(fs_image, image_size);
        printf("%s: successfully recovered\n", filename);
    }
}


DirEntry *compute_sha1_for_contiguous_file(DirEntry *dir_entry, unsigned char *target_sha1){
    unsigned int file_start_cluster_id = get_cluster_id(dir_entry);
    unsigned int *fat_addr = (unsigned int*)(fs_image + boot_entry->BPB_RsvdSecCnt * boot_entry->BPB_BytsPerSec);
    unsigned int file_size = dir_entry->DIR_FileSize;
    char *file_content = (char*) malloc(sizeof(char) * file_size);
    memset(file_content, '\0', sizeof(unsigned char) * file_size);
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    void *root_dir_addr = get_root_dir_addr();
    char *file_start_addr = root_dir_addr + (file_start_cluster_id -2) * boot_entry->BPB_SecPerClus * boot_entry->BPB_BytsPerSec;
    while (file_size > cluster_size){
        strncat(file_content, file_start_addr, cluster_size);
        file_start_addr += boot_entry->BPB_SecPerClus * boot_entry->BPB_BytsPerSec;
        file_size -= cluster_size;
    }
    strncat(file_content, file_start_addr, file_size);
    unsigned char file_sha1[SHA_DIGEST_LENGTH + 1];
    file_sha1[SHA_DIGEST_LENGTH] = '\0';
    SHA1((unsigned char*)file_content, SHA_DIGEST_LENGTH, file_sha1);
    if (memcmp(target_sha1, file_sha1, SHA_DIGEST_LENGTH) == 0){
        return dir_entry;
    }
    else{
        return NULL;
    }
}


void recover_file_with_sha1(char *filename, unsigned char* sha1){
    unsigned char* dirname = (unsigned char*)reformat_deleted_dirname(filename);
    dirname[0] = 0xe5;
    void *root_dir_addr = get_root_dir_addr();
    DirEntry *dir_entry = (DirEntry*) root_dir_addr;
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    for (size_t i = 0; i < max_entries_num; i++){
        unsigned char *dir_name = dir_entry->DIR_Name;
        if (memcmp(dirname, dir_name, sizeof(unsigned char) * 11) == 0){
                if(compute_sha1_for_contiguous_file(dir_entry, sha1)){
                    contiguous_file(dir_entry);
                    *dir_entry->DIR_Name = *filename;
                    printf("%s: successfully recovered with SHA-1\n", filename);
                    return;
                }
        }
        dir_entry++;
    }
    printf("%s: file not found\n", filename);
}