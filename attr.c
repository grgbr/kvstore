#include "common.h"
#include <kvstore/attr.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * Attribute store / iterator handling
 ******************************************************************************/

static int
kvs_attr_iter_get_elem(const DBT                 *key,
                       const DBT                 *item,
                       struct kvs_attr_iter_elem *elem)
{
	if (!key->data)
		return -EBADMSG;
	if (key->size != sizeof(db_recno_t))
		return -EMSGSIZE;

	if (!item->data)
		return -ENODATA;

	elem->id = *(unsigned int *)key->data - 1;
	elem->size = item->size;
	elem->data = item->data;

	return 0;
}

int
kvs_attr_iter_first(const struct kvs_iter     *iter,
                    struct kvs_attr_iter_elem *elem)
{
	kvs_assert(elem);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_first(iter, &key, &item);
	if (err)
		return err;

	return kvs_attr_iter_get_elem(&key, &item, elem);
}

int
kvs_attr_iter_next(const struct kvs_iter     *iter,
                   struct kvs_attr_iter_elem *elem)
{
	kvs_assert(elem);

	DBT key = { 0, };
	DBT item = { 0, };
	int err;

	err = kvs_iter_goto_next(iter, &key, &item);
	if (err)
		return err;

	return kvs_attr_iter_get_elem(&key, &item, elem);
}

int
kvs_attr_init_iter(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_iter        *iter)
{
	return kvs_init_iter(store, xact, iter);
}

int
kvs_attr_fini_iter(const struct kvs_iter *iter)
{
	return kvs_fini_iter(iter);
}

static const void *
kvs_attr_load_num(const struct kvs_store *store,
                  const struct kvs_xact  *xact,
                  unsigned int            attr_id,
                  size_t                  size)
{
	kvs_assert(attr_id < UINT_MAX);
	kvs_assert(size);

	int ret;

	db_recno_t id = (db_recno_t)attr_id + 1;
	DBT        key = { .data = &id, .size = sizeof(id), 0 };
	DBT        item = { 0 };

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0) {
		errno = -ret;
		return NULL;
	}

	if (item.size != size) {
		errno = EMSGSIZE;
		return NULL;
	}

	if (!item.data) {
		errno = ENODATA;
		return NULL;
	}

	return item.data;
}

int
kvs_attr_store_num(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   const void             *num,
                   size_t                  size)
{
	kvs_assert(attr_id < UINT_MAX);
	kvs_assert(num);
	kvs_assert(size);

	int ret;

	db_recno_t id = (db_recno_t)attr_id + 1;
	DBT        key = { .data = &id, .size = sizeof(id), 0 };
	DBT        item = { .data = (void *)num, .size = size, 0 };

	ret = kvs_put(store, xact, &key, &item, 0);
	kvs_assert(ret != -EEXIST);

	return ret;
}

int
kvs_attr_clear(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               unsigned int            attr_id)
{
	kvs_assert(attr_id < UINT_MAX);

	int ret;

	db_recno_t id = (db_recno_t)attr_id + 1;
	DBT        key = { .data = &id, .size = sizeof(id), 0 };

	ret = kvs_del(store, xact, &key);
	kvs_assert(ret != -EXDEV);

	return ret;
}

int
kvs_attr_open(struct kvs_store       *store,
              const struct kvs_depot *depot,
              const struct kvs_xact  *xact,
              const char             *path,
              const char             *name,
              mode_t                  mode)
{
	return kvs_open_store(store, depot, xact, path, name, DB_RECNO, mode);
}

int
kvs_attr_close(const struct kvs_store *store)
{
	return kvs_close_store(store);
}

/******************************************************************************
 * String attribute handling
 ******************************************************************************/

ssize_t
kvs_attr_iter_load_str(const struct kvs_attr_iter_elem  *elem,
                       char                            **string)
{
	kvs_attr_iter_assert_elem(elem);

	int err;

	err = kvs_dup_str(string, elem->data, elem->size);
	if (err)
		return err;

	return elem->size;
}

ssize_t
kvs_attr_load_str(const struct kvs_store  *store,
                  const struct kvs_xact   *xact,
                  unsigned int             attr_id,
                  char                   **string)
{
	kvs_assert(attr_id < UINT_MAX);

	int ret;

	db_recno_t id = (db_recno_t)attr_id + 1;
	DBT        key = { .data = &id, .size = sizeof(id), 0 };
	DBT        item = { 0 };

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);
	if (ret)
		return ret;

	if (!item.data)
		return -ENODATA;

	ret = kvs_dup_str(string, item.data, item.size);
	if (ret)
		return ret;

	return item.size;
}

