
#ifndef DBMILTER_CCLIENT
#define DBMILTER_CCLIENT

#include "bson.h"

#define DBMILTER_ALLOW 0
#define DBMILTER_BLOCK 2 //ALSO SMFIS_DISCARD
#define DBMILTER_TEMPFAIL 4

#define DBMILTER_NOSCANNER -7


#define REQUEST_TIMEOUT     5000//60000    //  msecs, (> 1000!)
#define REQUEST_RETRIES     1       //  Before we abandon
#define SERVER_ENDPOINT     "tcp://127.0.0.1:8585"


int dbmilter_send (bson_t *result);
bool dbmilter_cclient_ping();


extern char * cserver;

#endif /*DBMILTER_CCLIENT*/
