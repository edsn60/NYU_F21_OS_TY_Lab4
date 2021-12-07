//
// Created by ysw on 11/17/21.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <openssl/sha.h>
#include "fsinfo.h"
#include "fat_manipulate.h"

#define SHA_DIGEST_LENGTH 20

extern void* fs_image;
extern BootEntry *boot_entry;
extern size_t image_size;


DirEntry *locate_entry(unsigned char dirname[11], char *filename){
    unsigned int *root_clusters = get_root_clusters();
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    unsigned char *data_start_addr = get_data_start_addr();
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    int deleted_file_count = 0;
    DirEntry *target;
    unsigned int *root_cluster = root_clusters;
    while (*root_cluster != 0) {
        DirEntry *root_entry = (DirEntry*) (data_start_addr + (*root_cluster - 2) * cluster_size);
        for (size_t i = 0; i < max_entries_num; i++) {
            unsigned char *dir_name = root_entry->DIR_Name;
            if (memcmp(dirname, dir_name, sizeof(unsigned char) * 11) == 0) {
                if (deleted_file_count == 0) {
                    target = root_entry;
                    deleted_file_count++;
                } else {
                    printf("%s: multiple candidates found\n", filename);
                    abort();
                }
            }
            root_entry++;
        }
        root_cluster++;
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


void recover_contiguous_fat(DirEntry *dir_entry){
    unsigned int file_size = dir_entry->DIR_FileSize;
    unsigned int file_start_cluster = get_cluster_id(dir_entry);
    unsigned int fat_num = boot_entry->BPB_NumFATs;
    unsigned int fat_size = boot_entry->BPB_FATSz32 * boot_entry->BPB_BytsPerSec;
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;

    unsigned int *fat_addr = get_fat_start_addr();
    for (unsigned int i = 0; i < fat_num; i++) {
        unsigned int *file_start_cluster_in_fat = fat_addr + i * fat_size / sizeof(unsigned int);
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
    unsigned char* deleted_dirname = (unsigned char*)reformat_deleted_dirname(filename);
    deleted_dirname[0] = 0xe5;
    DirEntry * target = locate_entry(deleted_dirname, filename);
    if (target){
        *(target->DIR_Name) = *filename;
        recover_contiguous_fat(target);
        munmap(fs_image, image_size);
        printf("%s: successfully recovered\n", filename);
    }
}


char *compute_sha1(char *file_content, size_t file_size){
    unsigned char md[SHA_DIGEST_LENGTH] = {'\0'};
    SHA1((const unsigned char*)file_content, file_size, md);
    char *file_sha1 = (char*) malloc(sizeof(char) * SHA_DIGEST_LENGTH * 2);
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++){
        sprintf(&file_sha1[i*2], "%02x", (unsigned int)md[i]);
    }
    return file_sha1;
}


DirEntry *compare_sha1_for_contiguous_file(DirEntry *dir_entry, char *target_sha1){
    unsigned int file_start_cluster_id = get_cluster_id(dir_entry);
    size_t file_size = dir_entry->DIR_FileSize;

    char *data_start_addr = get_data_start_addr();
    char *file_start_addr = data_start_addr + (file_start_cluster_id - 2) * boot_entry->BPB_SecPerClus * boot_entry->BPB_BytsPerSec;

    char *file_sha1 = compute_sha1(file_start_addr, file_size);

    if (memcmp(target_sha1, file_sha1, SHA_DIGEST_LENGTH * 2) == 0){
        return dir_entry;
    }
    else{
        return NULL;
    }
}


void recover_file_with_sha1(char *filename, char* sha1){
    unsigned char* deleted_dirname = (unsigned char*)reformat_deleted_dirname(filename);
    deleted_dirname[0] = 0xe5;
    unsigned int *root_clusters = get_root_clusters();
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    unsigned char *data_start_addr = get_data_start_addr();
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    unsigned int *root_cluster = root_clusters;
    while (*root_cluster != 0) {
        DirEntry *dir_entry = (DirEntry*) (data_start_addr + (*root_cluster - 2) * cluster_size);
        for (size_t i = 0; i < max_entries_num; i++) {
            unsigned char *dir_name = dir_entry->DIR_Name;
            if (memcmp(deleted_dirname, dir_name, sizeof(unsigned char) * 11) == 0) {
                if (compare_sha1_for_contiguous_file(dir_entry, sha1)) {
                    recover_contiguous_fat(dir_entry);
                    *dir_entry->DIR_Name = *filename;
                    printf("%s: successfully recovered with SHA-1\n", filename);
                    return;
                }
            }
            dir_entry++;
        }
        root_cluster++;
    }
    printf("%s: file not found\n", filename);
}


char *get_file_sha1_by_clusters(const unsigned int *clusters, size_t cluster_num, size_t file_size){
    unsigned char *data_start_addr = get_data_start_addr();
    unsigned int cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    unsigned char md[SHA_DIGEST_LENGTH] = {'\0'};
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    char *file_content = (char*) malloc(sizeof(char) * (file_size));
    memset(file_content, '\0', sizeof(char) * (file_size));
    for (size_t i = 0; i < cluster_num - 1; i++){
        strncat(file_content, (char*)data_start_addr + (clusters[i] - 2) * cluster_size, cluster_size);
        SHA1_Update(&ctx, (char*)data_start_addr + (clusters[i] - 2) * cluster_size, cluster_size);
        file_size -= cluster_size;
    }
    strncat(file_content, (char*)data_start_addr + (clusters[cluster_num - 1] - 2) * cluster_size, file_size);

    SHA1_Update(&ctx, (char*)data_start_addr + (clusters[cluster_num - 1] - 2) * cluster_size, file_size);
    SHA1_Final(md, &ctx);

    char *file_sha1 = (char*) malloc(sizeof(char) * (SHA_DIGEST_LENGTH * 2 + 1));
    memset(file_sha1, 0, sizeof(char) * (SHA_DIGEST_LENGTH * 2 + 1));
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++){
        sprintf(&file_sha1[i*2], "%02x", (unsigned int)md[i]);
    }
    free(file_content);
    return file_sha1;
}


int find_element_in_array(const unsigned int *clusters, unsigned int cluster_num, unsigned int target){
    for (unsigned int i = 0; i < cluster_num; i++){
        if (*(clusters+i) == target){
            return 1;
        }
    }
    return 0;
}


unsigned int *brute_force_search(unsigned int *all_clusters, unsigned int *searched_clusters, unsigned int start, unsigned int end, unsigned int remain, char *target_sha1, size_t file_size, size_t cluster_num){
    if (start == end && remain > 1){
        return NULL;
    }
    if (remain == 0){
        char *file_sha1 = get_file_sha1_by_clusters(searched_clusters, cluster_num, file_size);
        if (memcmp(target_sha1, file_sha1, SHA_DIGEST_LENGTH * 2) == 0){
            return searched_clusters;
        }
        else{
            return NULL;
        }

    }
    unsigned int *fat_addr = (unsigned int*)fs_image + (boot_entry->BPB_RsvdSecCnt * boot_entry->BPB_BytsPerSec) / sizeof(unsigned int);
    unsigned int *searched_res;
    for (unsigned int i = 0; i <= end; i++){
        if (*(fat_addr + all_clusters[i]) == 0 && !find_element_in_array(searched_clusters, cluster_num-remain, all_clusters[i])){
            searched_clusters[cluster_num - remain] = all_clusters[i];
            searched_res = brute_force_search(all_clusters, searched_clusters, i+1, end, remain-1, target_sha1, file_size, cluster_num);
            if (searched_res){
                return searched_clusters;
            }
        }
    }
    return NULL;
}


void recover_non_contiguous_file_with_sha1(char* filename, char *sha1){
    unsigned char* deleted_dirname = (unsigned char*)reformat_deleted_dirname(filename);
    deleted_dirname[0] = 0xe5;
    unsigned int *root_clusters = get_root_clusters();
    char *data_start_addr = get_data_start_addr();
    size_t max_entries_num = boot_entry->BPB_BytsPerSec / sizeof(DirEntry) * boot_entry->BPB_SecPerClus;
    size_t cluster_size = boot_entry->BPB_BytsPerSec * boot_entry->BPB_SecPerClus;
    unsigned int all_clusters[12] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    unsigned int *root_cluster = root_clusters;
    while (*root_cluster != 0) {
        DirEntry *dir_entry = (DirEntry*) (data_start_addr + (*root_cluster - 2) * cluster_size);
        for (size_t i = 0; i < max_entries_num; i++) {
            unsigned char *dir_name = dir_entry->DIR_Name;
            if (memcmp(deleted_dirname, dir_name, sizeof(unsigned char) * 11) == 0) {
                size_t file_size = dir_entry->DIR_FileSize;
                unsigned int file_start_cluster_id = get_cluster_id(dir_entry);
                char *file_start_addr = data_start_addr + (file_start_cluster_id - 2) * cluster_size;
                if (file_size > cluster_size) {
                    size_t cluster_num;
                    if (file_size % cluster_size == 0) {
                        cluster_num = file_size / cluster_size;
                    } else {
                        cluster_num = file_size / cluster_size + 1;
                    }
                    unsigned int *clusters = (unsigned int *) malloc(sizeof(unsigned int) * cluster_num);
                    clusters[0] = file_start_cluster_id;

                    unsigned int *searched_clusters = (unsigned int *) malloc(sizeof(unsigned int) * cluster_num);
                    searched_clusters[0] = file_start_cluster_id;
                    searched_clusters = brute_force_search(all_clusters, searched_clusters, file_start_cluster_id - 3,
                                                           11, cluster_num - 1, sha1, file_size, cluster_num);
                    if (searched_clusters) {
                        recover_fats(searched_clusters, cluster_num);
                        *dir_entry->DIR_Name = *filename;
                        printf("%s: successfully recovered with SHA-1\n", filename);
                        return;
                    }
                } else {
                    char *file_sha1 = compute_sha1(file_start_addr, file_size);
                    if (memcmp(sha1, file_sha1, SHA_DIGEST_LENGTH * 2) == 0) {
                        recover_fats(&file_start_cluster_id, 1);
                        *dir_entry->DIR_Name = *filename;
                        printf("%s: successfully recovered with SHA-1\n", filename);
                        return;
                    }
                }
            }
            dir_entry++;
        }
        root_cluster++;
    }
    printf("%s: file not found\n", filename);
}