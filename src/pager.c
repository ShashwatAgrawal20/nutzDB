#include "headers/pager.h"

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
    pager->length = 0;
    pager->fd = fd;
    pager->length = file_length;
    return pager;
}
