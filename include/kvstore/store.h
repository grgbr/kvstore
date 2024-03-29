#ifndef _KVS_STORE_H
#define _KVS_STORE_H

#include <kvstore/config.h>
#include <db.h>
#include <stdio.h>

/******************************************************************************
 * Global kvstore library routines.
 ******************************************************************************/

#if defined(CONFIG_KVSTORE_ASSERT)

#include <utils/assert.h>

#define kvs_assert(_expr) \
	uassert("kvstore", _expr)

#else  /* !defined(CONFIG_KVS_ASSERT) */

#define kvs_assert(_expr)

#endif /* defined(CONFIG_KVS_ASSERT) */

#define KVS_VERB_OUT         (NULL)
#define KVS_VERB_QUIET       (NULL)
#define KVS_VERB_ERR_PREFIX  "kvstore error: "
#define KVS_VERB_INFO_PREFIX "kvstore info: "

extern void
kvs_enable_verb(FILE       *out_file,
                const char *err_prefix,
                const char *info_prefix);

extern void
kvs_disable_verb(void);

extern const char *
kvs_strerror(int err);

/******************************************************************************
 * Depot handling.
 ******************************************************************************/

struct kvs_depot {
	DB_ENV       *env;
	unsigned int  flags;
};

#define KVS_DEPOT_PRIV   (DB_PRIVATE)
#define KVS_DEPOT_THREAD (DB_THREAD)
#define KVS_DEPOT_MVCC   (DB_MULTIVERSION)

extern int
kvs_open_depot(struct kvs_depot *depot,
               const char       *path,
               size_t            max_log_size,
               unsigned int      flags,
               mode_t            mode);

extern int
kvs_close_depot(const struct kvs_depot *depot);

/******************************************************************************
 * Transaction handling.
 ******************************************************************************/

struct kvs_xact {
	DB_TXN *txn;
};

#define KVS_XACT_NOWAIT (DB_TXN_NOWAIT)

extern int
kvs_begin_xact(const struct kvs_depot *depot,
               const struct kvs_xact  *parent,
               struct kvs_xact        *xact,
               unsigned int            flags);

extern int
kvs_rollback_xact(const struct kvs_xact *xact);

extern int
kvs_commit_xact(const struct kvs_xact *xact);

extern int
kvs_end_xact(const struct kvs_xact *xact, int status);

extern int
kvs_abort_xact(const struct kvs_xact *xact, int status);

/******************************************************************************
 * Data store / index handling.
 ******************************************************************************/

struct kvs_chunk {
	size_t       size;
	const void * data;
	const void * priv;
};

struct kvs_iter {
	DBC *curs;
};

struct kvs_store {
	DB *db;
};

typedef int (kvs_bind_indx_fn)(const struct kvs_chunk *pkey,
                               const struct kvs_chunk *item,
                               struct kvs_chunk       *skey);

extern int
kvs_open_indx(struct kvs_store       *indx,
              const struct kvs_store *store,
              const struct kvs_depot *depot,
              const struct kvs_xact  *xact,
              const char             *path,
              const char             *name,
              mode_t                  mode,
              kvs_bind_indx_fn       *bind);

extern int
kvs_close_indx(const struct kvs_store *store);

#endif /* _KVS_STORE_H */
