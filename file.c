#include <kvstore/file.h>
#include "common.h"
#include <stdarg.h>
#include <utils/file.h>
#include <utils/dir.h>
#include <stdlib.h>
#include <sys/user.h>

/* Log record. */
struct kvs_file_log_rec {
	struct kvs_log_rec rec;
	DBT                path;
	mode_t             mode;
	DBT                orig;
	DBT                new;
};

#define kvs_file_assert(_file) \
	kvs_assert(_file); \
	kvs_assert(!(!!(_file)->size ^ !!(_file)->data)); \
	kvs_assert((_file)->off <= (_file)->size); \
	kvs_assert((_file)->env); \
	kvs_assert((_file)->dir >= 0)

/* Log record (user) fields size. */
static size_t
kvs_file_log_rec_size(const DBT *path_dbt,
                      const DBT *orig_dbt,
                      const DBT *new_dbt)
{
	return LOG_DBT_SIZE(path_dbt) +
	       sizeof(uint32_t) +
	       LOG_DBT_SIZE(orig_dbt) +
	       LOG_DBT_SIZE(new_dbt);
}

static size_t
kvs_file_log_rec_min_size(void)
{
	const DBT dbt = { 0, };

	return kvs_file_log_rec_size(&dbt, &dbt, &dbt);
}

/* Log record (user) fields specification. */
static DB_LOG_RECSPEC kvs_file_log_rec_spec[] = {
	{
		.type   = LOGREC_DBT,
		.offset = offsetof(struct kvs_file_log_rec, path),
		.name   = "path",
		.fmt    = ""
	},
	{
		.type   = LOGREC_ARG,
		.offset = offsetof(struct kvs_file_log_rec, mode),
		.name   = "mode",
		.fmt    = "%o"
	},
	{
		.type   = LOGREC_DBT,
		.offset = offsetof(struct kvs_file_log_rec, orig),
		.name   = "orig",
		.fmt    = ""
	},
	{
		.type   = LOGREC_DBT,
		.offset = offsetof(struct kvs_file_log_rec, new),
		.name   = "new",
		.fmt    = ""
	},
	/* End of specification marker. */
	{ .type   = LOGREC_Done, 0 }
};

#define KVS_FILE_TMP_SUFFIX     ".tmp"
#define KVS_FILE_TMP_SUFFIX_LEN (sizeof(KVS_FILE_TMP_SUFFIX) - 1)
#define KVS_FILE_NAME_MAX       (NAME_MAX - KVS_FILE_TMP_SUFFIX_LEN)

static ssize_t
kvs_file_check_path(const char *path, mode_t mode)
{
	kvs_assert(path);

	ssize_t len;

	len = upath_validate_path(path, KVS_FILE_NAME_MAX);
	if (len < 0)
		return len;

	if (!upath_is_file_name(path, len))
		return -EISDIR;

	if (!mode || (mode & ~ALLPERMS))
		return -EPERM;

	return len;
}

static int
kvs_file_put_log(DB_ENV    *env,
                 DB_TXN    *txn,
                 const DBT *path,
                 mode_t     mode,
                 const DBT *orig,
                 const DBT *new)
{
	kvs_assert(env);
	kvs_assert(txn);
	kvs_assert(path);
	kvs_assert(kvs_file_check_path(path->data, mode) > 0);
	kvs_assert(path->size ==
	           (size_t)kvs_file_check_path(path->data, mode) + 1);
	kvs_assert(orig);
	kvs_assert(new);

	return kvs_put_log_rec(env,
	                       txn,
	                       KVS_FILE_LOG_REC,
	                       kvs_file_log_rec_size(path, orig, new),
	                       kvs_file_log_rec_spec,
	                       path,
	                       (uint32_t)mode,
	                       orig,
	                       new);
}

static int
kvs_file_get_log(DB_ENV                   *env,
                 const DBT                *log_dbt,
                 struct kvs_file_log_rec **log_rec)
{
	kvs_assert(env);
	kvs_assert(log_dbt);
	kvs_assert(log_rec);

	int     err;
	ssize_t len;

	err = kvs_get_log_rec(env,
	                      log_dbt,
	                      kvs_file_log_rec_min_size(),
	                      kvs_file_log_rec_spec,
	                      log_rec);
	if (err)
		return err;

	if (!(*log_rec)->path.data)
		return ENOENT;

