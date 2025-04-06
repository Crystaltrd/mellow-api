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


struct accperms {
    int numeric;
    bool admin;
    bool staff;
    bool manage_stock;
    bool manage_inventories;
    bool see_accounts;
    bool monitor_history;
    bool has_inventory;
};

struct accperms int_to_accperms(int perm) {
    struct accperms perms = {
        .numeric = perm,
        .admin = (perm & (1 << 6)),
        .staff = (perm & (1 << 5)),
        .manage_stock = (perm & (1 << 4)),
        .manage_inventories = (perm & (1 << 3)),
        .see_accounts = (perm & (1 << 2)),
        .monitor_history = (perm & (1 << 1)),
        .has_inventory = (perm & 1)
    };
    return perms;
}

struct usr {
    char *UUID;
    char *disp_name;
    char *campus;
    char *role;
    struct accperms perms;
    bool authenticated;
    bool frozen;
};

struct usr curr_usr = {
    .authenticated = false,
    .frozen = false,
    .UUID = NULL,
    .disp_name = NULL,
    .campus = NULL,
    .role = NULL,
    .perms = {0, 0, 0, 0, 0, 0, 0, 0}
};

enum statement {
    STMTS_CHECK,
    STMTS_ADD,
    __STMT_STORE__,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts_data[STMTS__MAX] = {
    {
        (char *) "SELECT displayname, campus, role, frozen, perms,pwhash "
        "FROM ACCOUNT,ROLE "
        "WHERE roleName = role "
        "AND UUID=(?) "
        "LIMIT 1"
    },
    {
        (char *)
        "INSERT INTO SESSIONS "
        "VALUES((?),(?),datetime('now',(?),'localtime'))"
    },
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'LOGIN',datetime('now','localtime'),(?))"
    }
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
    if (r.method != KMETHOD_GET)
        return KHTTP_405;
    if (!r.fieldmap[KEY_UUID] || !r.fieldmap[KEY_PASSWD])
        return KHTTP_406;
    return KHTTP_200;
}


bool check_passwd() {
    size_t stmtid;
    size_t parmsz = 1;
    char *hash;
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        }
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, STMTS_CHECK, parmsz, parms, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx_data, stmtid)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    if (res->psz == 0) {
        sqlbox_finalise(boxctx_data, stmtid);
        return false;
    }
    curr_usr.authenticated = true;
    kasprintf(&curr_usr.UUID, "%s", r.fieldmap[KEY_UUID]->parsed.s);
    kasprintf(&curr_usr.disp_name, "%s", res->ps[0].sparm);
    kasprintf(&curr_usr.campus, "%s", res->ps[1].sparm);
    kasprintf(&curr_usr.role, "%s", res->ps[2].sparm);
    curr_usr.frozen = res->ps[3].iparm;
    curr_usr.perms = int_to_accperms((int) res->ps[4].iparm);
    kasprintf(&hash,"%s", res->ps[5].sparm);

    sqlbox_finalise(boxctx_data, stmtid);
    if (crypt_checkpass(r.fieldmap[KEY_PASSWD]->parsed.s, hash) != 0)
        return false;

    return true;
}

void open_session() {
    size_t parmsz = 3;
    char seed[128], *timestamp, sessionID[_PASSWORD_LEN + 64];
    kasprintf(&timestamp, "%lld", time(NULL));
    arc4random_buf(seed, 128);
    crypt_newhash(seed, "bcrypt,a", sessionID,_PASSWORD_LEN);
    strlcat(sessionID, timestamp,_PASSWORD_LEN + 64);
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_UUID]->parsed.s
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = sessionID
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = (r.fieldmap[KEY_REMEMBER]) ? "+7 days" : "+3 hours"
        }
    };
    if (sqlbox_exec(boxctx_data, dbid_data, STMTS_ADD, parmsz, parms,SQLBOX_STMT_CONSTRAINT) != SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");

    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_head(&r, kresps[KRESP_SET_COOKIE],
               "sessionID=%s; Path=/; Max-Age=%d", sessionID,
               ((r.fieldmap[KEY_REMEMBER]) ? 7 * 24 * 60 * 60 : 60 * 60 * 3));
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putboolp(&req, "authenticated",true);
    kjson_putstringp(&req, "sessionid", sessionID);

    kjson_putstringp(&req, "UUID", curr_usr.UUID);
    kjson_putstringp(&req, "disp_name", curr_usr.disp_name);
    kjson_putstringp(&req, "campus", curr_usr.campus);
    kjson_putstringp(&req, "role", curr_usr.role);
    kjson_putboolp(&req, "frozen", curr_usr.frozen);

    kjson_objp_open(&req, "perms");
    kjson_putintp(&req, "numeric", curr_usr.perms.numeric);
    kjson_putboolp(&req, "admin", curr_usr.perms.admin);
    kjson_putboolp(&req, "staff", curr_usr.perms.staff);
    kjson_putboolp(&req, "manage_stock", curr_usr.perms.manage_stock);
    kjson_putboolp(&req, "manage_inventories", curr_usr.perms.manage_inventories);
    kjson_putboolp(&req, "see_accounts", curr_usr.perms.see_accounts);
    kjson_putboolp(&req, "monitor_history", curr_usr.perms.monitor_history);
    kjson_putboolp(&req, "has_inventory", curr_usr.perms.has_inventory);
    kjson_obj_close(&req);
}

void save(const bool failed) {
    size_t parmsz_save = 3;
    struct sqlbox_parm parms_save[] = {
        {
            .type = (r.fieldmap[KEY_UUID]) ? SQLBOX_PARM_STRING : SQLBOX_PARM_NULL,
            .sparm = (r.fieldmap[KEY_UUID]) ? r.fieldmap[KEY_UUID]->parsed.s : NULL
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.remote
        },
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = failed ? "ACCESS DENIED" : "LOGIN SUCCESSFUL"
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
        save(true);
        goto cleanup;
    }
    if (check_passwd() == false) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_PLAIN]);
        khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putboolp(&req, "authenticated",false);
        kjson_putstringp(&req, "error", "The Username or Password that you provided are wrong");
        kjson_obj_close(&req);
        save(true);
        goto cleanup;
    }
    open_session();
    save(false);
cleanup:
    sqlbox_close(boxctx_data, dbid_data);
    sqlbox_free(boxctx_data);
    khttp_free(&r);
    return EXIT_SUCCESS;
}
