#ifndef _KVS_FILE_H
#define _KVS_FILE_H

#include <db.h>
#include <utils/cdefs.h>

struct kvs_depot;
struct kvs_xact;

struct kvs_file {
	unsigned int  off;
	size_t        size;
	char         *data;
	DB_ENV       *env;
	int           dir;
};

extern int __printf(2, 3)
kvs_file_printf(struct kvs_file *file, const char *format, ...);

extern int
kvs_file_write(struct kvs_file *file, const void *data, size_t size);

extern int
kvs_file_realize(struct kvs_file        *file,
                 const struct kvs_xact  *xact,
                 const char             *path,
                 mode_t                  mode);

extern int
kvs_file_init(struct kvs_file *file, const struct kvs_depot *depot);

extern void
kvs_file_fini(const struct kvs_file *file);

#endif /* _KVS_FILE_H */
