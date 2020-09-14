#ifndef _KVS_COMMON_H
#define _KVS_COMMON_H

#include "kvstore/config.h"
#include <kvstore/store.h>
#include <utils/cdefs.h>

#if defined(CONFIG_KVSTORE_DEBUG)

extern void
kvs_env_dbg(DB_ENV     *env,
            const char *fmt,
            ...) __nonull(1) __printf(2, 3);

#else  /* !defined(CONFIG_KVSTORE_DEBUG) */

static inline void __nonull(1) __printf(2, 3)
kvs_env_dbg(DB_ENV *env __unused, const char *fmt __unused, ...)
{
}

#endif /* defined(CONFIG_KVSTORE_DEBUG) */

#define kvs_assert_depot(_depot) \
	kvs_assert(_depot); \
	kvs_assert((_depot)->env); \
	kvs_assert(!((_depot)->flags & ~(KVS_DEPOT_THREAD | KVS_DEPOT_MVCC)));

#define kvs_assert_xact(_xact) \
	kvs_assert(_xact); \
	kvs_assert((_xact)->txn)

#define KVS_STR_MAX (4096U)

extern int
kvs_err_from_bdb(int err);

extern int
kvs_dup_str(char **string, const void *data, size_t len);

#if defined(CONFIG_KVSTORE_TYPE_STRPILE)

struct upile;

extern int
kvs_deserialize_strpile(struct upile *pile, const void *data, size_t size);

extern ssize_t
kvs_serialize_strpile(const struct upile *pile, void **data);

#endif /* defined(CONFIG_KVSTORE_TYPE_STRPILE) */

#define KVS_CHUNK_INIT_DBT(_chunk) \
	{ \
		.data = (void *)(_chunk)->data, \
		.size = (_chunk)->size, \
		0, \
	}

extern int
kvs_iter_goto_first(const struct kvs_iter *iter, DBT *key, DBT *item);

extern int
kvs_iter_goto_next(const struct kvs_iter *iter, DBT *key, DBT *item);

extern int
kvs_iter_goto_last(const struct kvs_iter *iter, DBT *key, DBT *item);

extern int
kvs_iter_goto_prev(const struct kvs_iter *iter, DBT *key, DBT *item);

extern int
kvs_init_iter(const struct kvs_store *store,
              const struct kvs_xact  *xact,
              struct kvs_iter        *iter);

extern int
kvs_fini_iter(const struct kvs_iter *iter);

extern int
kvs_get(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key,
        DBT                    *item,
        unsigned int            flags);

extern int
kvs_pget(const struct kvs_store *index,
         const struct kvs_xact  *xact,
         DBT                    *ikey,
         DBT                    *pkey,
         DBT                    *item,
         unsigned int            flags);

extern int
kvs_put(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key,
        DBT                    *item,
        unsigned int            flags);

extern int
kvs_del(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key);

extern int
kvs_open_store(struct kvs_store       *store,
               const struct kvs_depot *depot,
               const struct kvs_xact  *xact,
               const char             *path,
               const char             *name,
               DBTYPE                  type,
               mode_t                  mode);

extern int
kvs_close_store(const struct kvs_store *store);

#if defined(CONFIG_KVSTORE_LOG)

#if defined(CONFIG_KVSTORE_DEBUG)

extern const char *
kvs_recop_str(db_recops op);

#define KVS_LOG_REC_BASE_FMT \
	"%s log record:\n" \
	"    op       %s\n" \
	"    lsn      [%u][%u]\n" \
	"    type     %u\n" \
	"    prev lsn [%u][%u]\n" \
	"    txnid    %x\n"

#define kvs_log_rec_dbg(_env, _lsn, _op, _rec, _name, _fmt, ...) \
	({ \
		DB_ENV                   *__env = _env; \
		DB_LSN                   *__lsn = _lsn; \
		db_recops                 __op = _op; \
		const struct kvs_log_rec *__rec = _rec; \
		const char               *__name = _name; \
		\
		kvs_env_dbg(__env, \
		            KVS_LOG_REC_BASE_FMT _fmt, \
		            __name, \
		            kvs_recop_str(__op), \
		            __lsn->file, __lsn->offset, \
		            __rec->type, \
		            __rec->prev.file, __rec->prev.offset, \
		            __rec->txn->txnid, \
		            ## __VA_ARGS__); \
	 })

#else  /* !defined(CONFIG_KVSTORE_DEBUG) */

#define kvs_log_rec_dbg(_env, _lsn, _op, _rec, _name, _fmt, ...)

#endif /* defined(CONFIG_KVSTORE_DEBUG) */

struct kvs_log_rec {
	uint32_t  type;
	DB_TXN   *txn;
	DB_LSN    prev;
};

#define KVS_USER_LOG_REC \
	(DB_user_BEGIN)

#define KVS_LOG_REC_SIZE \
	(sizeof_member(struct kvs_log_rec, type) + \
	 sizeof_member(DB_TXN, txnid) + \
	 sizeof_member(struct kvs_log_rec, prev))

#define kvs_get_log_rec(_env, _dbt, _size, _spec, _rec) \
	({ \
		DB_ENV            *__env = _env; \
		const DBT         *__dbt = _dbt; \
		uint32_t           __size = _size; \
		DB_LOG_RECSPEC    *__spec = _spec; \
		int                __ret; \
		\
		*(_rec) = NULL; \
		if (__dbt->size < (KVS_LOG_REC_SIZE + __size)) \
			__ret = EMSGSIZE; \
		else \
			__ret = __env->log_read_record(__env, \
		                                       NULL, \
		                                       NULL, \
		                                       __dbt->data, \
		                                       __spec, \
		                                       sizeof(**(_rec)), \
		                                       (void **)(_rec)); \
		__ret; \
	 })

#define kvs_put_log_rec(_env, _txn, _type, _size, _spec, ...) \
	({ \
		DB_ENV         *__env = _env; \
		DB_TXN         *__txn = _txn; \
		uint32_t        __type = _type; \
		uint32_t        __size = _size; \
		DB_LOG_RECSPEC *__spec = _spec; \
		DB_LSN          __lsn = { 0, }; \
		int             __ret; \
		\
		__ret = __env->log_put_record(__env, \
		                              NULL, \
		                              __txn, \
		                              &__lsn, \
		                              DB_FLUSH, \
		                              KVS_USER_LOG_REC + __type, \
		                              0, \
		                              KVS_LOG_REC_SIZE + __size, \
		                              __spec, \
		                              ## __VA_ARGS__); \
		(!__ret) ? 0 : kvs_err_from_bdb(__ret); \
	 })

extern int
kvs_print_log_rec(DB_ENV         *env,
                  DBT            *rec,
                  DB_LSN         *lsn,
                  char           *name,
                  DB_LOG_RECSPEC *spec);

extern void
kvs_init_log(const struct kvs_depot *depot);

#else  /* !defined(CONFIG_KVSTORE_LOG) */

static inline void
kvs_init_log(const struct kvs_depot *depot __unused)
{
}

#endif /* defined(CONFIG_KVSTORE_LOG) */

#if defined(CONFIG_KVSTORE_FILE)

#define KVS_FILE_LOG_REC (0)

extern int
kvs_file_handle_log_rec(DB_ENV    *env,
                        DBT       *rec,
                        DB_LSN    *lsn,
                        db_recops  op);

#endif /* defined(CONFIG_KVSTORE_FILE) */

#endif /* _KVS_COMMON_H */
