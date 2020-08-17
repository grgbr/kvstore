#include <kvstore/store.h>
#include <kvstore/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, const char * const argv[])
{
	char             *err_pfx;
	char             *info_pfx;
	const char       *argv0 = basename(argv[0]);
	struct kvs_depot  depot;
	struct kvs_xact   xact;
	struct kvs_file   file;
	int               err;
	int               ret = EXIT_FAILURE;

	if (asprintf(&err_pfx, "%s: " KVS_VERB_ERR_PREFIX, argv0) < 0)
		return EXIT_FAILURE;
	if (asprintf(&info_pfx, "%s: " KVS_VERB_INFO_PREFIX, argv0) < 0)
		goto free_err;
	kvs_enable_verb(KVS_VERB_OUT, err_pfx, info_pfx);

	err = kvs_open_depot(&depot,
	                     "testdb",
	                     512 << 10,
	                     0,
	                     S_IRWXU);
	if (err) {
		fprintf(stderr,
		        "failed to open depot: %s (%d).\n",
		        strerror(-err),
		        -err);
		goto free_info;
	}

	err = kvs_file_init(&file, &depot);
	if (err) {
		fprintf(stderr,
		        "failed to init file: %s (%d).\n",
		        strerror(-err),
		        -err);
		goto close_depot;
	}

	err = kvs_begin_xact(&depot, NULL, &xact, 0);
	if (err) {
		fprintf(stderr,
		        "failed to start transaction: %s (%d).\n",
		        strerror(-err),
		        -err);
		goto fini_file;
	}

#if 1
	kvs_file_printf(&file, "deadbeef\n");

	err = kvs_file_realize(&file, &xact, "atom", S_IRUSR | S_IWUSR);
	if (err)
		fprintf(stderr,
		        "failed to create atom file: %s (%d).\n",
		        strerror(-err),
		        -err);
#endif

#if 0
	{
		int *fault = NULL;
		*fault = 2;
	}
#endif


#if 1
	if (!err) {
		err = kvs_commit_xact(&xact);
		if (err) {
			fprintf(stderr,
			        "failed to commit transaction: %s (%d).\n",
			        strerror(-err),
			        -err);
			goto close_depot;
		}
	}
	else {
#endif
		err = kvs_rollback_xact(&xact);
		if (err)
			fprintf(stderr,
			        "failed to abort transaction: %s (%d).\n",
			        strerror(-err),
			        -err);
		goto close_depot;
#if 1
	}
#endif

	ret = EXIT_SUCCESS;

fini_file:
	kvs_file_fini(&file);

close_depot:
	err = kvs_close_depot(&depot);
	if (err)
		fprintf(stderr,
		        "failed to close depot: %s (%d).\n",
		        strerror(-err),
		        -err);

free_info:
	free(info_pfx);

free_err:
	free(err_pfx);

	return ret;
}
