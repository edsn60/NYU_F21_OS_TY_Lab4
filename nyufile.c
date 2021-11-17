#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define USAGE_MESSAGE "Usage: ./nyufile disk <options>\n-i                     Print the file system information.\n-l                     List the root directory.\n-r filename [-s sha1]  Recover a contiguous file.\n-R filename -s sha1    Recover a possibly non-contiguous file.\n"


void print_usage_message(){
    fprintf(stderr, USAGE_MESSAGE);
    exit(-1);
}


int main(int argc, char *const argv[]) {
    if (argc < 3 || argc > 6) {
        print_usage_message();
    }
    char *disk_image_name = argv[1];
    char *filename;
    int status;
    optind = 2;
    status = getopt(argc, argv, "ilr:R:");
    if (status != -1) {
        switch (status) {
            case 105 :   // -i
                if (argc == 3 && optind == 3) {
                    // TODO::
                } else {
                    print_usage_message();
                }
                break;
            case 108:   // l
                if (argc == 3 && optind == 3) {
                    // TODO::
                } else {
                    print_usage_message();
                }
                break;
            case 114:   // r
                filename = optarg;
                if (optind == 4) {
                    if (argc == 4) {
                        // TODO::
                    } else if (argc == 6 && strcmp(argv[4], "-s") == 0 && strcmp(argv[5], "sha1") == 0) {
                        // TODO::
                    } else {
                        print_usage_message();
                    }
                }
                break;
            case 82:    // R
                filename = optarg;
                if (optind == 4 && argc == 6 && strcmp(argv[4], "-s") == 0 && strcmp(argv[5], "sha1") == 0) {
                    // TODO::
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