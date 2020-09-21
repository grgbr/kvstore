#ifndef _KVS_REPO_H
#define _KVS_REPO_H

#include <kvstore/table.h>

struct kvs_repo_desc {
	unsigned int                 tbl_nr;
	const struct kvs_table_desc *tables[];
};

#define kvs_repo_assert_desc(_desc) \
	kvs_assert(_desc); \
	kvs_assert((_desc)->tbl_nr && (_desc)->tables)

struct kvs_repo {
	struct kvs_table           *tables;
	struct kvs_depot            depot;
	const struct kvs_repo_desc *desc;
};

#define kvs_repo_assert(_repo) \
	kvs_assert(_repo); \
	kvs_assert((_repo)->tables); \
	kvs_repo_assert_desc((_repo)->desc)

static inline int
kvs_repo_begin_xact(const struct kvs_repo *repo,
                    const struct kvs_xact *parent,
                    struct kvs_xact       *xact,
                    unsigned int           flags)
{
	kvs_repo_assert(repo);

	return kvs_begin_xact(&repo->depot, parent, xact, flags);
}

static inline const struct kvs_table *
kvs_repo_get_table(const struct kvs_repo *repo, unsigned int tbl_id)
{
	kvs_repo_assert(repo);
	kvs_assert(tbl_id < repo->desc->tbl_nr);

	return &repo->tables[tbl_id];
}

extern int
kvs_repo_open(struct kvs_repo *repo,
              const char      *path,
              size_t           max_log,
              unsigned int     flags,
              mode_t           mode);

extern int
kvs_repo_close(const struct kvs_repo *repo);

extern struct kvs_repo *
kvs_repo_create(const struct kvs_repo_desc *desc);

extern void
kvs_repo_destroy(struct kvs_repo *repo);

#endif /* _KVS_REPO_H */
