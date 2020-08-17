#include "common.h"
#include <errno.h>


#if defined(CONFIG_KVSTORE_DEBUG)

const char *
kvs_recop_str(db_recops op)
{
	switch (op) {
	case DB_TXN_ABORT:
		return "abort";
	case DB_TXN_APPLY:
		return "apply";
	case DB_TXN_BACKWARD_ROLL:
		return "backward roll";
	case DB_TXN_FORWARD_ROLL:
		return "forward roll";
	default:
		return "unknown";
	}
}

#endif /* defined(CONFIG_KVSTORE_DEBUG) */

typedef int (kvs_handle_log_rec_fn)(DB_ENV    *env,
                                    DBT       *log_dbt,
                                    DB_LSN    *lsn,
                                    db_recops  op);

static kvs_handle_log_rec_fn * const kvs_log_rec_dispatchers[] = {
	[KVS_FILE_LOG_REC] = kvs_file_handle_log_rec
};

static int
kvs_handle_log_rec(DB_ENV *env, DBT *rec, DB_LSN *lsn, db_recops op)
{
	unsigned int type;

	if (rec->size < KVS_LOG_REC_SIZE)
		return EBADMSG;
	if (!rec->data)
		return EFAULT;

	type = ((struct kvs_log_rec *)rec->data)->type - KVS_USER_LOG_REC;
	if (type >= array_nr(kvs_log_rec_dispatchers))
		return ENOTSUP;

	return kvs_log_rec_dispatchers[type](env, rec, lsn, op);
}

int
kvs_print_log_rec(DB_ENV         *env,
                  DBT            *rec,
                  DB_LSN         *lsn,
                  char           *name,
                  DB_LOG_RECSPEC *spec)
{
	extern int __log_print_record(ENV *,
	                              DBT *,
	                              DB_LSN *,
	                              char *,
	                              DB_LOG_RECSPEC *,
	                              void *);

	return __log_print_record(env->env, rec, lsn, name, spec, NULL);
}

void
kvs_init_log(const struct kvs_depot *depot)
{
	int err;

	err = depot->env->set_app_dispatch(depot->env, kvs_handle_log_rec);
	kvs_assert(!err);
}
