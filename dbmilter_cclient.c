

//  Lazy Pirate client
//  Use zmq_poll to do a safe request-reply
//  To run, start lpserver and then randomly kill/restart it

#include "czmq.h"
#include "bson.h"
#include "dbmilter_cclient.h"
#include <bsoncompare.h>
#include "dbmilter.h"


char * cserver = SERVER_ENDPOINT;

bool dbmilter_cclient_ping()
{
    int success = false;
    zctx_t *ctx = zctx_new ();
    printf ("I: connecting to server…\n");
    void *client = zsocket_new (ctx, ZMQ_REQ);
    assert (client);
    zsocket_connect (client, cserver);
    int retries_left = REQUEST_RETRIES;
    while (retries_left && !zctx_interrupted) {
        //  We send a request, then we work to get a reply
        char request [5];
        sprintf (request, "%s", "ping");
        zstr_send (client, request);
        int expect_reply = 1;
        while (expect_reply) {
            //  Poll socket for a reply, with timeout
            zmq_pollitem_t items [] = { { client, 0, ZMQ_POLLIN, 0 } };
            int rc = zmq_poll (items, 1, REQUEST_TIMEOUT * ZMQ_POLL_MSEC);
            if (rc == 0)
            {
                expect_reply = 0;
                retries_left = 0;
            } else if (rc > 0){
                if (items [0].revents & ZMQ_POLLIN) {
                    char *reply = zstr_recv (client);
                    printf ("I: server replied OK (%s)\n", reply);
                    success = true;
                    retries_left = 0;
                    expect_reply = 0;
                    free (reply);
                }  else {
                    printf ("E: no response from server\n");
                    expect_reply = 0;
                    retries_left = 0;
                }
            }
        }
    }
    zsocket_destroy(ctx, client);
    zctx_destroy (&ctx);
    return success;
}

int dbmilter_send (bson_t *result)
{
    int disposition = DBMILTER_TEMPFAIL; //allow
    char * as_json;
    as_json = bson_as_json(result, NULL);



    zctx_t *ctx = zctx_new ();
    //printf ("I: connecting to server…\n");
    void *client = zsocket_new (ctx, ZMQ_REQ);
    assert (client);
    zsocket_connect (client, cserver);

    int sequence = 0;
    int retries_left = REQUEST_RETRIES;
    while (retries_left && !zctx_interrupted) {
        //  We send a request, then we work to get a reply
        char request [10];
        sprintf (request, "%d", ++sequence);
        zstr_send (client, as_json);

        int expect_reply = 1;
        while (expect_reply) {
            //  Poll socket for a reply, with timeout
            zmq_pollitem_t items [] = { { client, 0, ZMQ_POLLIN, 0 } };
            int rc = zmq_poll (items, 1, REQUEST_TIMEOUT * ZMQ_POLL_MSEC);


            //  Here we process a server reply and exit our loop if the
            //  reply is valid. If we didn't a reply we close the client
            //  socket and resend the request. We try a number of times
            //  before finally abandoning:
            if (rc > 0){
                if (items [0].revents & ZMQ_POLLIN) {
                    //  We got a reply from the server, must match sequence
                    char *reply = zstr_recv (client);

                    retries_left = 0;
                    expect_reply = 0;
                    if (reply)
                    {
                        bson_t *doc = bson_new_from_json((const uint8_t*)reply, strlen(reply), NULL);
                        bool result = mongoc_matcher_match(result_matcher, doc);
                        doc_destroy(doc);
                        //printf ("I: server replied OK (%s)\n", reply);
                        if (result){
                            disposition = DBMILTER_BLOCK;
                            printf ("I: THIS SHOULD BE BLOCKED ()\n");
                        } else {
                            disposition = DBMILTER_ALLOW;
                        }
                        free (reply);
                    }
                }  else {
                    printf ("W: no response from server\n");
                    expect_reply = 0;
                    retries_left = 0;
                }
            } else if (rc < 0) {
                printf ("W: error from server\n");
            }

        }
    }
    bson_free(as_json);

    zsocket_destroy(ctx, client);
    zctx_destroy (&ctx);
    return disposition;
}
