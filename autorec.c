#include "common.h"
#include <kvstore/autorec.h>
#include <string.h>
#include <errno.h>

#define KVS_AUTOREC_INIT_RID(_id) \
	{ \
		.pgno = (uint64_t)(_id) >> \
		        (sizeof_member(DB_HEAP_RID, indx) * CHAR_BIT), \
		.indx = (uint64_t)(_id) & \
		        ((1U << \
		          (sizeof_member(DB_HEAP_RID, indx) * CHAR_BIT)) - 1) \
	}

#define KVS_AUTOREC_INIT_KEY(_rid) \
	{ \
		.data = _rid, \
		.size = DB_HEAP_RID_SZ, \
		0, \
	}

static uint64_t
kvs_autorec_key_to_id(const DBT *key)
{
	kvs_assert(key);
	kvs_assert(key->data);
	kvs_assert(key->size == DB_HEAP_RID_SZ);

	const DB_HEAP_RID *rid = key->data;

	kvs_assert(rid->pgno);

	/*
	 * Watch out ! BDB stores heap IDs in a packed manner, which DB_HEAP_RID
	 * typedef is not.
	 * This is the reason why we perform assignments using DB_HEAP_RID's
	 * internal fields.
	 */
	return (uint64_t)(rid->pgno << (sizeof(rid->indx) * CHAR_BIT)) |
	       (uint64_t)rid->indx;
}

static void
kvs_autorec_fill_rec(const DBT        *key,
                     uint64_t         *id,
                     const DBT        *itm,
                     struct kvs_chunk *item)
{
	kvs_assert(key);
	kvs_assert(id);
	kvs_assert(itm);
	kvs_assert(itm->data || !itm->size);
	kvs_assert(item);

	*id = kvs_autorec_key_to_id(key);

	item->data = itm->data;
	item->size = itm->size;
}

int
kvs_autorec_iter_first(const struct kvs_iter *iter,
                       uint64_t              *id,
                       struct kvs_chunk      *item)
{
	DBT key = { 0, };
	DBT itm = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &itm);
	if (err)
		return err;

	kvs_autorec_fill_rec(&key, id, &itm, item);

	return 0;
}

int
kvs_autorec_iter_next(const struct kvs_iter *iter,
                      uint64_t              *id,
                      struct kvs_chunk      *item)
{
	DBT key = { 0, };
	DBT itm = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &itm);
	if (err)
		return err;

	kvs_autorec_fill_rec(&key, id, &itm, item);

	return 0;
}

int
kvs_autorec_init_iter(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      struct kvs_iter        *iter)
{
	return kvs_init_iter(store, xact, iter);
}

int
kvs_autorec_fini_iter(const struct kvs_iter *iter)
{
	return kvs_fini_iter(iter);
}

int
kvs_autorec_get_byid(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     uint64_t                id,
                     struct kvs_chunk       *item)
{
	kvs_assert(kvs_autorec_id_isok(id));
	kvs_assert(item);

	DB_HEAP_RID rid = KVS_AUTOREC_INIT_RID(id);
	DBT         key = KVS_AUTOREC_INIT_KEY(&rid);
	DBT         itm = { 0 };
	int         ret;

	ret = kvs_get(store, xact, &key, &itm, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	kvs_assert(key.size == DB_HEAP_RID_SZ);
	kvs_assert(!memcmp(key.data, &rid, DB_HEAP_RID_SZ));
	kvs_assert(itm.data || !itm.size);

	item->size = itm.size;
	item->data = itm.data;

	return 0;
}

int
kvs_autorec_get_byfield(const struct kvs_store *index,
                        const struct kvs_xact  *xact,
                        const struct kvs_chunk *field,
                        uint64_t               *id,
                        struct kvs_chunk       *item)
{
	kvs_assert(index);
	kvs_assert(index->db);
	kvs_assert(index->db->app_private);
	kvs_assert(id);
	kvs_assert(item);

	DBT ikey = KVS_CHUNK_INIT_DBT(field);
	DBT pkey = { 0, };
	DBT itm = { 0, };
	int ret;

	ret = kvs_pget(index, xact, &ikey, &pkey, &itm, 0);
	if (ret < 0)
		return ret;

	*id = kvs_autorec_key_to_id(&pkey);

	kvs_assert(itm.data);
	kvs_assert(itm.size);
	item->size = itm.size;
	item->data = itm.data;

	return 0;
}

int
kvs_autorec_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                uint64_t               *id,
                const struct kvs_chunk *item)
{
	kvs_assert(id);
	kvs_assert(item);

	DBT key = { 0, };
	DBT itm = KVS_CHUNK_INIT_DBT(item);
	int ret;

	ret = kvs_put(store, xact, &key, &itm, DB_APPEND);
	if (ret)
		return ret;

	*id = kvs_autorec_key_to_id(&key);

	return 0;
}

int
kvs_autorec_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   uint64_t                id,
                   const struct kvs_chunk *item)
{
	kvs_assert(kvs_autorec_id_isok(id));
	kvs_assert(item);

	DB_HEAP_RID rid = KVS_AUTOREC_INIT_RID(id);
	DBT         key = KVS_AUTOREC_INIT_KEY(&rid);
	DBT         itm = KVS_CHUNK_INIT_DBT(item);

	return kvs_put(store, xact, &key, &itm, 0);
}

int
kvs_autorec_del_byid(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     uint64_t                id)
{
	kvs_assert(kvs_autorec_id_isok(id));

	DB_HEAP_RID rid = KVS_AUTOREC_INIT_RID(id);
	DBT         key = KVS_AUTOREC_INIT_KEY(&rid);

	return kvs_del(store, xact, &key);
}

int
kvs_autorec_del_byfield(const struct kvs_store *index,
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
kvs_autorec_open(struct kvs_store       *store,
                 const struct kvs_depot *depot,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode)
{
	/* Heap databases don't support named sub-databases. */
	return kvs_open_store(store, depot, xact, path, NULL, DB_HEAP, mode);
}

int
kvs_autorec_close(const struct kvs_store *store)
{
	return kvs_close_store(store);
}
