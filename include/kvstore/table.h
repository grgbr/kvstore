#ifndef _KVS_TABLE_H
#define _KVS_TABLE_H

#include <kvstore/store.h>
#include <stdlib.h>

struct kvs_table;

typedef int (kvs_table_open_store_fn)(struct kvs_table       *table,
                                      const struct kvs_depot *depot,
                                      const struct kvs_xact  *xact,
                                      mode_t                  mode);

typedef int (kvs_table_close_store_fn)(const struct kvs_table *table);

struct kvs_table_store_ops {
	kvs_table_open_store_fn  *open;
	kvs_table_close_store_fn *close;
};

#define kvs_table_assert_store_ops(_ops) \
	kvs_assert(_ops); \
	kvs_assert((_ops)->open); \
	kvs_assert((_ops)->close)

struct kvs_table_desc {
	struct kvs_table_store_ops data_ops;
	unsigned int               indx_nr;
	struct kvs_table_store_ops indx_ops[];
};

#if defined(CONFIG_KVSTORE_ASSERT)

#include <utils/assert.h>

#define kvs_table_assert_desc(_desc) \
	({ \
		const struct kvs_table_desc *__desc = _desc; \
		unsigned int                 __i; \
		\
		kvs_assert(__desc); \
		kvs_table_assert_store_ops(&__desc->data_ops); \
		for (__i = 0; __i < __desc->indx_nr; __i++) \
			kvs_table_assert_store_ops(&__desc->indx_ops[__i]); \
	 })

#else  /* !defined(CONFIG_KVS_ASSERT) */

#define kvs_table_assert_desc(_desc)

#endif /* defined(CONFIG_KVS_ASSERT) */

struct kvs_table {
	struct kvs_store             data;
	unsigned int                 indx_cnt;
	struct kvs_store            *indices;
	const struct kvs_table_desc *desc;
};

#define kvs_table_assert(_table) \
	kvs_assert(_table); \
	kvs_table_assert_desc((_table)->desc); \
	kvs_assert(!(_table)->desc->indx_nr || (_table)->indices)


static inline struct kvs_store *
kvs_table_get_data_store(const struct kvs_table *table)
{
	kvs_table_assert(table);

	return (struct kvs_store *)&table->data;
}

static inline struct kvs_store *
kvs_table_get_indx_store(const struct kvs_table *table, unsigned int indx_id)
{
	kvs_table_assert(table);
	kvs_assert(indx_id < table->desc->indx_nr);

	return &table->indices[indx_id];
}

extern int
kvs_table_open(struct kvs_table       *table,
               const struct kvs_depot *depot,
               const struct kvs_xact  *xact,
               mode_t                  mode);

extern int
kvs_table_close(const struct kvs_table *table);

extern int
kvs_table_init(struct kvs_table *table, const struct kvs_table_desc *desc);

static inline void
kvs_table_exit(const struct kvs_table *table)
{
	kvs_table_assert(table);

	if (table->desc->indx_nr)
		free(table->indices);
}

#endif /* _KVS_TABLE_H */
