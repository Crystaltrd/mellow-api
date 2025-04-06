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
struct kreq r;
struct kjsonreq req;

enum key {
    KEY_UUID,
    KEY_NAME,
    KEY_PASSWD,
    KEY_ROLE,
    KEY_CAMPUS,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "UUID"},
    {kvalid_stringne, "name"},
    {kvalid_stringne, "passwd"},
    {kvalid_stringne, "role"},
    {kvalid_stringne, "campus"},
};

enum statement {
    STMTS_CHECK,
    STMTS_ADD,
    __STMT_STORE__,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts_data[STMTS__MAX] = {
    {
        (char *) "SELECT NOT EXISTS(SELECT 1 FROM ACCOUNT WHERE UUID = (?)) AND "
        "EXISTS(SELECT 1 FROM ROLE WHERE roleName = (?) AND perms <= 1) AND "
        "EXISTS(SELECT 1 FROM CAMPUS WHERE campusName = (?))"
    },
    {
        (char *)
        "INSERT INTO ACCOUNT (UUID, displayname, pwhash, campus, role) "
        "VALUES ((?), (?), (?), (?), (?))"
    },
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'SIGNUP',datetime('now','localtime'),(?))"
    }
};

struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx_data;
struct sqlbox_cfg cfg_data;
size_t dbid_data; // Database associated with a config and a context

void alloc_ctx_cfg() {
    memset(&cfg_data, 0, sizeof(struct sqlbox_cfg));
    cfg_data.msg.func_short = warnx;
    cfg_data.srcs.srcsz = 1;
    cfg_data.srcs.srcs = srcs;
    cfg_data.stmts.stmtsz = STMTS__MAX;
    cfg_data.stmts.stmts = pstmts_data;
    if ((boxctx_data = sqlbox_alloc(&cfg_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_data = sqlbox_open(boxctx_data, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}

enum khttp sanitize() {
    if (r.method != KMETHOD_POST)
        return KHTTP_405;
    if (!(r.fieldmap[KEY_UUID] && r.fieldmap[KEY_NAME] && r.fieldmap[KEY_PASSWD] && r.fieldmap[KEY_ROLE] && r.fieldmap[
              KEY_CAMPUS]))
        return KHTTP_406;
    return KHTTP_200;
}

bool check() {
    size_t stmtid;
    size_t parmsz = 3;
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_ROLE]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_CAMPUS]->parsed.s
        }
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, STMTS_CHECK, parmsz, parms, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx_data, stmtid)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    if (res->ps[0].iparm == 0) {
        sqlbox_finalise(boxctx_data, stmtid);
        return false;
    }
    sqlbox_finalise(boxctx_data, stmtid);
    return true;
}

void create_acc() {
    char hash[_PASSWORD_LEN];
    crypt_newhash(r.fieldmap[KEY_PASSWD]->parsed.s, "bcrypt,a", hash,_PASSWORD_LEN);
    size_t parmsz = 5;
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_NAME]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = hash
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_CAMPUS]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_ROLE]->parsed.s
        },
    };
    if (sqlbox_exec(boxctx_data, dbid_data, STMTS_ADD, parmsz, parms,SQLBOX_STMT_CONSTRAINT) != SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE],"%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putboolp(&req, "account_created",true);
    kjson_objp_open(&req, "user");
    kjson_putstringp(&req, "UUID", r.fieldmap[KEY_UUID]->parsed.s);
    kjson_putstringp(&req, "disp_name", r.fieldmap[KEY_NAME]->parsed.s);
    kjson_putstringp(&req, "campus", r.fieldmap[KEY_CAMPUS]->parsed.s);
    kjson_putstringp(&req, "role", r.fieldmap[KEY_ROLE]->parsed.s);
    kjson_putboolp(&req, "frozen", false);
    kjson_objp_open(&req, "perms");
    kjson_putintp(&req, "numeric", 1);
    kjson_putboolp(&req, "admin", 0);
    kjson_putboolp(&req, "staff", 0);
    kjson_putboolp(&req, "manage_stock", 0);
    kjson_putboolp(&req, "manage_inventories", 0);
    kjson_putboolp(&req, "see_accounts", 0);
    kjson_putboolp(&req, "monitor_history", 0);
    kjson_putboolp(&req, "has_inventory", 0);
    kjson_obj_close(&req);

    kjson_obj_close(&req);
    kjson_obj_close(&req);
}

void save(const bool failed) {
    char *description;
    kasprintf(&description, "%s, Parms: (UUID: %s,disp_name: %s,role: %s,campus: %s)",
              failed ? "SIGNUP FAILED" : "SIGNUP SUCCESSFUL", r.fieldmap[KEY_UUID]->parsed.s,
              r.fieldmap[KEY_NAME]->parsed.s, r.fieldmap[KEY_ROLE]->parsed.s, r.fieldmap[KEY_CAMPUS]->parsed.s);
    size_t parmsz_save = 3;
    struct sqlbox_parm parms_save[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.remote
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = description
        },
    };
    if (sqlbox_exec(boxctx_data, dbid_data, __STMT_STORE__, parmsz_save, parms_save,SQLBOX_STMT_CONSTRAINT) !=
        SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, NULL, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;

    alloc_ctx_cfg();
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        goto cleanup;
    }
    if (check() == false) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],"%s", kmimetypes[KMIME_APP_JSON]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putboolp(&req, "account_created",false);
        kjson_putstringp(&req, "error", "Something went wrong, Review your parameters and try again later!");
        kjson_obj_close(&req);
        save(true);
        goto cleanup;
    }
    create_acc();
    save(false);
cleanup:
    sqlbox_close(boxctx_data, dbid_data);
    sqlbox_free(boxctx_data);
    khttp_free(&r);
    return 0;
}
