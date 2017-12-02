#ifndef DBMILTER_WRITER
#define DBMILTER_WRITER

#include <bson.h>


extern char * output_path;


bool
dbmilter_write_file(bson_oid_t * oid,
                    bson_t * result);

#endif /* DBMILTER_WRITER */