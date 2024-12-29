#pragma once

#include <stddef.h>

#include "lib.h"

typedef struct {
    void *pages[TABLE_MAX_PAGES];
    size_t fd;
    size_t file_length;
} Pager;

Pager *pager_init(void);
void *get_page(Pager *pager, size_t page_num);
