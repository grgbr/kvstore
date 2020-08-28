#include "common.h"
#include <kvstore/autoidx.h>
#include <string.h>

const struct kvs_autoidx_id kvs_autoidx_none = { .rid = { 0, } };

#define kvs_autoidx_assert_desc(_desc) \
	kvs_assert(!!(_desc)->data ^ !!kvs_assert((_desc)->size))

static int
kvs_autoidx_fill_desc(const DBT               *key,
                      const DBT               *item,
                      struct kvs_autoidx_desc *desc)
{
	kvs_assert(key->data);
	kvs_assert(key->size == sizeof(desc->id.rid));
	kvs_assert(item);
	kvs_assert(desc);

	if (!(!!item->data ^ !!item->size))
		return -EBADMSG;

	desc->id.rid = *(DB_HEAP_RID *)key->data;
	desc->data = item->data;
	desc->size = item->size;

	return 0;
}

int
kvs_autoidx_iter_first(const struct kvs_iter   *iter,
                       struct kvs_autoidx_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &item);
	if (err)
		return err;

	return kvs_autoidx_fill_desc(&key, &item, desc);
}

int
kvs_autoidx_iter_next(const struct kvs_iter   *iter,
                      struct kvs_autoidx_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &item);
	if (err)
		return err;

	return kvs_autoidx_fill_desc(&key, &item, desc);
}

int
kvs_autoidx_init_iter(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      struct kvs_iter        *iter)
{
	return kvs_init_iter(store, xact, iter);
}

int
kvs_autoidx_fini_iter(const struct kvs_iter *iter)
{
	return kvs_fini_iter(iter);
}

ssize_t
kvs_autoidx_get(const struct kvs_store  *store,
                const struct kvs_xact   *xact,
                struct kvs_autoidx_id    id,
                const void             **data)
{
	kvs_assert(data);

	DBT key = { .data = (void *)&id.rid, .size = sizeof(id.rid), 0 };
	DBT item = { 0 };
	int ret;

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	if (!(!!item.data ^ !!item.size))
		return -EBADMSG;

	kvs_assert(key.size == sizeof(id.rid));
	kvs_assert(!memcmp(key.data, &id, sizeof(id.rid)));

	*data = item.data;

	return item.size;
}

int
kvs_autoidx_get_desc(const struct kvs_store  *store,
                     const struct kvs_xact   *xact,
                     struct kvs_autoidx_id    id,
                     struct kvs_autoidx_desc *desc)
{
	kvs_assert(desc);

	int ret;

	ret = kvs_autoidx_get(store, xact, id, &desc->data);
	if (ret < 0)
		return ret;

	desc->id = id;
	desc->size = (size_t)ret;

	return 0;
}

int
kvs_autoidx_update(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_autoidx_id   id,
                   const void             *data,
                   size_t                  size)
{
	kvs_assert(!!data ^ !!size);

	DBT key = { .data = (void *)&id.rid, .size = sizeof(id.rid), 0 };
	DBT item = { .data = (void *)data, .size = size, 0 };

	return kvs_put(store, xact, &key, &item, 0);
}

int
kvs_autoidx_add(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autoidx_id  *id,
                const void             *data,
                size_t                  size)
{
	kvs_assert(id);
	kvs_assert(!!data ^ !!size);

	DBT key = { 0, };
	DBT item = { .data = (void *)data, .size = size, 0 };
	int ret;

	ret = kvs_put(store, xact, &key, &item, DB_APPEND | DB_NOOVERWRITE);
	if (ret)
		return ret;

	kvs_assert(key.data);
	kvs_assert(key.size == sizeof(id->rid));

	id->rid = *(DB_HEAP_RID *)key.data;

	return 0;
}

int
kvs_autoidx_del(const struct kvs_store *store,
                const struct kvs_xact  *xact,
                struct kvs_autoidx_id   id)
{
	DBT key = { .data = (void *)&id.rid, .size = sizeof(id.rid), 0 };

	return kvs_del(store, xact, &key);
}

int
kvs_autoidx_open(struct kvs_store       *store,
                const struct kvs_depot *depot,
                const struct kvs_xact  *xact,
                const char             *path,
                mode_t                  mode)
{
	/* Heap databases don't support named sub-databases. */
	return kvs_open_store(store, depot, xact, path, NULL, DB_HEAP, mode);
}

int
kvs_autoidx_close(const struct kvs_store *store)
{
	return kvs_close_store(store);
}
