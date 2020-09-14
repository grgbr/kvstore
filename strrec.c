#include "common.h"
#include <kvstore/strrec.h>
#include <utils/string.h>

#define kvs_strrec_assert_id(_id) \
	kvs_assert(_id); \
	kvs_assert((_id)->data); \
	kvs_assert(ustr_parse((_id)->data, KVS_STR_MAX) == (ssize_t)(_id)->size)

#define KVS_STRREC_INIT_KEY(_id) \
	{ \
		.data = (void *)(_id)->data, \
		.size = (_id)->size, \
		0, \
	}

static void
kvs_strrec_fill_rec(const DBT        *key,
                    struct kvs_chunk *id,
                    const DBT        *itm,
                    struct kvs_chunk *item)
{
	kvs_assert(key);
	kvs_assert(id);
	kvs_assert(itm);
	kvs_assert(itm->data || !itm->size);
	kvs_assert(item);

	id->size = key->size;
	id->data = key->data;
	kvs_strrec_assert_id(id);

	item->data = itm->data;
	item->size = itm->size;
}

int
kvs_strrec_iter_first(const struct kvs_iter *iter,
                      struct kvs_chunk      *id,
                      struct kvs_chunk      *item)
{
	DBT key = { 0, };
	DBT itm = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &itm);
	if (err)
		return err;

	kvs_strrec_fill_rec(&key, id, &itm, item);

	return 0;
}

int
kvs_strrec_iter_next(const struct kvs_iter *iter,
                     struct kvs_chunk      *id,
                     struct kvs_chunk      *item)
{
	DBT key = { 0, };
	DBT itm = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &itm);
	if (err)
		return err;

	kvs_strrec_fill_rec(&key, id, &itm, item);

	return 0;
}

int
kvs_strrec_init_iter(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     struct kvs_iter        *iter)
{
	return kvs_init_iter(store, xact, iter);
}

int
kvs_strrec_fini_iter(const struct kvs_iter *iter)
{
	return kvs_fini_iter(iter);
}

int
kvs_strrec_get_byid(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    const struct kvs_chunk *id,
                    struct kvs_chunk       *item)
{
	kvs_strrec_assert_id(id);
	kvs_assert(item);

	DBT key = KVS_STRREC_INIT_KEY(id);
	DBT itm = { 0 };
	int ret;

	ret = kvs_get(store, xact, &key, &itm, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	kvs_assert(!memcmp(key.data, id->data, key.size));
	kvs_assert(key.size == id->size);

	item->size = itm.size;
	item->data = itm.data;

	return 0;
}

int
kvs_strrec_get_byfield(const struct kvs_store *index,
                       const struct kvs_xact  *xact,
                       const struct kvs_chunk *field,
                       struct kvs_chunk       *id,
                       struct kvs_chunk       *item)
{
	kvs_assert(index);
	kvs_assert(index->db);
	kvs_assert(index->db->app_private);
	kvs_assert(field);
	kvs_assert(id);
	kvs_assert(item);

	DBT ikey = KVS_CHUNK_INIT_DBT(field);
	DBT pkey = { 0, };
	DBT itm = { 0, };
	int ret;

	ret = kvs_pget(index, xact, &ikey, &pkey, &itm, 0);
	if (ret < 0)
		return ret;

	id->size = pkey.size;
	id->data = pkey.data;
	kvs_strrec_assert_id(id);

	kvs_assert(itm.data);
	kvs_assert(itm.size);
	item->size = itm.size;
	item->data = itm.data;

	return 0;
}

int
kvs_strrec_add(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const struct kvs_chunk *id,
               const struct kvs_chunk *item)
{
	kvs_strrec_assert_id(id);
	kvs_assert(item);

	DBT key = KVS_STRREC_INIT_KEY(id);
	DBT itm = KVS_CHUNK_INIT_DBT(item);

	return kvs_put(store, xact, &key, &itm, DB_NOOVERWRITE);
}

int
kvs_strrec_put(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const struct kvs_chunk *id,
               const struct kvs_chunk *item)
{
	kvs_strrec_assert_id(id);
	kvs_assert(item);

	DBT key = KVS_STRREC_INIT_KEY(id);
	DBT itm = KVS_CHUNK_INIT_DBT(item);

	return kvs_put(store, xact, &key, &itm, 0);
}

int
kvs_strrec_del_byid(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    const struct kvs_chunk *id)
{
	kvs_strrec_assert_id(id);

	DBT key = KVS_STRREC_INIT_KEY(id);

	return kvs_del(store, xact, &key);
}

int
kvs_strrec_del_byfield(const struct kvs_store *index,
                       const struct kvs_xact  *xact,
                       const struct kvs_chunk *field)
{
	kvs_assert(index);
	kvs_assert(index->db);
	kvs_assert(index->db->app_private);
	kvs_assert(field);

	DBT key = KVS_CHUNK_INIT_DBT(field);

	return kvs_del(index, xact, &key);
}

int
kvs_strrec_open(struct kvs_store       *store,
                const struct kvs_depot *depot,
                const struct kvs_xact  *xact,
                const char             *path,
                const char             *name,
                mode_t                  mode)
{
	return kvs_open_store(store, depot, xact, path, name, DB_BTREE, mode);
}

int
kvs_strrec_close(const struct kvs_store *store)
{
	return kvs_close_store(store);
}
