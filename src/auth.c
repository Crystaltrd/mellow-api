#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge() */
#include <err.h> /* err(), warnx() */
#include <inttypes.h>
#include <stdlib.h> /* EXIT_FAILURE */
#include <string.h> /* memset() */
#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#ifndef __BSD_VISIBLE
#define	_PASSWORD_LEN		128

int
crypt_checkpass(const char *password, const char *hash);

int
crypt_newhash(const char *password, const char *pref, char *hash,
              size_t hashsize);

#endif
#define SESSIONID_MAX 99999999
struct kreq r;
struct kjsonreq req;

enum statement {
    STMTS_CHECK,
    STMTS_ADD,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {(char *) "SELECT pwhash FROM ACCOUNT WHERE UUID=(?) LIMIT 1"},
    {(char *) "INSERT INTO SESSIONS VALUES((?),(?),datetime('now',(?),'localtime'))"}
};

/*
 * Array of sources(databases) and their access mode, we only have one which is our central database.
 * And since this microservice handles authentification (writting into the SESSIONS) table, we open it
 * as Read-Write
 */
struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx;
struct sqlbox_cfg cfg;
size_t dbid; // Database associated with a config and a context

void alloc_ctx_cfg() {
    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = STMTS__MAX;
    cfg.stmts.stmts = pstmts;
    if ((boxctx = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(boxctx, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}

enum key {
    KEY_UUID,
    KEY_PASSWD,
    KEY_REMEMBER,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "UUID"},
    {kvalid_stringne, "passwd"},
    {NULL, "remember"},
};


enum khttp sanitize() {
    if (r.method != KMETHOD_POST)
        return KHTTP_405;
    if (!r.fieldmap[KEY_UUID] || !r.fieldmap[KEY_PASSWD])
        return KHTTP_406;
    return KHTTP_200;
}


int check_passwd() {
    size_t stmtid;
    size_t parmsz = 1;
    char hash[_PASSWORD_LEN];
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        }
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx, dbid, STMTS_CHECK, parmsz, parms, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx, stmtid)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    if (res->psz == 0) {
        sqlbox_finalise(boxctx, stmtid);
        sqlbox_close(boxctx, dbid);
        return EXIT_FAILURE;
    }
    strncpy(hash, res->ps[0].sparm,_PASSWORD_LEN);
    sqlbox_finalise(boxctx, stmtid);
    if (crypt_checkpass(r.fieldmap[KEY_PASSWD]->parsed.s, hash) == 0)
        return EXIT_SUCCESS;

    return EXIT_FAILURE;
}

void open_session() {
    size_t parmsz = 3;
    char buf[32];
    const uint32_t sessionID = arc4random_uniform(SESSIONID_MAX);
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        },
        {
            .type = SQLBOX_PARM_INT,
            .iparm = sessionID
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = (r.fieldmap[KEY_REMEMBER]) ? "+7 days" : "+3 hours"
        }
    };
    if (sqlbox_exec(boxctx, dbid, STMTS_ADD, parmsz, parms,SQLBOX_STMT_CONSTRAINT) != SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");

    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_head(&r, kresps[KRESP_SET_COOKIE],
               "sessionID=%d; Path=/; expires=%s", sessionID,
               khttp_epoch2str(time(NULL) + ((r.fieldmap[KEY_REMEMBER]) ? 7 * 24 * 60 * 60 : 60 * 60 * 3), buf, sizeof(buf)));
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putboolp(&req, "authorized",true);
    kjson_putintstrp(&req, "sessionid", sessionID);
    kjson_obj_close(&req);
    khttp_free(&r);
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, NULL, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    alloc_ctx_cfg();
    if (check_passwd() == EXIT_FAILURE) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putboolp(&req, "authorized",false);
        kjson_putstringp(&req, "error", "The Username or Password that you provided are wrong");
        kjson_obj_close(&req);
        khttp_free(&r);
        return 0;
    }
    open_session();
    sqlbox_close(boxctx, dbid);
    sqlbox_free(boxctx);
    return EXIT_SUCCESS;
}