	len = kvs_file_check_path((*log_rec)->path.data, (*log_rec)->mode);
	if (len < 0)
		return -len;

	if ((*log_rec)->path.size != ((size_t)len + 1))
		return ENOENT;

	return 0;
}

static int
kvs_file_build_at(int         dir,
                  const char *path,
                  size_t      len,
                  mode_t      mode,
                  const char *data,
                  size_t      size)
{
	kvs_assert(dir >= 0);
	kvs_assert(kvs_file_check_path(path, mode) > 0);
	kvs_assert(len == (size_t)kvs_file_check_path(path, mode));

	char *tmp;
	int   fd;
	int   ret;

	tmp = malloc(NAME_MAX);
	if (!tmp)
		return -ENOMEM;

	memcpy(tmp, path, len);
	memcpy(&tmp[len], KVS_FILE_TMP_SUFFIX, KVS_FILE_TMP_SUFFIX_LEN + 1);

	fd = ufile_nointr_new_at(dir,
	                         tmp,
	                         O_WRONLY | O_NOFOLLOW | O_TRUNC,
	                         mode);
	if (fd < 0) {
		ret = -errno;
		goto free;
	}

	ret = ufile_fchmod(fd, mode);
	if (ret < 0)
		goto close;

	ret = ufile_nointr_full_write(fd, data, size);
	if (ret)
		goto close;

	ret = ufile_close(fd);
	if (ret)
		goto unlink;

	ret = ufile_rename_at(dir, tmp, path);
	if (ret)
		goto unlink;

	free(tmp);

	return 0;

close:
	ufile_close(fd);

unlink:
	ufile_unlink_at(dir, tmp);

free:
	free(tmp);

	return ret;
}

int
kvs_file_handle_log_rec(DB_ENV    *env,
                        DBT       *log_dbt,
                        DB_LSN    *lsn,
                        db_recops  op)
{
	kvs_assert(env);
	kvs_assert(log_dbt);
	kvs_assert(lsn);

	if (op != DB_TXN_PRINT) {
		int                      ret;
		const char              *home;
		int                      dir;
		struct kvs_file_log_rec *log_rec;
		const char              *data;
		size_t                   size;

		ret = env->get_home(env, &home);
		if (ret)
			return ret;

		dir = udir_nointr_open(home, O_RDONLY | O_NOFOLLOW | O_PATH);
		if (dir < 0)
			return -dir;

		ret = kvs_file_get_log(env, log_dbt, &log_rec);
		if (ret)
			goto close;

		kvs_log_rec_dbg(env,
		                lsn,
		                op,
		                &log_rec->rec,
		                "kvs_file",
		                "    path '%s'\n"
		                "    mode %o\n"
		                "    orig %u\n"
		                "    new  %u",
		                (char *)log_rec->path.data,
		                log_rec->mode,
		                log_rec->orig.size,
		                log_rec->new.size);

		switch (op) {
		case DB_TXN_ABORT:
		case DB_TXN_BACKWARD_ROLL:
			/*
			 * If we're aborting, we need to restore file's old
			 * content.
			 * DB may attempt to undo or redo operations without
			 * knowing whether they have already been done or
			 * undone, so we should never assume in a recovery
			 * function that the task definitely needs doing or
			 * undoing.
			 */
			data = log_rec->orig.data;
			size = log_rec->orig.size;
			break;

		case DB_TXN_FORWARD_ROLL:
			/*
			 * The forward direction is just the opposite, i.e.
			 * replay last committed transaction to generate
			 * up-to-date file content.
			 */
			data = log_rec->new.data;
			size = log_rec->new.size;
			break;

		case DB_TXN_APPLY:
		default:
			kvs_assert(0);
		}

		ret = -kvs_file_build_at(dir,
		                         (char *)log_rec->path.data,
		                         log_rec->path.size - 1,
		                         log_rec->mode,
		                         data,
		                         size);

		/*
		 * The recovery function is responsible for returning the LSN of
		 * the previous log record in this transaction, so that
		 * transaction aborts can follow the chain backwards.
		 *
		 * (If we'd wanted the LSN of this record earlier, we could have
		 * read it from lsnp, as well--but because we weren't working
		 * with pages or other objects that store their LSN and base
		 * recovery decisions on it, we didn't need to.)
		 */
		*lsn = log_rec->rec.prev;

		free(log_rec);

		if (ret)
			goto close;

		return -udir_close(dir);

close:
		udir_close(dir);

		return ret;
	}

	return kvs_print_log_rec(env,
	                         log_dbt,
	                         lsn,
	                         "kvs_file",
	                         kvs_file_log_rec_spec);
}

