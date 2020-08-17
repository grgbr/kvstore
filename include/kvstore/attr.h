#ifndef _KVS_ATTR_H
#define _KVS_ATTR_H

#include <kvstore/store.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

/******************************************************************************
 * Attribute store / iterator handling
 ******************************************************************************/

struct kvs_attr_iter_elem {
	unsigned int  id;
	size_t        size;
	const void   *data;
};

#define kvs_attr_iter_assert_elem(_elem) \
	kvs_assert((_elem)); \
	kvs_assert((_elem)->id < UINT_MAX); \
	kvs_assert((_elem)->size); \
	kvs_assert((_elem)->data)

static inline unsigned int
kvs_attr_iter_id(const struct kvs_attr_iter_elem *elem)
{
	kvs_attr_iter_assert_elem(elem);

	return elem->id;
}

extern int
kvs_attr_iter_first(const struct kvs_iter     *iter,
                    struct kvs_attr_iter_elem *elem);

extern int
kvs_attr_iter_next(const struct kvs_iter     *iter,
                   struct kvs_attr_iter_elem *elem);

extern int
kvs_attr_init_iter(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   struct kvs_iter        *iter);

extern int
kvs_attr_fini_iter(const struct kvs_iter *iter);

extern int
kvs_attr_store_num(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   const void             *num,
                   size_t                  size);

extern int
kvs_attr_clear(const struct kvs_store *store,
               const struct kvs_xact  *xact,
               unsigned int            attr_id);

extern int
kvs_attr_open(struct kvs_store       *store,
              const struct kvs_depot *depot,
              const struct kvs_xact  *xact,
              const char             *path,
              const char             *name,
              mode_t                  mode);

extern int
kvs_attr_close(const struct kvs_store *store);

/******************************************************************************
 * String attribute handling
 ******************************************************************************/

extern ssize_t
kvs_attr_iter_load_str(const struct kvs_attr_iter_elem  *elem,
                       char                            **string);

extern ssize_t
kvs_attr_load_str(const struct kvs_store  *store,
                  const struct kvs_xact   *xact,
                  unsigned int             attr_id,
                  char                   **string);

extern int
kvs_attr_store_str(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   const char             *str,
                   size_t                  len);

#if defined(CONFIG_KVSTORE_TYPE_STRPILE)

struct upile;

extern int
kvs_attr_iter_load_strpile(const struct kvs_attr_iter_elem *elem,
                           struct upile                    *pile);

extern int
kvs_attr_load_strpile(const struct kvs_store  *store,
                      const struct kvs_xact   *xact,
                      unsigned int             attr_id,
                      struct upile            *pile);

extern int
kvs_attr_store_strpile(const struct kvs_store  *store,
                       const struct kvs_xact   *xact,
                       unsigned int             attr_id,
                       const struct upile      *pile);

#endif /* defined(CONFIG_KVSTORE_TYPE_STRPILE) */

/******************************************************************************
 * Boolean attribute handling
 ******************************************************************************/

static inline int
kvs_attr_iter_load_bool(const struct kvs_attr_iter_elem *elem, bool *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(bool *)elem->data;

	return 0;
}

extern int
kvs_attr_load_bool(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   bool                   *value);

