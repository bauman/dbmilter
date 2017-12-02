//
// Created by dan on 5/16/16.
//

#ifndef DBMILTER_H
#define DBMILTER_H


#include <mongoc-matcher.h>
#include "libmilter/mfapi.h"
#include "libmilter/mfdef.h"



/*
sfsistat
mlfi_connect(SMFICTX    *ctx,
             char       *hostname,
             _SOCK_ADDR *hostaddr);

sfsistat
mlfi_helo(SMFICTX *ctx,
          char *helohost);

sfsistat
mlfi_envfrom(SMFICTX *ctx,
             char **envfrom);

sfsistat
mlfi_envrcpt(SMFICTX *ctx,
             char **argv);

sfsistat
mlfi_header(SMFICTX *ctx,
            char *headerf,
            char *headerv);

sfsistat
mlfi_eoh(SMFICTX *ctx);


sfsistat
mlfi_body(SMFICTX *ctx,
          u_char *bodyp,
          size_t bodylen);



sfsistat
mlfi_eom(SMFICTX *ctx);

sfsistat
mlfi_close(SMFICTX *ctx);

sfsistat
mlfi_abort(SMFICTX *ctx);


sfsistat
mlfi_unknown(SMFICTX *ctx,
             char *cmd);

sfsistat
mlfi_data(SMFICTX *ctx);

sfsistat
mlfi_negotiate(SMFICTX *ctx,
               unsigned long f0,
               unsigned long f1,
               unsigned long f2,
               unsigned long f3,
               unsigned long *pf0,
               unsigned long *pf1,
               unsigned long *pf2,
               unsigned long *pf3);


*/
extern mongoc_matcher_t * result_matcher;

#endif //DBMILTER_H
