#pragma once

#include <stddef.h>

#include "lib.h"

typedef struct {
    void *pages[TABLE_MAX_PAGES];
    size_t fd;
    size_t length;
} Pager;

Pager *pager_init(void);
