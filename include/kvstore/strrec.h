#ifndef _KVS_STRREC_H
#define _KVS_STRREC_H

#include <kvstore/store.h>
#include <stdbool.h>

/******************************************************************************
 * String primary keyed record store handling
 ******************************************************************************/

extern int
kvs_strrec_iter_first(const struct kvs_iter *iter,
                      struct kvs_chunk      *id,
                      struct kvs_chunk      *item);

extern int
kvs_strrec_iter_next(const struct kvs_iter *iter,
                     struct kvs_chunk      *id,
                     struct kvs_chunk      *item);

extern int
kvs_strrec_init_iter(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     struct kvs_iter        *iter);

extern int
kvs_strrec_fini_iter(const struct kvs_iter *iter);

extern int
kvs_strrec_get_byid(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    const struct kvs_chunk *id,
                    struct kvs_chunk       *item);

extern int
kvs_strrec_get_byfield(const struct kvs_store *index,
                       const struct kvs_xact  *xact,
                       const struct kvs_chunk *field,
                       struct kvs_chunk       *id,
                       struct kvs_chunk       *item);

extern int
kvs_strrec_add(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const struct kvs_chunk *id,
               const struct kvs_chunk *item);

extern int
kvs_strrec_put(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const struct kvs_chunk *id,
               const struct kvs_chunk *item);

extern int
kvs_strrec_del_byid(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    const struct kvs_chunk *id);

extern int
kvs_strrec_del_byfield(const struct kvs_store *index,
                       const struct kvs_xact  *xact,
                       const struct kvs_chunk *field);

extern int
kvs_strrec_open(struct kvs_store       *store,
                const struct kvs_depot *depot,
                const struct kvs_xact  *xact,
                const char             *path,
                const char             *name,
                mode_t                  mode);

extern int
kvs_strrec_close(const struct kvs_store *store);

#endif /* _KVS_STRREC_H */
