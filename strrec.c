#include "common.h"
#include <kvstore/strrec.h>
#include <string.h>

#define kvs_strrec_assert_desc(_desc) \
	kvs_assert(_desc); \
	kvs_assert((_desc)->id); \
	kvs_assert(*(_desc)->id); \
	kvs_assert((_desc)->len); \
	kvs_assert((_desc)->len < KVS_STR_MAX); \
	kvs_assert(!!(_desc)->data ^ !! kvs_assert((_desc)->size))

static int
kvs_strrec_fill_desc(const DBT              *key,
                     const DBT              *item,
                     struct kvs_strrec_desc *desc)
{
	kvs_assert(key->data);
	kvs_assert(key->size);
	kvs_assert(item);
	kvs_assert(desc);

	if (!((char *)key->data)[0] || (key->size >= KVS_STR_MAX))
		return -EKEYREJECTED;

	if (!(!!item->data ^ !!item->size))
		return -EBADMSG;

	desc->id = (char *)key->data;
	desc->len = key->size;
	desc->data = item->data;
	desc->size = item->size;

	return 0;
}

int
kvs_strrec_iter_first(const struct kvs_iter *iter, struct kvs_strrec_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &item);
	if (err)
		return err;

	return kvs_strrec_fill_desc(&key, &item, desc);
}

int
kvs_strrec_iter_next(const struct kvs_iter *iter, struct kvs_strrec_desc *desc)
{
	kvs_assert(desc);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &item);
	if (err)
		return err;

	return kvs_strrec_fill_desc(&key, &item, desc);
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

#define kvs_strrec_assert_id(_id, _len) \
	kvs_assert(_id); \
	kvs_assert(*(_id)); \
	kvs_assert(_len); \
	kvs_assert((_len) < KVS_STR_MAX)

ssize_t
kvs_strrec_get(const struct kvs_store  *store,
               const struct kvs_xact   *xact,
               const char              *id,
               size_t                   len,
               const void             **data)
{
	kvs_strrec_assert_id(id, len);
	kvs_assert(data);

	DBT key = { .data = (void *)id, .size = len, 0 };
	DBT item = { 0 };
	int ret;

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	if (!(!!item.data ^ !!item.size))
		return -EBADMSG;

	kvs_assert(key.size == len);
	kvs_assert(!memcmp(key.data, id, len));

	*data = item.data;

	return item.size;
}

int
kvs_strrec_get_desc(const struct kvs_store  *store,
                    const struct kvs_xact   *xact,
                    const char              *id,
                    size_t                   len,
                    struct kvs_strrec_desc  *desc)
{
	kvs_assert(desc);

	int ret;

	ret = kvs_strrec_get(store, xact, id, len, &desc->data);
	if (ret < 0)
		return ret;

	desc->id = id;
	desc->len = len;
	desc->size = (size_t)ret;

	return 0;
}

int
kvs_strrec_put(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const char             *id,
               size_t                  len,
               const void             *data,
               size_t                  size)
{
	kvs_strrec_assert_id(id, len);
	kvs_assert(!!data ^ !!size);

	DBT key = { .data = (void *)id, .size = len, 0 };
	DBT item = { .data = (void *)data, .size = size, 0 };

	return kvs_put(store, xact, &key, &item, 0);
}

int
kvs_strrec_add(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const char             *id,
               size_t                  len,
               const void             *data,
               size_t                  size)
{
	kvs_strrec_assert_id(id, len);
	kvs_assert(!!data ^ !!size);

	DBT key = { .data = (void *)id, .size = len, 0 };
	DBT item = { .data = (void *)data, .size = size, 0 };

	return kvs_put(store, xact, &key, &item, DB_NOOVERWRITE);
}

int
kvs_strrec_del(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               const char             *id,
               size_t                  len)
{
	kvs_strrec_assert_id(id, len);

	DBT key = { .data = (void *)id, .size = len, 0 };

	return kvs_del(store, xact, &key);
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
