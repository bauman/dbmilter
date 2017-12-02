/*
 *  Copyright (c) 2006 Proofpoint, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 * $Id: example.c,v 8.5 2013-11-22 20:51:36 ca Exp $
 */

/*
**  A trivial example filter that logs all email to a file.
**  This milter also has some callbacks which it does not really use,
**  but they are defined to serve as an example.
*/

//milter-test-server -s inet:4242@localhost --mail-file=sample.eml
//milter-test-server -s inet:4242@127.0.0.1 -m sample.eml --end-of-message-macro=ii:1234
//for i in `seq 1 50001`; do milter-test-server -s inet:4242@localhost -m sample.eml ; done
//seq 1 90000 | xargs -P12 -n1  milter-test-server -s inet:4242@localhost -m sample.eml

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "libmilter/mfapi.h"
#include "libmilter/mfdef.h"
#include "dbmilter.h"
#include <bson.h>
#include "dbmilter_serializer.h"
#include "dbmilter_cclient.h"
#include "dbmilter_writer.h"
#include <bsoncompare.h>

mongoc_matcher_t *result_matcher = NULL;


struct mlfiPriv
{
    bson_oid_t oid;
    bson_t  envelope;
    bson_t  froms;
    uint32_t fromlen;
    bson_t  recipients;
    uint32_t tolen;
    bson_t  headers;
    uint32_t headerlen;
    bson_t  body;
    size_t total_body;
    uint32_t body_count;
    uint32_t state;
    char _STR_BUFFER[16];

};

#define MLFIPRIV	((struct mlfiPriv *) smfi_getpriv(ctx))

static unsigned long mta_caps = 0;

sfsistat
mlfi_cleanup(SMFICTX *ctx,
             bool ok)
{
    sfsistat rstat = SMFIS_CONTINUE;
    struct mlfiPriv *priv = MLFIPRIV;


    bson_destroy(&priv->froms);
    bson_destroy(&priv->recipients);
    bson_destroy(&priv->headers);
    bson_destroy(&priv->body);
    bson_destroy(&priv->envelope);
    bson_free(priv);
    smfi_setpriv(ctx, NULL);

    return rstat;
}

sfsistat
mlfi_connect(SMFICTX    *ctx,
             char       *hostname,
             _SOCK_ADDR *hostaddr)
{
    return SMFIS_CONTINUE;
}
sfsistat
mlfi_helo(SMFICTX *ctx,
          char *helohost) //state 0
{

    return SMFIS_CONTINUE;
}


