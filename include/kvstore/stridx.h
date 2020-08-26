#ifndef _KVS_STRIDX_H
#define _KVS_STRIDX_H

#include <kvstore/store.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

/******************************************************************************
 * String index store / iterator handling
 ******************************************************************************/

struct kvs_stridx_desc {
	const char *id;
	size_t      len;
	const void *data;
	size_t      size;
};

extern int
kvs_stridx_iter_first(const struct kvs_iter  *iter,
                      struct kvs_stridx_desc *desc);

extern int
kvs_stridx_iter_next(const struct kvs_iter *iter, struct kvs_stridx_desc *desc);

extern int
kvs_stridx_init_iter(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     struct kvs_iter        *iter);

extern int
kvs_stridx_fini_iter(const struct kvs_iter *iter);

extern ssize_t
kvs_stridx_get(const struct kvs_store *store,
               const struct kvs_xact  *xact,
	       const char             *id,
	       size_t                  len,
	       const void            **data);

extern int
kvs_stridx_get_desc(const struct kvs_store  *store,
                    const struct kvs_xact   *xact,
                    const char              *id,
                    size_t                   len,
                    struct kvs_stridx_desc  *desc);

extern int
kvs_stridx_put(const struct kvs_store *store,
               const struct kvs_xact  *xact,
	       const char             *id,
	       size_t                  len,
	       const void             *data,
	       size_t                  size);

extern int
kvs_stridx_add(const struct kvs_store *store,
               const struct kvs_xact  *xact,
	       const char             *id,
	       size_t                  len,
	       const void             *data,
	       size_t                  size);

extern int
kvs_stridx_del(const struct kvs_store *store,
               const struct kvs_xact  *xact,
	       const char             *id,
	       size_t                  len);

extern int
kvs_stridx_open(struct kvs_store       *store,
                const struct kvs_depot *depot,
                const struct kvs_xact  *xact,
                const char             *path,
                const char             *name,
                mode_t                  mode);

extern int
kvs_stridx_close(const struct kvs_store *store);

#endif /* _KVS_STRIDX_H */