static int
kvs_file_grow(struct kvs_file *file, size_t size)
{
	kvs_file_assert(file);
	kvs_assert(size);

	size_t sz;

	sz = ualign_upper(file->size + size, usys_page_size());

	file->data = realloc(file->data, sz);
	if (!file->data)
		return -ENOMEM;

	file->size = sz;

	return 0;
}

int __printf(2, 3)
kvs_file_printf(struct kvs_file *file, const char *format, ...)
{
	kvs_file_assert(file);
	kvs_assert(format);
	kvs_assert(*format);

	va_list args;
	size_t  free = file->size - file->off;
	int     bytes;
	int     ret;

	va_start(args, format);

	bytes = vsnprintf(&file->data[file->off], free, format, args);
	if (bytes < 0) {
		ret = -errno;
		goto end;
	}

	if ((size_t)bytes >= free) {
		if (kvs_file_grow(file, bytes + 1)) {
			ret = -ENOMEM;
			goto end;
		}

		free = file->size - file->off;
		bytes = vsnprintf(&file->data[file->off], free, format, args);
		if (bytes < 0) {
			ret = -errno;
			goto end;
		}
	}

	file->off += bytes;
	ret = 0;

end:
	va_end(args);

	return ret;
}

int
kvs_file_write(struct kvs_file *file, const void *data, size_t size)
{
	kvs_file_assert(file);
	kvs_assert(data);
	kvs_assert(size);

	if (size > (file->size - file->off))
		if (kvs_file_grow(file, size))
			return -ENOMEM;

	memcpy(&file->data[file->off], data, size);
	file->off += size;

	return 0;
}

int
kvs_file_realize(struct kvs_file        *file,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode)
{
	kvs_file_assert(file);
	kvs_assert_xact(xact);
	kvs_assert(path);
	kvs_assert(mode);

	ssize_t len;
	DBT     path_dbt = { .data = (void *)path, 0, };
	DBT     orig_dbt = { 0, };
	DBT     new_dbt = { .data = file->data, .size = file->off, 0, };
	int     fd;
	int     ret;

	len = kvs_file_check_path(path, mode);
	if (len < 0)
		return len;

	fd = ufile_nointr_open_at(file->dir, path, O_RDONLY | O_NOFOLLOW);
	if (fd >= 0) {
		struct stat st;

		ret = ufile_fstat(fd, &st);
		if (ret)
			goto close;

		orig_dbt.size = (size_t)st.st_size;
		orig_dbt.data = malloc(orig_dbt.size);
		if (!orig_dbt.data)
			goto close;

		ret = ufile_nointr_full_read(fd, orig_dbt.data, orig_dbt.size);
		if (ret)
			goto free;
	}
	else {
		if (fd != -ENOENT)
			return fd;

		orig_dbt.data = NULL;
		orig_dbt.size = 0;
	}

	path_dbt.size = len + 1;
	ret = kvs_file_put_log(file->env,
	                       xact->txn,
	                       &path_dbt,
	                       mode,
	                       &orig_dbt,
	                       &new_dbt);
	if (ret) {
		ret = kvs_err_from_bdb(ret);
		goto free;
	}

	ret = kvs_file_build_at(file->dir,
	                        path,
	                        len,
	                        mode,
	                        file->data,
	                        file->off);

free:
	free(orig_dbt.data);

	if (fd >= 0) {
close:
		ufile_close(fd);
	}

	return ret;
}

int
kvs_file_init(struct kvs_file *file, const struct kvs_depot *depot)
{
	kvs_assert(file);
	kvs_assert_depot(depot);

	const char *home;
	int         err;

	err = depot->env->get_home(depot->env, &home);
	if (err)
		return kvs_err_from_bdb(err);

	file->dir = udir_nointr_open(home, O_RDONLY | O_NOFOLLOW | O_PATH);
	if (file->dir < 0)
		return file->dir;

	file->off = 0;
	file->size = 0;
	file->data = NULL;
	file->env = depot->env;

	return 0;
}

void
kvs_file_fini(const struct kvs_file *file)
{
	kvs_file_assert(file);

	free(file->data);
	ufile_close(file->dir);
}
