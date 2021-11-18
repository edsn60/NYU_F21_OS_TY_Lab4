#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "fsinfo.h"
#include "file_recovery.h"

#define USAGE_MESSAGE "Usage: ./nyufile disk <options>\n-i                     Print the file system information.\n-l                     List the root directory.\n-r filename [-s sha1]  Recover a contiguous file.\n-R filename -s sha1    Recover a possibly non-contiguous file.\n"


void *fs_image;
BootEntry *boot_entry;
size_t image_size;


void print_usage_message(){
    fprintf(stderr, USAGE_MESSAGE);
    exit(-1);
}


void map_fs_image(char *disk_image_name){
    struct stat st;
    int fd = open(disk_image_name, O_RDWR);
    if (fd == -1){
        fprintf(stderr, "Error: %s, no such file\n", disk_image_name);
    }
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Error: fstat error\n");
        exit(-1);
    }
    image_size = st.st_size;
    fs_image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    boot_entry = (BootEntry*)fs_image;
    if (fs_image == MAP_FAILED){
        fprintf(stderr, "Error, mmap failed: %d: %s\n", errno, strerror(errno));
        abort();
    }
}


int main(int argc, char *const argv[]) {
    if (argc < 3 || argc > 6) {
        print_usage_message();
    }
    char *disk_image_name = argv[1];
    char *filename;
    unsigned char *sha1;
    map_fs_image(disk_image_name);
    int status;
    optind = 2;
    status = getopt(argc, argv, ":ilr:R:");
    if (status != -1) {
        switch (status) {
            case 105 :   // -i
                if (argc == 3 && optind == 3) {
                    print_fs_info();
                } else {
                    print_usage_message();
                }
                break;
            case 108:   // l
                if (argc == 3 && optind == 3) {
                    list_root_dir();
                } else {
                    print_usage_message();
                }
                break;
            case 114:   // r
                filename = optarg;
                if (optind == 4) {
                    if (argc == 4) {
                        recover_file_with_name(filename);
                    } else if (argc == 6) {
                        status = getopt(argc, argv, ":s:");
                        if (status == 115){
                            sha1 = (unsigned char*)optarg;
                            recover_file_with_sha1(filename, sha1);
                        }
                        else{
                            print_usage_message();
                        }
                    } else {
                        print_usage_message();
                    }
                }
                break;
            case 82:    // R
                filename = optarg;
                if (optind == 4 && argc == 6) {
                    status = getopt(argc, argv, ":s:");
                    if (status == 115){
                        sha1 = (unsigned char*)optarg;
                        // TODO::
                    }
                    else{
                        print_usage_message();
                    }
                } else {
                    print_usage_message();
                }
                break;
            default:
                print_usage_message();
        }
    } else {
        print_usage_message();
    }
}