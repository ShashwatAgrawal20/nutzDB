#include "headers/pager.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "headers/lib.h"

Pager *pager_init() {
    int fd = open(PAGER_FILENAME, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        fprintf(stderr, "Error: Unable to open the pager file %s\n",
                PAGER_FILENAME);
    }
    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager *pager = malloc(sizeof(Pager));
    if (!pager) {
        fprintf(stderr, "Error: Memory allocation failed for pager.\n");
        return NULL;
    }
    for (uint8_t i = 0; i < TABLE_MAX_PAGES; ++i) {
        pager->pages[i] = NULL;
    }
    pager->fd = fd;
    pager->file_length = file_length;
    return pager;
}

void *get_page(Pager *pager, size_t page_num) {
    if (pager->pages[page_num] == NULL) {
        void *page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        // The condition if (pager.length % PAGE_SIZE) checks for a partial page
        // at the end of the data.
        // It evaluates to true when pager.length is not a multiple of
        // PAGE_SIZE, indicating leftover bytes. Example Scenarios:
        // 1. Length = 12288 bytes (3 pages, no remainder): condition is false,
        // num_pages = 3.
        // 2. Length = 13312 bytes (3 full pages + 512 bytes): condition is
        // true, num_pages = 4 (page 3 is partial).
        if (pager->file_length % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);
            if (bytes_read == -1) {
                fprintf(stderr, "Error reading file: %d\n", errno);
                return NULL;
            }
        }

        pager->pages[page_num] = page;
    }

    return pager->pages[page_num];
}
