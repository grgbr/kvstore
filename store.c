#include "common.h"
#include <kvstore/store.h>
#include <utils/string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#if defined(CONFIG_KVSTORE_DEBUG)

#include <stdarg.h>
#include <stdlib.h>

void __nonull(1) __printf(2, 3)
kvs_env_dbg(DB_ENV *env, const char *fmt, ...)
{
	void (*msg_call)(const DB_ENV *, const char *) = NULL;

	env->get_msgcall(env, &msg_call);
	if (msg_call) {
		va_list  args;
		char    *msg;
		int      ret;

		va_start(args, fmt);
		ret = vasprintf(&msg, fmt, args);
		va_end(args);

		if (ret >= 0) {
			msg_call(env, msg);
			free(msg);
		}
	}
}

#endif /* defined(CONFIG_KVSTORE_DEBUG) */

int
kvs_err_from_bdb(int err)
{
	switch (err) {
	case DB_FOREIGN_CONFLICT:
		return -EADDRINUSE;
	case DB_NOTFOUND:
		return -ENOENT;
	case DB_KEYEXIST:
		return -EEXIST;
	case DB_SECONDARY_BAD:
		return -EXDEV;
	case DB_KEYEMPTY:
		return -ENOKEY;
	case DB_LOCK_DEADLOCK:
		return -EDEADLOCK;
	case DB_LOCK_NOTGRANTED:
		return -EBUSY;
	case DB_RUNRECOVERY:
		return -ENOTRECOVERABLE;
	default:
		return -err;
	}
}

const char *
kvs_strerror(int err)
{
	switch (err) {
	case -EADDRINUSE:
		return "Foreign key conflict";
	case -ENOENT:
		return "Key/data pair not found";
	case -EEXIST:
		return "Key/data pair already exists";
	case -EXDEV:
		return "Bad secondary index";
	case -ENOKEY:
		return "Key empty";
	case -EDEADLOCK:
		return "Deadlock detected";
	case DB_LOCK_NOTGRANTED:
		return "Lock not granted";
	case DB_RUNRECOVERY:
		return "Unrecoverable error";
	default:
		return strerror(-err);
	}
}

int
kvs_dup_str(char **string, const void *data, size_t len)
{
	kvs_assert(string);
	kvs_assert(data);

	char *str;

	if (len >= KVS_STR_MAX)
		return -ENAMETOOLONG;

	str = ustr_clone(data, len);
	if (!str)
		return -errno;

	*string = str;

	return 0;
}

#if defined(CONFIG_KVSTORE_TYPE_STRPILE)

#include <utils/pile.h>

struct kvs_strlot_elm {
	uint16_t len;
	char     bytes[0];
} __packed;

struct kvs_strlot {
	size_t                 size;
	char                  *curr;
	struct kvs_strlot_elm *elms;
};

#define kvs_strlot_assert(_lot) \
	kvs_assert(_lot); \
	kvs_assert((_lot)->size); \
	kvs_assert((_lot)->curr); \
	kvs_assert((_lot)->elms)

#define KVS_STRLOT_SIZE_MIN \
	(sizeof_member(struct kvs_strlot_elm, len) + 1)

static bool
kvs_strlot_has_ended(const struct kvs_strlot *lot)
{
	kvs_strlot_assert(lot);

	return (char *)lot->curr >= &((char *)lot->elms)[lot->size];
}

static char *
kvs_strlot_data(const struct kvs_strlot *lot)
{
	kvs_assert(!kvs_strlot_has_ended(lot));

	return (char *)lot->elms;
}

static const struct kvs_strlot_elm *
kvs_strlot_iter_next(struct kvs_strlot *lot)
{
	kvs_assert(!kvs_strlot_has_ended(lot));

	struct kvs_strlot_elm *elm = (struct kvs_strlot_elm *)lot->curr;

	lot->curr = &elm->bytes[elm->len];

	return (struct kvs_strlot_elm *)lot->curr;
}

static const struct kvs_strlot_elm *
kvs_strlot_begin_iter(struct kvs_strlot *lot, const char *data, size_t size)
{
	kvs_assert(lot);
	kvs_assert(data);
	kvs_assert(size);

	lot->size = size;
	lot->curr = (char *)data;
	lot->elms = (struct kvs_strlot_elm *)data;

	return lot->elms;
}

#define kvs_strlot_foreach_elm(_lot, _data, _size, _elm) \
	for ((_elm) = kvs_strlot_begin_iter(_lot, _data, _size); \
	     !kvs_strlot_has_ended(_lot); \
	     (_elm) = kvs_strlot_iter_next(_lot))

