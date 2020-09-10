#ifndef _KVS_AUTOREC_H
#define _KVS_AUTOREC_H

#include <kvstore/store.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

/******************************************************************************
 * Automatic index store / iterator handling
 ******************************************************************************/

struct kvs_autorec_id {
	DB_HEAP_RID rid;
};

extern const struct kvs_autorec_id kvs_autorec_none;

#define KVS_AUTOREC_NONE \
	kvs_autorec_none

extern bool
kvs_autorec_id_isok(struct kvs_autorec_id id);

struct kvs_autorec_desc {
	struct kvs_autorec_id  id;
	const void            *data;
	size_t                 size;
};

extern int
kvs_autorec_iter_first(const struct kvs_iter  *iter,
                       struct kvs_autorec_desc *desc);

extern int
kvs_autorec_iter_next(const struct kvs_iter   *iter,
                      struct kvs_autorec_desc *desc);

extern int
kvs_autorec_init_iter(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      struct kvs_iter        *iter);

extern int
kvs_autorec_fini_iter(const struct kvs_iter *iter);

extern ssize_t
kvs_autorec_get(const struct kvs_store  *store,
                const struct kvs_xact   *xact,
                struct kvs_autorec_id    id,
	        const void             **data);

extern int
kvs_autorec_get_desc(const struct kvs_store  *store,
                     const struct kvs_xact   *xact,
                     struct kvs_autorec_id    id,
                     struct kvs_autorec_desc *desc);

extern int
kvs_autorec_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_autorec_id   id,
                   const void             *data,
                   size_t                  size);

extern int
kvs_autorec_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autorec_id  *id,
	        const void             *data,
	        size_t                  size);

extern int
kvs_autorec_del(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autorec_id   id);

extern int
kvs_autorec_open(struct kvs_store       *store,
                 const struct kvs_depot *depot,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode);

extern int
kvs_autorec_close(const struct kvs_store *store);

#endif /* _KVS_AUTOREC_H */