int
kvs_attr_store_str(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   const char             *str,
                   size_t                  len)
{
	kvs_assert(attr_id < UINT_MAX);
	kvs_assert(str);

	int ret;

	db_recno_t id = (db_recno_t)attr_id + 1;
	DBT        key = { .data = &id, .size = sizeof(id), 0 };
	DBT        item = { .data = len ? (char *)str : NULL, .size = len, 0 };

	if (len >= KVS_STR_MAX)
		return -ENAMETOOLONG;

	ret = kvs_put(store, xact, &key, &item, 0);
	kvs_assert(ret != -EEXIST);

	return ret;
}

#if defined(CONFIG_KVSTORE_TYPE_STRPILE)

int
kvs_attr_iter_load_strpile(const struct kvs_attr_iter_elem *elem,
                           struct upile                    *pile)
{
	kvs_attr_iter_assert_elem(elem);

	return kvs_deserialize_strpile(pile, elem->data, elem->size);
}

int
kvs_attr_load_strpile(const struct kvs_store  *store,
                      const struct kvs_xact   *xact,
                      unsigned int             attr_id,
                      struct upile            *pile)
{
	kvs_assert(attr_id < UINT_MAX);

	db_recno_t                   id = (db_recno_t)attr_id + 1;
	DBT                          key = { .data = &id,
	                                     .size = sizeof(id), 0 };
	DBT                          item = { 0 };
	int                          ret;

	ret = kvs_get(store, xact, &key, &item, 0);
	kvs_assert(ret != -EXDEV);

	if (ret < 0)
		return ret;

	return kvs_deserialize_strpile(pile, item.data, item.size);
}

int
kvs_attr_store_strpile(const struct kvs_store  *store,
                       const struct kvs_xact   *xact,
                       unsigned int             attr_id,
                       const struct upile      *pile)
{
	kvs_assert(attr_id < UINT_MAX);

	db_recno_t         id = (db_recno_t)attr_id + 1;
	DBT                key = { .data = &id, .size = sizeof(id), 0 };
	DBT                item = { 0, };
	int                ret;

	ret = kvs_serialize_strpile(pile, &item.data);
	if (ret < 0)
		return 0;

	item.size = ret;

	ret = kvs_put(store, xact, &key, &item, 0);
	kvs_assert(ret != -EEXIST);

	/* Release memory allocated by serialization process. */
	free(item.data);

	return ret;
}

#endif /* defined(CONFIG_KVSTORE_TYPE_STRPILE) */

/******************************************************************************
 * Boolean attribute handling
 ******************************************************************************/

int
kvs_attr_load_bool(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   bool                   *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(bool *)val;

	return 0;
}

int
kvs_attr_load_uint64(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint64_t               *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(uint64_t *)val;

	return 0;
}

int
kvs_attr_load_int64(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int64_t                *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(int64_t *)val;

	return 0;
}

int
kvs_attr_load_uint32(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint32_t               *value)
{
	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(uint32_t *)val;

	return 0;
}

int
kvs_attr_load_int32(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int32_t                *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(int32_t *)val;

	return 0;
}

int
kvs_attr_load_uint16(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint16_t               *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(uint16_t *)val;

	return 0;
}

int
kvs_attr_load_int16(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int16_t                *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(int16_t *)val;

	return 0;
}

int
kvs_attr_load_uint8(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    uint8_t                *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(uint8_t *)val;

	return 0;
}

int
kvs_attr_load_int8(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   int8_t                 *value)
{
	kvs_assert(value);

	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*value));
	if (!val)
		return -errno;

	*value = *(int8_t *)val;

	return 0;
}

#if defined(CONFIG_KVSTORE_TYPE_INADDR)

int
kvs_attr_iter_load_inaddr(const struct kvs_attr_iter_elem *elem,
                          struct in_addr                  *addr)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(addr);

	if (elem->size != sizeof(*addr))
		return -EMSGSIZE;

	*addr = *(struct in_addr *)elem->data;

	return 0;
}

int
kvs_attr_load_inaddr(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     struct in_addr         *addr)
{
	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*addr));
	if (!val)
		return -errno;

	*addr = *(struct in_addr *)val;

	return 0;
}

#endif /* defined(CONFIG_KVSTORE_TYPE_INADDR) */

#if defined(CONFIG_KVSTORE_TYPE_IN6ADDR)

int
kvs_attr_iter_load_in6addr(const struct kvs_attr_iter_elem *elem,
                           struct in6_addr                 *addr)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(addr);

	if (elem->size != sizeof(*addr))
		return -EMSGSIZE;

	*addr = *(struct in6_addr *)elem->data;

	return 0;
}

int
kvs_attr_load_in6addr(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      struct in6_addr        *addr)
{
	const void *val;

	val = kvs_attr_load_num(store, xact, attr_id, sizeof(*addr));
	if (!val)
		return -errno;

	*addr = *(struct in6_addr *)val;

	return 0;
}

#endif /* defined(CONFIG_KVSTORE_TYPE_IN6ADDR) */