static size_t
kvs_strlot_pushed_size(const struct kvs_strlot *lot)
{
	kvs_strlot_assert(lot);

	return (size_t)(lot->curr - (char *)lot->elms);
}

static void
kvs_strlot_push(struct kvs_strlot *lot, const char *str, size_t len)
{
	kvs_strlot_assert(lot);

	struct kvs_strlot_elm *elm = (struct kvs_strlot_elm *)
	                             ualign_upper((unsigned long)lot->curr,
	                                          sizeof(elm->len));

	kvs_assert(&elm->bytes[len] <= &((char *)lot->elms)[lot->size]);

	elm->len = (uint16_t)len;
	memcpy(elm->bytes, str, len);

	lot->curr = &elm->bytes[len];
}

static int
kvs_strlot_init(struct kvs_strlot *lot, size_t size, unsigned int nr)
{
	kvs_assert(lot);
	kvs_assert(size >= nr);
	kvs_assert(nr);

	size_t sz = size - nr + (nr * (sizeof(*lot->elms) + 1));

	lot->elms = malloc(size - nr +
	                   (nr * (sizeof(lot->elms[0]) +
	                          ualign_mask(sizeof(lot->elms[0])))));
	if (!lot->elms)
		return -ENOMEM;

	lot->size = sz;
	lot->curr = (char *)lot->elms;

	return 0;
}

int
kvs_deserialize_strpile(struct upile *pile, const void *data, size_t size)
{
	kvs_assert(upile_is_empty(pile));

	struct kvs_strlot            lot;
	const struct kvs_strlot_elm *elm;
	int                          ret = -ENODATA;

	if (!data)
		return -ENODATA;

	if (size < KVS_STRLOT_SIZE_MIN)
		return -EBADMSG;

	kvs_strlot_foreach_elm(&lot, data, size, elm) {
		if (!elm->len) {
			ret = -EBADMSG;
			break;
		}
		if (elm->len >= KVS_STR_MAX) {
			ret = -ENAMETOOLONG;
			break;
		}
		if (!upile_clone_str(pile, elm->bytes, elm->len)) {
			ret = -errno;
			break;
		}
	}

	if (ret)
		goto clear;

	if (upile_is_empty(pile)) {
		ret = -ENODATA;
		goto clear;
	}

	return 0;

clear:
	upile_clear(pile);

	return ret;
}

ssize_t
kvs_serialize_strpile(const struct upile *pile, void **data)
{
	kvs_assert(!upile_is_empty(pile));
	kvs_assert(data);

	struct kvs_strlot  lot;
	int                ret;
	const char        *str;

	ret = kvs_strlot_init(&lot, upile_size(pile), upile_nr(pile));
	if (ret)
		return ret;

	upile_foreach_str(pile, str)
		kvs_strlot_push(&lot, str, upile_str_len(str));

	/*
	 * Give ownership to caller who will have to free the underlying
	 * memory.
	 */
	*data = kvs_strlot_data(&lot);

	return kvs_strlot_pushed_size(&lot);
}

#endif /* defined(CONFIG_KVSTORE_TYPE_STRPILE) */

int
kvs_begin_xact(const struct kvs_depot *depot,
               const struct kvs_xact  *parent,
               struct kvs_xact        *xact,
               unsigned int            flags)
{
	kvs_assert_depot(depot);
	kvs_assert(!parent || parent->txn);
	kvs_assert(xact);
	kvs_assert(!(flags & ~KVS_XACT_NOWAIT));

	int ret;

	ret = depot->env->txn_begin(depot->env,
	                            parent ? parent->txn : NULL,
	                            &xact->txn,
	                            flags);

	return kvs_err_from_bdb(ret);
}

