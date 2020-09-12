#include "common.h"
#include <kvstore/autorec.h>
#include <string.h>

/*
 * Definition of THE invalid automatic identifier.
 * Watch out ! BDB stores heap IDs in a packed manner, which
 * struct kvs_autorec_id is not.
 * We could perform assignments / comparisons using internal fields of the
 * DB_HEAP_RID typedef (struct __db_heap_rid). For portability / sustainability
 * purposes this is not the way it is implemented however.
 * Hence the memcmp that may be found at various places throughout this file.
 */
const struct kvs_autorec_id kvs_autorec_none = { .rid = { 0, } };

#define kvs_autorec_assert_desc(_desc) \
	kvs_assert((_desc)->data || !(_desc)->size)

bool
kvs_autorec_id_isok(struct kvs_autorec_id id)
{
	return memcmp(&id.rid, &kvs_autorec_none.rid, DB_HEAP_RID_SZ);
}

static int
kvs_autorec_fill_desc(const DBT               *key,
                      const DBT               *item,
                      struct kvs_autorec_desc *desc)
{
	kvs_assert(key->data);
	kvs_assert(key->size == DB_HEAP_RID_SZ);
	kvs_assert(memcmp(key->data, &kvs_autorec_none.rid, DB_HEAP_RID_SZ));
	kvs_assert(item);
	kvs_assert(desc);

	if (item->size && !item->data)
		return -EBADMSG;

	memcpy(&desc->id.rid, key->data, DB_HEAP_RID_SZ);
	desc->data = item->data;
	desc->size = item->size;

	return 0;
}

int
kvs_autorec_iter_first(const struct kvs_iter   *iter,
                       struct kvs_autorec_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &item);
	if (err)
		return err;

	return kvs_autorec_fill_desc(&key, &item, desc);
}

int
kvs_autorec_iter_next(const struct kvs_iter   *iter,
                      struct kvs_autorec_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &item);
	if (err)
		return err;

	return kvs_autorec_fill_desc(&key, &item, desc);
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

ssize_t
kvs_autorec_get(const struct kvs_store  *store,
                const struct kvs_xact   *xact,
                struct kvs_autorec_id    id,
                const void             **data)
{
	kvs_assert(kvs_autorec_id_isok(id));
	kvs_assert(data);

	DBT key = { .data = (void *)&id.rid, .size = DB_HEAP_RID_SZ, 0, };
	DBT item = { 0 };
	int ret;

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	if (item.size && !item.data)
		return -EBADMSG;

	kvs_assert(!memcmp(key.data, &id.rid, DB_HEAP_RID_SZ));
	kvs_assert(key.size == DB_HEAP_RID_SZ);

	*data = item.data;

	return item.size;
}

ssize_t
kvs_autorec_get_byfield(const struct kvs_store  *index,
                        const struct kvs_xact   *xact,
                        const struct kvs_chunk  *field,
                        struct kvs_autorec_id   *id,
                        const void             **data)
{
	kvs_assert_chunk(field);
	kvs_assert(id);
	kvs_assert(data);

	DBT ikey = { .data = (void *)field->data, .size = field->size, 0, };
	DBT pkey = { 0, };
	DBT item = { 0, };
	int ret;

	ret = kvs_pget(index, xact, &ikey, &pkey, &item, 0);
	if (ret < 0)
		return ret;

	if (item.size && !item.data)
		return -EBADMSG;

	kvs_assert(pkey.data);
	kvs_assert(pkey.size == DB_HEAP_RID_SZ);
	kvs_assert(item.data);
	kvs_assert(item.size);

	memcpy(&id->rid, pkey.data, DB_HEAP_RID_SZ);
	*data = item.data;

	return item.size;
}

int
kvs_autorec_get_desc(const struct kvs_store  *store,
                     const struct kvs_xact   *xact,
                     struct kvs_autorec_id    id,
                     struct kvs_autorec_desc *desc)
{
	kvs_assert(desc);

	int ret;

	ret = kvs_autorec_get(store, xact, id, &desc->data);
	if (ret < 0)
		return ret;

	desc->id = id;
	desc->size = (size_t)ret;

	return 0;
}

int
kvs_autorec_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_autorec_id   id,
                   const void             *data,
                   size_t                  size)
{
	kvs_assert(kvs_autorec_id_isok(id));
	kvs_assert(data || !size);

	DBT key = { .data = (void *)&id.rid, .size = DB_HEAP_RID_SZ, 0, };
	DBT item = { .data = (void *)data, .size = size, 0 };

	return kvs_put(store, xact, &key, &item, 0);
}

int
kvs_autorec_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autorec_id  *id,
                const void             *data,
                size_t                  size)
{
	kvs_assert(id);
	kvs_assert(data || !size);

	DBT key = { 0, };
	DBT item = { .data = (void *)data, .size = size, 0, };
	int ret;

	ret = kvs_put(store, xact, &key, &item, DB_APPEND);
	if (ret)
		return ret;

	kvs_assert(key.data);
	kvs_assert(key.size == DB_HEAP_RID_SZ);

	memcpy(&id->rid, key.data, DB_HEAP_RID_SZ);

	return 0;
}

int
kvs_autorec_del(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autorec_id   id)
{
	kvs_assert(kvs_autorec_id_isok(id));

	DBT key = { .data = (void *)&id.rid, .size = DB_HEAP_RID_SZ, 0, };

	return kvs_del(store, xact, &key);
}

int
kvs_autorec_del_byfield(const struct kvs_store *index,
                        const struct kvs_xact  *xact,
                        const struct kvs_chunk *field)
{
	kvs_assert_chunk(field);

	DBT key = { .data = (void *)field->data, .size = field->size, 0, };

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
