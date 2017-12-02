#include "dbmilter_writer.h"
#include <stdio.h>
#include <bson.h>


#define DBMILTER_OID_SIZE 25

char * output_path = NULL;

bool
dbmilter_write_file(bson_oid_t * oid,
                    bson_t * result)
{
    bool success = false;
    FILE *ofp;
    char outputFilename[DBMILTER_OID_SIZE];
    bson_oid_to_string(oid, outputFilename);
    char * full_file_name;
    size_t fss = strlen(output_path);
    full_file_name = (char *)bson_malloc0(strlen(output_path)+DBMILTER_OID_SIZE);
    strncat(full_file_name, output_path, fss);
    strncat(full_file_name, outputFilename, DBMILTER_OID_SIZE);

    ofp = fopen(full_file_name, "w");

    if (ofp == NULL) {
        fprintf(stderr, "Can't open output file %s!\n",
                outputFilename);
    } else {
        const u_int8_t * bsonstr = bson_get_data(result);
        size_t s = fwrite(bsonstr, 1, result->len, ofp);
        if (s == (size_t) result->len){
            success = true;
        } else {
            fprintf(stderr, "failed writing %d bytes to %s\n", result->len, full_file_name);
        }
    }
    bson_free(full_file_name);
    fflush(ofp);
    fclose(ofp);
    return success;
}

