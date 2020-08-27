#ifndef _KVS_AUTOIDX_H
#define _KVS_AUTOIDX_H

#include <kvstore/store.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

/******************************************************************************
 * Automatic index store / iterator handling
 ******************************************************************************/

struct kvs_autoidx_id {
	DB_HEAP_RID rid;
};

struct kvs_autoidx_desc {
	struct kvs_autoidx_id  id;
	const void            *data;
	size_t                 size;
};

extern int
kvs_autoidx_iter_first(const struct kvs_iter  *iter,
                      struct kvs_autoidx_desc *desc);

extern int
kvs_autoidx_iter_next(const struct kvs_iter *iter, struct kvs_autoidx_desc *desc);

extern int
kvs_autoidx_init_iter(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     struct kvs_iter        *iter);

extern int
kvs_autoidx_fini_iter(const struct kvs_iter *iter);

extern ssize_t
kvs_autoidx_get(const struct kvs_store *store,
                const struct kvs_xact   *xact,
                struct kvs_autoidx_id    id,
	        const void             **data);

extern int
kvs_autoidx_get_desc(const struct kvs_store  *store,
                     const struct kvs_xact   *xact,
                     struct kvs_autoidx_id    id,
                     struct kvs_autoidx_desc *desc);

extern int
kvs_autoidx_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_autoidx_id   id,
                   const void             *data,
                   size_t                  size);

extern int
kvs_autoidx_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autoidx_id  *id,
	        const void             *data,
	        size_t                  size);

extern int
kvs_autoidx_del(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autoidx_id   id);

extern int
kvs_autoidx_open(struct kvs_store       *store,
                 const struct kvs_depot *depot,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode);

extern int
kvs_autoidx_close(const struct kvs_store *store);

#endif /* _KVS_AUTOIDX_H */