sfsistat
mlfi_envfrom(SMFICTX *ctx,
             char **envfrom) //State 1
{
    struct mlfiPriv *priv;
    /* allocate some private memory */
    priv = bson_malloc0(sizeof *priv);
    smfi_setpriv(ctx, priv);

    priv = MLFIPRIV;

    if (priv->state < 1){
        priv->state = 1;
        bson_oid_init (&priv->oid, NULL);
        bson_init(&priv->envelope);
        bson_init(&priv->froms);
        bson_init(&priv->recipients);
        bson_init(&priv->headers);
        bson_init(&priv->body);
    }

    const char * key ;
    size_t st = bson_uint32_to_string (priv->fromlen, &key, priv->_STR_BUFFER, sizeof priv->_STR_BUFFER);
    bson_append_utf8(&priv->froms, key, st, *envfrom, -1);
    priv->fromlen++;
    /* continue processing */
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_envrcpt(SMFICTX *ctx,
             char **argv)//State 2
{

    struct mlfiPriv *priv = MLFIPRIV;

    const char * key ;
    size_t st = bson_uint32_to_string (priv->tolen, &key, priv->_STR_BUFFER, sizeof priv->_STR_BUFFER);
    bson_append_utf8(&priv->recipients, key, st, *argv, -1);
    priv->tolen++;
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_header(SMFICTX *ctx,
            char *headerf,
            char *headerv) //state 3
{
    /* write the header to the log file */

    struct mlfiPriv *priv = MLFIPRIV;

    bson_t headerline;
    bson_init(&headerline);
    bson_append_utf8(&headerline, headerf, -1, headerv, -1);

    const char * key ;
    size_t st = bson_uint32_to_string (priv->headerlen, &key, priv->_STR_BUFFER, sizeof priv->_STR_BUFFER);
    bson_append_document(&priv->headers, key, st, &headerline);
    priv->headerlen++;
    bson_destroy(&headerline);
    /* continue processing */
    return SMFIS_CONTINUE;

}

sfsistat
mlfi_eoh(SMFICTX *ctx)//State 4
{
    //struct mlfiPriv *priv = MLFIPRIV;
    /* continue processing */
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_body(SMFICTX *ctx,
          u_char *bodyp,
          size_t bodylen)//State 5
{
    struct mlfiPriv *priv = MLFIPRIV;
    const char * key ;
    size_t st = bson_uint32_to_string (priv->body_count, &key, priv->_STR_BUFFER, sizeof priv->_STR_BUFFER);
    bson_append_binary(&priv->body,key, st, BSON_SUBTYPE_BINARY, bodyp, bodylen);
    priv->body_count++;
    priv->total_body += bodylen;
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_eom(SMFICTX *ctx) //State 6
{
    struct mlfiPriv *priv = MLFIPRIV;
    int disposition = DBMILTER_TEMPFAIL;
    //size_t *s;
    //char * as_json;
    //as_json = bson_as_json(&priv->body, NULL);
    //printf("count: %d\nsize:%d\n%s\n", priv->body_count, priv->total_body, as_json);

    bson_append_array(&priv->envelope, "from", 4, &priv->froms);
    bson_append_array(&priv->envelope, "to", 2, &priv->recipients);
    bson_append_array(&priv->envelope, "headers", 7, &priv->headers);
    bson_append_array(&priv->envelope, "body", 4, &priv->body);
    char * i = smfi_getsymval(ctx, "i") ;
    bson_append_utf8(&priv->envelope, "mid", 3, i, -1);

    //as_json = bson_as_json(&priv->envelope, NULL);
    //printf("%s\n", as_json);
    //bson_free(as_json);

    bson_t result;
    bson_init(&result);
    int serialized = dbm_serialize(&priv->oid, &priv->envelope, &result);
    if (serialized == 0){
        if (output_path) {
            dbmilter_write_file(&priv->oid, &result);
        }
        if (cserver){
            disposition = dbmilter_send(&result);
        } else {
            disposition = DBMILTER_ALLOW;
        }
    }
    bson_destroy(&result);
    mlfi_cleanup(ctx, true);
    return disposition;
}

sfsistat
mlfi_close(SMFICTX *ctx)
{
    return SMFIS_ACCEPT;
}

sfsistat
mlfi_abort(SMFICTX *ctx)
{
    return mlfi_cleanup(ctx, false);
}

sfsistat
mlfi_unknown(SMFICTX *ctx,
             char *cmd)
{
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_data(SMFICTX *ctx)
{
    return SMFIS_CONTINUE;
}

sfsistat
mlfi_negotiate(SMFICTX *ctx,
               unsigned long f0,
               unsigned long f1,
               unsigned long f2,
               unsigned long f3,
               unsigned long *pf0,
               unsigned long *pf1,
               unsigned long *pf2,
               unsigned long *pf3)

{
    /* milter actions: add headers */\
    *pf0 = SMFIF_ADDHDRS|SMFIF_QUARANTINE;

    /* milter protocol steps: all but connect, HELO, RCPT */
    *pf1 = 0;
    mta_caps = f1;
    if ((mta_caps & SMFIP_NR_HDR) != 0)
        *pf1 |= SMFIP_NR_HDR;
    *pf2 = 0;
    *pf3 = 0;
    return SMFIS_CONTINUE;
}

struct smfiDesc smfilter =
        {
                "SampleFilter",	/* filter name */
                SMFI_VERSION,	/* version code -- do not change */
                SMFIF_ADDHDRS|SMFIF_QUARANTINE,	/* flags */
                mlfi_connect,		/* connection info filter */
                mlfi_helo,		/* SMTP HELO command filter */
                mlfi_envfrom,	/* envelope sender filter */
                mlfi_envrcpt,		/* envelope recipient filter */
                mlfi_header,	/* header filter */
                mlfi_eoh,	/* end of header */
                mlfi_body,	/* body block filter */
                mlfi_eom,	/* end of message */
                mlfi_abort,	/* message aborted */
                mlfi_close,	/* connection cleanup */
                NULL,	/* unknown/unimplemented SMTP commands */
                mlfi_data,	/* DATA command filter */
                mlfi_negotiate	/* option negotiation at connection startup */
        };


void
print_help(char *argv[]){
    fprintf(stderr, "Usage \n\t%s: -p <port> -o <output folder> -s <scanner address (if desired)> -d <block JSON> \n", argv[0]);
}
int
main(int argc,
     char *argv[])

{
    //const char * jsonspec = "{\"signatures.name\":\"DENY\"}";
    //bson_t      *buf_spec = NULL;
    //buf_spec = bson_new_from_json ((const uint8_t*)jsonspec, -1, NULL);
    result_matcher = NULL;
    bool setconn;
    bool conn_available = false;
    bool disposition_spec = false;
    int c;
    int return_code = 0;




    setconn = false;

    /* Process command line options */
    while ((c = getopt(argc, argv, "o:s:p:d:")) != -1)
    {
        switch (c)
        {
            case 'p':
                if (optarg == NULL || *optarg == '\0')
                {
                    (void) fprintf(stderr, "Illegal conn: %s\n",
                                   optarg);
                    exit(EX_USAGE);
                }
                (void) smfi_setconn(optarg);
                setconn = true;
                break;
            case 'd':
                if (optarg == NULL || *optarg == '\0')
                {
                    (void) fprintf(stderr, "%s", "Illegal disposition spec\n");
                    exit(EX_USAGE);
                }
                bson_t      *buf_spec = NULL;
                buf_spec = bson_new_from_json ((const uint8_t*)optarg, -1, NULL);
                if (buf_spec){
                    result_matcher = mongoc_matcher_new (buf_spec, NULL);
                    doc_destroy(buf_spec);
                    if (result_matcher){
                        disposition_spec = true;
                    }
                }
                break;
            case 's':
                if (optarg == NULL || *optarg == '\0')
                {
                    (void) fprintf(stderr, "%s", "Illegal server spec\n");
                    exit(EX_USAGE);
                }
                cserver = bson_strdup(optarg);
                break;
            case 'o':
                if (optarg == NULL || *optarg == '\0')
                {
                    (void) fprintf(stderr, "%s   %s", "Illegal file path spec\n", optarg);
                    exit(EX_USAGE);
                }
                output_path = bson_strdup(optarg);
                break;
        }
    }

    if (cserver){
        conn_available = dbmilter_cclient_ping();
    }


    if (!setconn)
    {
        fprintf(stderr, "%s: Missing required -p argument\n", argv[0]);
        print_help(argv);
        exit(EX_USAGE);
    }
    if (!disposition_spec)
    {
        fprintf(stderr, "%s: Missing required -d argument\n", argv[0]);
        print_help(argv);
        exit(EX_USAGE);
    }
    if (smfi_register(smfilter) == MI_FAILURE)
    {
        fprintf(stderr, "smfi_register failed\n");
        exit(EX_UNAVAILABLE);
    }
    if (cserver && conn_available)
    {
        fprintf(stderr, "Entering Loop...\n");
        return_code = smfi_main();
    } else if (cserver && !conn_available){
        fprintf(stderr, "Scanner selected is not available\n");
        return_code = DBMILTER_NOSCANNER;
    }
    usleep(1);
    fprintf(stderr, "Closing Milter\n");
    usleep(1);
    smfi_stop();
    matcher_destroy(result_matcher);
    bson_free(cserver);
    return return_code;
}

