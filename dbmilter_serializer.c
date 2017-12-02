#include "dbmilter.h"
#include "dbmilter_cclient.h"
#include <bson.h>


static
bool
dbm_simulate_tree(bson_oid_t *oid, bson_t * result, bson_t *envelope, bson_t *co, bson_t *ct){

    bson_append_oid(co, "_id", 3, oid);
    char * oidstr = bson_malloc0(25); //TODO: CHECK NOT NULL

    bson_oid_to_string(oid, oidstr);

    bson_append_utf8(co, "filename", 8, oidstr, 24);
    bson_append_int32(co, "hash", 4, 0);
    bson_append_int32(co, "size", 4, 0);

    bson_t cid;
    bson_init(&cid);
    bson_append_array_begin(co, "client_id", 9, &cid);;
    bson_append_array_end(co, &cid);


    bson_t traceup;
    bson_init(&traceup);
    bson_append_array_begin(co, "trace_up", 8, &traceup);
    bson_append_oid(&traceup, "_id", 3, oid);
    bson_append_array_end(co, &traceup);


    bson_t tracedn;
    bson_init(&tracedn);
    bson_append_array_begin(co, "trace_down", 10, &tracedn);
    bson_append_array_end(co, &tracedn);

    bson_t tokens;
    bson_init(&tokens);
    bson_append_document_begin(co, "tokens", 6, &tokens);
    bson_append_document_end(co, &tokens);

    bson_t sigs;
    bson_init(&sigs);
    bson_append_array_begin(co, "signatures", 10, &sigs);
    bson_append_array_end(co, &sigs);

    bson_t actions;
    bson_init(&actions);
    bson_append_array_begin(co, "actions", 7, &actions);


        bson_t actions_item;
        bson_init(&actions_item);
        bson_append_utf8(&actions_item, "action", 6, "reconstructmilter", 17);
        bson_append_int32(&actions_item, "priority", 8, 2);
        bson_append_null(&actions_item, "args", 4);
        bson_append_bool(&actions_item, "complete", 8, false);
        bson_oid_t aid;
        bson_oid_init (&aid, NULL);
        bson_append_oid(&actions_item, "action_id", 9, &aid);

    bson_append_document(&actions, "0", 1, &actions_item);

    bson_append_array_end(co, &actions);

    bson_append_document(co, "envelope", 8, envelope);

    bson_append_document(ct, oidstr, 24, co);
    bson_append_document(result, "cobra_tree", 10, ct);


    bson_destroy(&actions_item);
    bson_free(oidstr);
    return true;
}


int
dbm_serialize(bson_oid_t *oid, bson_t *envelope, bson_t* result){
    int disposition = DBMILTER_ALLOW;
    bson_append_oid(result, "_id", 3, oid);

    bson_t all_actions;
    bson_init(&all_actions);
    bson_append_array_begin(result, "all_actions", 11, &all_actions);
    bson_append_array_end(result, &all_actions);

    bson_append_int32(result, "inspection_order", 16, 1);
    bson_append_bool(result, "remove_binary", 13, true);
    bson_append_int32(result, "return_data", 11, 1);
    bson_append_int32(result, "timeout", 7, 30);


    bson_t co, ct;
    bson_init(&co);
    bson_init(&ct);

    dbm_simulate_tree(oid, result, envelope, &co, &ct);


    /*
     *
     *
    size_t *s;
    char * as_json;
    as_json = bson_as_json(result, NULL);
    printf("%s\n", as_json);
    bson_free(as_json);

    */

    bson_destroy(&co);
    bson_destroy(&ct);
    bson_destroy(&all_actions);

    return disposition;
}