static inline int
kvs_attr_store_bool(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    bool                    value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

/******************************************************************************
 * 64 bits integer attribute handling
 ******************************************************************************/

static inline int
kvs_attr_iter_load_uint64(const struct kvs_attr_iter_elem *elem,
                          uint64_t                        *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(uint64_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_uint64(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint64_t               *value);

static inline int
kvs_attr_store_uint64(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      uint64_t                value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

static inline int
kvs_attr_iter_load_int64(const struct kvs_attr_iter_elem *elem,
                         int64_t                         *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(int64_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_int64(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int64_t                *value);

static inline int
kvs_attr_store_int64(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     int64_t                 value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

/******************************************************************************
 * 32 bits integer attribute handling
 ******************************************************************************/

static inline int
kvs_attr_iter_load_uint32(const struct kvs_attr_iter_elem *elem,
                          uint32_t                        *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(uint32_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_uint32(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint32_t               *value);

static inline int
kvs_attr_store_uint32(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      uint32_t                value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

static inline int
kvs_attr_iter_load_int32(const struct kvs_attr_iter_elem *elem,
                         int32_t                         *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(int32_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_int32(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int32_t                *value);

static inline int
kvs_attr_store_int32(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     int32_t                 value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

/******************************************************************************
 * 16 bits integer attribute handling
 ******************************************************************************/

static inline int
kvs_attr_iter_load_uint16(const struct kvs_attr_iter_elem *elem,
                          uint16_t                        *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(uint16_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_uint16(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint16_t               *value);

static inline int
kvs_attr_store_uint16(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      uint16_t                value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

static inline int
kvs_attr_iter_load_int16(const struct kvs_attr_iter_elem *elem,
                         int16_t                         *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(int16_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_int16(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int16_t                *value);

static inline int
kvs_attr_store_int16(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int16_t                 value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

/******************************************************************************
 * 8 bits integer attribute handling
 ******************************************************************************/

static inline int
kvs_attr_iter_load_uint8(const struct kvs_attr_iter_elem *elem,
                         uint8_t                         *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(uint8_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_uint8(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    uint8_t                *value);

static inline int
kvs_attr_store_uint8(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     uint8_t                 value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

static inline int
kvs_attr_iter_load_int8(const struct kvs_attr_iter_elem *elem,
                        int8_t                          *value)
{
	kvs_attr_iter_assert_elem(elem);
	kvs_assert(value);

	if (elem->size != sizeof(*value))
		return -EMSGSIZE;

	*value = *(int8_t *)elem->data;

	return 0;
}

extern int
kvs_attr_load_int8(const struct kvs_store *store,
                   const struct kvs_xact  *xact,
                   unsigned int            attr_id,
                   int8_t                 *value);

static inline int
kvs_attr_store_int8(const struct kvs_store *store,
                    const struct kvs_xact  *xact,
                    unsigned int            attr_id,
                    int8_t                  value)
{
	return kvs_attr_store_num(store, xact, attr_id, &value, sizeof(value));
}

/******************************************************************************
 * IPv4/IPv6 (socket) address attribute handling
 ******************************************************************************/

#if defined(CONFIG_KVSTORE_TYPE_INADDR)

#include <netinet/in.h>

extern int
kvs_attr_iter_load_inaddr(const struct kvs_attr_iter_elem *elem,
                          struct in_addr                  *addr);

extern int
kvs_attr_load_inaddr(const struct kvs_store *store,
                     const struct kvs_xact  *xact,
                     unsigned int            attr_id,
                     struct in_addr         *value);

static inline int
kvs_attr_store_inaddr(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      const struct in_addr   *value)
{
	return kvs_attr_store_num(store, xact, attr_id, value, sizeof(*value));
}

#endif /* defined(CONFIG_KVSTORE_TYPE_INADDR) */

#if defined(CONFIG_KVSTORE_TYPE_IN6ADDR)

#include <netinet/in.h>

extern int
kvs_attr_iter_load_in6addr(const struct kvs_attr_iter_elem *elem,
                           struct in6_addr                 *addr);

extern int
kvs_attr_load_in6addr(const struct kvs_store *store,
                      const struct kvs_xact  *xact,
                      unsigned int            attr_id,
                      struct in6_addr        *value);

static inline int
kvs_attr_store_in6addr(const struct kvs_store *store,
                       const struct kvs_xact  *xact,
                       unsigned int            attr_id,
                       const struct in6_addr  *value)
{
	return kvs_attr_store_num(store, xact, attr_id, value, sizeof(*value));
}

#endif /* defined(CONFIG_KVSTORE_TYPE_IN6ADDR) */

#endif /* _KVS_ATTR_H */