int
kvs_rollback_xact(const struct kvs_xact *xact)
{
	kvs_assert_xact(xact);

	int ret;

	ret = xact->txn->abort(xact->txn);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_commit_xact(const struct kvs_xact *xact)
{
	kvs_assert_xact(xact);

	int ret;

	ret = xact->txn->commit(xact->txn, 0);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

#define kvs_assert_iter(_iter) \
	kvs_assert(_iter); \
	kvs_assert(&(_iter)->curs)

static int
kvs_iter_goto(const struct kvs_iter *iter,
              DBT                   *key,
              DBT                   *item,
              unsigned int           flags)
{
	kvs_assert_iter(iter);
	kvs_assert(key || item);
	kvs_assert(flags & (DB_FIRST | DB_NEXT | DB_LAST | DB_PREV));
	kvs_assert(!(flags & ~(DB_FIRST | DB_NEXT | DB_LAST | DB_PREV)));

	int ret;

	ret = iter->curs->c_get(iter->curs, key, item, flags);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_iter_goto_first(const struct kvs_iter *iter, DBT *key, DBT *item)
{
	return kvs_iter_goto(iter, key, item, DB_FIRST);
}

int
kvs_iter_goto_next(const struct kvs_iter *iter, DBT *key, DBT *item)
{
	return kvs_iter_goto(iter, key, item, DB_NEXT);
}

int
kvs_iter_goto_last(const struct kvs_iter *iter, DBT *key, DBT *item)
{
	return kvs_iter_goto(iter, key, item, DB_LAST);
}

int
kvs_iter_goto_prev(const struct kvs_iter *iter, DBT *key, DBT *item)
{
	return kvs_iter_goto(iter, key, item, DB_PREV);
}

int
kvs_init_iter(const struct kvs_store *store,
              const struct kvs_xact  *xact,
              struct kvs_iter        *iter)
{
	kvs_assert(store);
	kvs_assert(store->db);
	kvs_assert_xact(xact);
	kvs_assert(iter);

	int ret;

	ret = store->db->cursor(store->db, xact->txn, &iter->curs, 0);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_fini_iter(const struct kvs_iter *iter)
{
	kvs_assert_iter(iter);

	int ret;

	ret = iter->curs->c_close(iter->curs);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_get(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key,
        DBT                    *item,
        unsigned int            flags)
{
	kvs_assert(store);
	kvs_assert(store->db);
	kvs_assert_xact(xact);
	kvs_assert(key);
	kvs_assert(key->size);
	kvs_assert(key->data);
	kvs_assert(item);

	int ret;

	ret = store->db->get(store->db, xact->txn, key, item, flags);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_pget(const struct kvs_store *index,
         const struct kvs_xact  *xact,
         DBT                    *ikey,
         DBT                    *pkey,
         DBT                    *item,
         unsigned int            flags)
{
	kvs_assert(index);
	kvs_assert(index->db);
	kvs_assert_xact(xact);
	kvs_assert(ikey);
	kvs_assert(ikey->data);
	kvs_assert(ikey->size);
	kvs_assert(pkey);
	kvs_assert(item);

	int ret;

	ret = index->db->pget(index->db, xact->txn, ikey, pkey, item, flags);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

int
kvs_put(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key,
        DBT                    *item,
        unsigned int            flags)
{
	kvs_assert(store);
	kvs_assert(store->db);
	kvs_assert_xact(xact);
	kvs_assert(key);
#if defined(CONFIG_KVSTORE_ASSERT)
	if (flags & DB_APPEND) {
		kvs_assert(!key->data);
		kvs_assert(!key->size);
	}
	else {
		kvs_assert(key->data);
		kvs_assert(key->size);
	}
#endif /* defined(CONFIG_KVSTORE_ASSERT) */
	kvs_assert(item);
	kvs_assert(item->data || !item->size);

	int ret;

	ret = store->db->put(store->db, xact->txn, key, item, flags);

	/*
	 * Note: BDB will return EINVAL in case of violation of unique secondary
	 * index integrity contraint.
	 */
	if (ret == EINVAL)
		return -EEXIST;

	return kvs_err_from_bdb(ret);
}

int
kvs_del(const struct kvs_store *store,
        const struct kvs_xact  *xact,
        DBT                    *key)
{
	kvs_assert(store);
	kvs_assert(store->db);
	kvs_assert_xact(xact);
	kvs_assert(key);
	kvs_assert(key->size);
	kvs_assert(key->data);

	int ret;

	ret = store->db->del(store->db, xact->txn, key, 0);
	kvs_assert(ret != EINVAL);

	return kvs_err_from_bdb(ret);
}

/*
 * Warning !
 * Even if open failed, close method SHALL be called ! The reason why is that
 * the transaction given in argument MUST be closed before closing the store
 * itself.
 * See the Berkeley DB->close() documentation for more infos.
 */
int
kvs_open_store(struct kvs_store       *store,
               const struct kvs_depot *depot,
               const struct kvs_xact  *xact,
               const char             *path,
               const char             *name,
               DBTYPE                  type,
               mode_t                  mode)
{
	kvs_assert(store);
	kvs_assert_depot(depot);
	kvs_assert(path);
	kvs_assert(*path);
	kvs_assert(!name || *name);
	kvs_assert(!((type == DB_HEAP) && name));
	kvs_assert(!((type == DB_QUEUE) && name));

	int err;

	err = db_create(&store->db, depot->env, 0);
	kvs_assert(err != EINVAL);
	if (err) {
		store->db = NULL;
		return kvs_err_from_bdb(err);
	}

	err = store->db->open(store->db,
	                      xact ? xact->txn : NULL,
	                      path,
	                      name,
	                      type,
	                      (xact ? 0 : DB_AUTO_COMMIT) | DB_CREATE |
	                      depot->flags,
	                      mode);
	kvs_assert(err != EINVAL);
	kvs_assert(err != DB_REP_HANDLE_DEAD);
	kvs_assert(err != DB_REP_LOCKOUT);

	return kvs_err_from_bdb(err);
}

int
kvs_close_store(const struct kvs_store *store)
{
	kvs_assert(store);

	if (store->db) {
		int ret;

		/*
		 * Although not strictly required, it is recommended to close
		 * all db handles before closing the environment.
		 *
		 * Warning: all cursors should be closed, all transactions
		 * resolved before closing db handles !
		 *
		 * Use DB_NOSYNC to spare flash memory write / erase cycles. It
		 * should be set ONLY when LOGGING and TRANSACTIONS enabled so
		 * that the database is recoverable after system / application
		 * crashes.
		 */
		ret = store->db->close(store->db, DB_NOSYNC);
		kvs_assert(ret != EINVAL);

		return kvs_err_from_bdb(ret);
	}

	return 0;
}

static int
kvs_bind_index(DB *index, const DBT *pkey, const DBT *pitem, DBT *skey)
{
	kvs_assert(index);
	kvs_assert(index->app_private);
	kvs_assert(pkey);
	kvs_assert(pkey->data);
	kvs_assert(pkey->size);
	kvs_assert(pitem);
	kvs_assert(skey);

	kvs_bind_index_fn *bind = index->app_private;
	ssize_t            ret;

	if (!pitem->size)
		return -EMSGSIZE;

	if (!pitem->data)
		return -ENODATA;

	ret = bind(pkey->data,
	           pkey->size,
	           pitem->data,
	           pitem->size,
	           &skey->data);
	if (ret <= 0)
		return (!ret) ? DB_DONOTINDEX : ret;

	kvs_assert(skey->data);

	skey->size = (size_t)ret;

	return 0;
}

int
kvs_open_index(struct kvs_store       *index,
               const struct kvs_store *store,
               const struct kvs_depot *depot,
               const struct kvs_xact  *xact,
               const char             *path,
               const char             *name,
               mode_t                  mode,
               kvs_bind_index_fn      *bind)
{
	int err;

	err = kvs_open_store(index, depot, xact, path, name, DB_BTREE, mode);
	if (err)
		return kvs_err_from_bdb(err);

	index->db->app_private = bind;

	err = store->db->associate(store->db,
	                           xact->txn,
	                           index->db,
	                           kvs_bind_index,
	                           DB_CREATE);
	kvs_assert(err != EINVAL);
	kvs_assert(err != DB_REP_HANDLE_DEAD);
	kvs_assert(err != DB_REP_LOCKOUT);

	return kvs_err_from_bdb(err);
}

int
kvs_close_index(const struct kvs_store *store)
{
	return kvs_close_store(store);
}

int
kvs_close_depot(const struct kvs_depot *depot)
{
	kvs_assert_depot(depot);

	/*
	 * No need to use DB_FORCESYNCENV (over NFS) since memory regions are
	 * located in system memory.
	 * No need to use DB_FORCESYNC either since our database handles should
	 * be closed using the DB_NOSYNC flag to spare flash write / erase
	 * cycles (see bdb_close_store);
	 */
	
	return kvs_err_from_bdb(depot->env->close(depot->env, 0));
}

static struct {
	FILE       *out;
	const char *err_pfx;
	const char *info_pfx;
} kvs_verb = {
	.out      = KVS_VERB_OUT,
	.err_pfx  = KVS_VERB_ERR_PREFIX,
	.info_pfx = KVS_VERB_QUIET
};

static void
kvs_print_err(const DB_ENV *env __unused, const char *pfx, const char *msg)
{
	fprintf(kvs_verb.out, "%s%s\n", pfx, msg);
}

#if defined(CONFIG_KVSTORE_DEBUG)

static void
kvs_print_msg(const DB_ENV *env __unused, const char *msg)
{
	fprintf(kvs_verb.out, "%s%s\n", kvs_verb.info_pfx, msg);
}

#endif /* defined(CONFIG_KVSTORE_DEBUG) */

int
kvs_open_depot(struct kvs_depot *depot,
               const char       *path,
               size_t            max_log_size,
               unsigned int      flags,
               mode_t            mode)
{
	kvs_assert(depot);
	kvs_assert(path);
	kvs_assert(*path);
	kvs_assert(!(flags & ~(KVS_DEPOT_PRIV |
	                       KVS_DEPOT_THREAD |
	                       KVS_DEPOT_MVCC)));
	kvs_assert(mode);

	int err;

	if (mkdir(path, mode)) {
		if (errno != EEXIST)
			return -errno;
	}

	err = db_env_create(&depot->env, 0);
	if (err)
		return kvs_err_from_bdb(err);

	/* Setup verbosity. */
	if (!kvs_verb.out)
		kvs_verb.out = stderr;

	if (kvs_verb.err_pfx) {
		depot->env->set_errpfx(depot->env, kvs_verb.err_pfx);
		depot->env->set_errcall(depot->env, kvs_print_err);
	}
	else
		depot->env->set_errcall(depot->env, NULL);

#if defined(CONFIG_KVSTORE_DEBUG)
	if (kvs_verb.info_pfx) {
		depot->env->set_verbose(depot->env, DB_VERB_DEADLOCK, 1);
		depot->env->set_verbose(depot->env, DB_VERB_FILEOPS, 1);
		depot->env->set_verbose(depot->env, DB_VERB_FILEOPS_ALL, 1);
		depot->env->set_verbose(depot->env, DB_VERB_RECOVERY, 1);
		depot->env->set_verbose(depot->env, DB_VERB_REGISTER, 1);
		depot->env->set_verbose(depot->env, DB_VERB_WAITSFOR, 1);
		depot->env->set_msgcall(depot->env, kvs_print_msg);
	}
	else
		depot->env->set_msgcall(depot->env, NULL);
#endif /* defined(CONFIG_KVSTORE_DEBUG) */

	/* Restrict maximum logging file size. */
	depot->env->set_lg_max(depot->env, max_log_size);

	kvs_init_log(depot);

	depot->flags = flags & (KVS_DEPOT_THREAD | KVS_DEPOT_MVCC);

	if (!(flags & KVS_DEPOT_PRIV)) {
		/*
		 * Generate a unique system wide System V IPC key for using
		 * System V shared memory segment based memory regions to limit
		 * underlying filesystem I/O activity.
		 */
		key_t key;

		key = ftok(path, 'F');
		if (key < 0) {
			err = -errno;
			goto err;
		}

		err = depot->env->set_shm_key(depot->env, key);
		kvs_assert(!err);

		/*
		 * Since multiple processes might access the store, init the
		 * locking subsystem.
		 * In addition, tell libdb to allocate region memory from system
		 * shared memory instead of from heap memory or memory backed by
		 * the filesystem.
		 */
		flags |= DB_INIT_LOCK | DB_REGISTER | DB_SYSTEM_MEM;
	}

	if (flags & KVS_DEPOT_THREAD)
		/* Init the locking subsystem, if threading requested. */
		flags |= DB_INIT_LOCK;

	/* Open environment with transaction and automatic recovery support. */
	err = depot->env->open(depot->env,
	                       path,
	                       DB_INIT_MPOOL |
	                       DB_INIT_TXN |
	                       DB_RECOVER |
	                       DB_CREATE |
	                       flags,
	                       mode & ~(S_IXUSR | S_IXGRP | S_IXOTH));
	kvs_assert(err != DB_RUNRECOVERY);
	kvs_assert(err != EINVAL);
	if (err)
		/*
		 * When opening fails, environment must be closed to discard
		 * environment handle.
		 */
		goto err;

	return 0;

err:
	/*
	 * Free resources allocated by db_env_create() and discard environment
	 * handle.
	 */
	kvs_close_depot(depot);

	return kvs_err_from_bdb(err);
}

void
kvs_enable_verb(FILE       *out_file,
                const char *error_prefix,
                const char *info_prefix)
{
	kvs_verb.out = out_file;
	kvs_verb.err_pfx = error_prefix;
	kvs_verb.info_pfx = info_prefix;
}

void
kvs_disable_verb(void)
{
	kvs_verb.out = NULL;
	kvs_verb.err_pfx = NULL;
	kvs_verb.info_pfx = NULL;
}
