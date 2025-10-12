#include "pool.h"
#include "downstream.h"
#include "include/common.h"
#include "upstream.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

Pool *pool_init(Upstream *up) {
    if (!up)
        return NULL;
    Pool *pool = malloc(sizeof(Pool));
    if (!pool)
        return NULL;
    pool->ds_buf = ds_buf_init();
    pool->us = up;
    return pool;
}

void pool_free(Pool *p) {
    ds_buf_free(p->ds_buf);
}

int pool_add_ds(Pool *p, Downstream *ds);


int parr_realloc(PoolArr *parr, size_t n) {
    parr->arr = realloc(parr->arr, n);
    if (!parr->arr) {
        return -1;
    }
    parr->cap = n;
    return 0;
}

PoolArr *parr_init(size_t n) {
    PoolArr *parr = malloc(sizeof(PoolArr));
    if (!parr) {
        die("p_arr_init() parr malloc");
        return NULL;
    }
    parr->arr = malloc(n * sizeof(Pool));
    if (!parr->arr) {
        die("p_arr_init() arr malloc");
        return NULL;
    }
    parr->cap = n;
    parr->size = 0;
    return parr;
}

int parr_append(PoolArr *parr, Pool *pool) {
    if (parr->size == parr->cap) {
        if (!parr_realloc(parr, parr->cap * 2))
            return -1;
    }
    parr->arr[parr->size++] = pool;
    return 1;
}

int parr_remove_us(PoolArr *parr, Upstream *us) {
    for (size_t i = 0; i < parr->size; ++i) {
        if (parr->arr[i]->us == us) {
            while (i + 1 < parr->size) {
                Pool *tmp = parr->arr[i];
                parr->arr[i + 1] = parr->arr[i];
                parr->arr[i] = tmp;
                i++;
            }
            parr->size--;
            return 1;
        }
    }
    return -1;
}

Pool *pget_ds(PoolArr *parr, Downstream *ds) {
    for (size_t i = 0; i < parr->size; ++i) {
        if (ds_buf_exists(parr->arr[i]->ds_buf, ds) > 0) {
            return parr->arr[i];
        }
    }
    return NULL;
}

Pool *pget_us(PoolArr *parr, Upstream *us) {
    for (size_t i = 0; i < parr->size; ++i) {
        if (parr->arr[i]->us == us) {
            return parr->arr[i];
        }
    }
    return NULL;
}
