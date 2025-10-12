#pragma once

#include "downstream.h"
#include "upstream.h"

typedef struct Pool {
    DownstreamBuffer *ds_buf;
    Upstream *us;
} Pool;

Pool *pool_init(Upstream *up);
void pool_free(Pool *p);
int pool_add_ds(Pool *p, Downstream *ds);


typedef struct PoolArr {
    Pool **arr;
    size_t size;
    size_t cap;
} PoolArr;

PoolArr *parr_init(size_t n);
int parr_append(PoolArr *parr, Pool *pool);
int parr_remove_us(PoolArr *parr, Upstream *us);
Pool *pget_ds(PoolArr *parr, Downstream *ds);
Pool *pget_us(PoolArr *parr, Upstream *us);
