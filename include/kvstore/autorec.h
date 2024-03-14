#ifndef _KVS_AUTOREC_H
#define _KVS_AUTOREC_H

#include <kvstore/store.h>
#include <stdbool.h>

/******************************************************************************
 * Automatic primary keyed record store handling
 ******************************************************************************/

#define KVS_AUTOREC_INVAL_ID \
	((uint64_t)0)

#define KVS_AUTOREC_MAX_ID \
	((uint64_t)1 << ((sizeof_member(DB_HEAP_RID, pgno) + \
	                  sizeof_member(DB_HEAP_RID, indx)) * CHAR_BIT))

static inline bool
kvs_autorec_id_isok(uint64_t id)
{
	return (id > 0) && (id < KVS_AUTOREC_MAX_ID);
}

extern int
kvs_autorec_iter_first(const struct kvs_iter *iter,
                       uint64_t              *id,
                       struct kvs_chunk      *item);

extern int
kvs_autorec_iter_next(const struct kvs_iter *iter,
                      uint64_t              *id,
                      struct kvs_chunk      *item);

extern int
kvs_autorec_init_iter(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      struct kvs_iter        *iter);

extern int
kvs_autorec_fini_iter(const struct kvs_iter *iter);

extern int
kvs_autorec_get_byid(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     uint64_t                id,
                     struct kvs_chunk       *item);

extern int
kvs_autorec_get_byfield(const struct kvs_store *index,
                        const struct kvs_xact  *xact,
                        const struct kvs_chunk *field,
                        uint64_t               *id,
                        struct kvs_chunk       *item);

extern int
kvs_autorec_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                uint64_t               *id,
                const struct kvs_chunk *item);

extern int
kvs_autorec_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   uint64_t                id,
                   const struct kvs_chunk *item);

extern int
kvs_autorec_del_byid(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     uint64_t                id);

extern int
kvs_autorec_del_byfield(const struct kvs_store *index,
                        const struct kvs_xact  *xact,
                        const struct kvs_chunk *field);

extern int
kvs_autorec_open(struct kvs_store       *store,
                 const struct kvs_depot *depot,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode);

extern int
kvs_autorec_close(const struct kvs_store *store);

#endif /* _KVS_AUTOREC_H */
