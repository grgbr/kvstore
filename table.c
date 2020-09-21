#include <kvstore/table.h>
#include <errno.h>

int
kvs_table_open(struct kvs_table       *table,
               const struct kvs_depot *depot,
               const struct kvs_xact  *xact,
               mode_t                  mode)
{
	kvs_table_assert(table);

	const struct kvs_table_desc *desc = table->desc;
	unsigned int                 cnt = 0;
	int                          ret;

	ret = desc->data_ops.open(table, depot, xact, mode);

	while (!ret && (cnt < desc->indx_nr)) {
		ret = desc->indx_ops[cnt].open(table, depot, xact, mode);
		cnt++;
	}

	table->indx_cnt = cnt;

	return ret;
}

int
kvs_table_close(const struct kvs_table *table)
{
	kvs_table_assert(table);
	kvs_assert(table->indx_cnt <= table->desc->indx_nr);

	unsigned int                 cnt = table->indx_cnt;
	const struct kvs_table_desc *desc = table->desc;
	int                          ret = 0;

	while (!ret && cnt) {
		cnt--;
		ret = desc->indx_ops[cnt].close(table);
	}

	while (cnt) {
		cnt--;
		desc->indx_ops[cnt].close(table);
	}

	if (!ret)
		ret = desc->data_ops.close(table);
	else
		desc->data_ops.close(table);

	return ret;
}

int
kvs_table_init(struct kvs_table *table, const struct kvs_table_desc *desc)
{
	kvs_assert(table);
	kvs_table_assert_desc(desc);

	if (desc->indx_nr) {
		table->indices = malloc(desc->indx_nr *
		                        sizeof(table->indices[0]));
		if (!table->indices)
			return -errno;
	}

	table->desc = desc;

	return 0;
}
