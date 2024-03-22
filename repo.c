#include <kvstore/repo.h>
#include <kvstore/table.h>
#include <errno.h>

static int
kvs_repo_do_close(const struct kvs_repo *repo, unsigned int count)
{
	int ret = 0;
	int err;

	while (count && !ret) {
		count--;
		ret = kvs_table_close(&repo->tables[count]);
	}

	if (!ret)
		return kvs_close_depot(&repo->depot);

	err = ret;
	while (count && (err != DB_RUNRECOVERY)) {
		count--;
		err = kvs_table_close(&repo->tables[count]);
	}

	if (err != DB_RUNRECOVERY)
		err = kvs_close_depot(&repo->depot);

	return (err != DB_RUNRECOVERY) ? ret : DB_RUNRECOVERY;
}

int
kvs_repo_open(struct kvs_repo *repo,
              const char      *path,
              size_t           max_log,
              unsigned int     flags,
              mode_t           mode)
{
	kvs_repo_assert(repo);

	int             err;
	struct kvs_xact xact;
	unsigned int    cnt;
	unsigned int    nr;

	err = kvs_open_depot(&repo->depot, path, max_log, flags, mode);
	if (err)
		return err;

	cnt = 0;
	err = kvs_begin_xact(&repo->depot, NULL, &xact, 0);
	if (err)
		goto close;

	nr = repo->desc->tbl_nr;
	do {
		err = kvs_table_open(&repo->tables[cnt],
		                     &repo->depot,
		                     &xact,
		                     mode);
		cnt++;
	} while (!err && (cnt < nr));

	err = kvs_complete_xact(&xact, err);
	if (err)
		goto close;

	return 0;

close:
	if (err != DB_RUNRECOVERY)
		if (kvs_repo_do_close(repo, cnt) == DB_RUNRECOVERY)
			return DB_RUNRECOVERY;

	return err;
}

int
kvs_repo_close(const struct kvs_repo *repo)
{
	kvs_repo_assert(repo);

	return kvs_repo_do_close(repo, repo->desc->tbl_nr);
}

static void
kvs_repo_do_exit(const struct kvs_repo *repo, unsigned int count)
{
	while (count--)
		kvs_table_exit(&repo->tables[count]);

	free(repo->tables);
}

static int
kvs_repo_init(struct kvs_repo *repo, const struct kvs_repo_desc *desc)
{
	kvs_assert(repo);
	kvs_repo_assert_desc(desc);

	unsigned int t;
	int          err;

	repo->tables = malloc(desc->tbl_nr * sizeof(repo->tables[0]));
	if (!repo->tables)
		return -errno;

	for (t = 0; t < desc->tbl_nr; t++) {
		err = kvs_table_init(&repo->tables[t], desc->tables[t]);
		if (err)
			break;
	}

	if (err)
		goto exit;

	repo->desc = desc;

	return 0;

exit:
	kvs_repo_do_exit(repo, t);

	return err;
}

static void
kvs_repo_exit(const struct kvs_repo *repo)
{
	kvs_repo_assert(repo);

	kvs_repo_do_exit(repo, repo->desc->tbl_nr);
}

struct kvs_repo *
kvs_repo_create(const struct kvs_repo_desc *desc)
{
	struct kvs_repo *repo;
	int              err;

	repo = malloc(sizeof(*repo));
	if (!repo)
		return NULL;

	err = kvs_repo_init(repo, desc);
	if (err)
		goto free;

	return repo;

free:
	free(repo);

	errno = -err;
	return NULL;
}

void
kvs_repo_destroy(struct kvs_repo *repo)
{
	kvs_repo_exit(repo);

	free(repo);
}
