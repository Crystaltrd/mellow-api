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
struct kreq r;
struct kjsonreq req;

enum statement {
    STMTS_LOGOUT,
    STMTS_LOGOUTALL,
    __STMTS_LOGIN__,
    STMTS__MAX
};

enum key {
    KEY_SESSION,
    KEY_UUID,
    KEY_DEAUTH_ALL,
    KEY__MAX,
};


static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "sessionID"},
    {kvalid_stringne, "uuid"},
    {NULL, "all"}
};
static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {(char *) "DELETE FROM SESSIONS WHERE sessionID = (?)"},
    {(char *) "DELETE FROM SESSIONS WHERE account = (?)"},
    {
        (char *)
        "SELECT ACCOUNT.UUID, displayname, pwhash, campus, role, perms, frozen "
        "FROM ROLE,"
        "ACCOUNT "
        "LEFT JOIN SESSIONS S on ACCOUNT.UUID = S.account "
        "WHERE ACCOUNT.role = ROLE.roleName "
        "AND sessionID = (?) "
        "GROUP BY ACCOUNT.UUID, displayname, pwhash, campus, perms, frozen "
    },
};

struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx_data;
struct sqlbox_cfg cfg_data;
size_t dbid_data; // Database associated with a config and a context for query results
void alloc_ctx_cfg() {
    memset(&cfg_data, 0, sizeof(struct sqlbox_cfg));
    cfg_data.msg.func_short = warnx;
    cfg_data.srcs.srcsz = 1;
    cfg_data.srcs.srcs = srcs;
    cfg_data.stmts.stmtsz = STMTS__MAX;
    cfg_data.stmts.stmts = pstmts;
    if ((boxctx_data = sqlbox_alloc(&cfg_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid_data = sqlbox_open(boxctx_data, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
}

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

void fill_user() {
    struct kpair *field;
    if ((field = r.cookiemap[KEY_SESSION])) {
        size_t stmtid;
        size_t parmsz = 1;
        const struct sqlbox_parmset *res;
        struct sqlbox_parm parms[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s},
        };
        if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, __STMTS_LOGIN__, parmsz, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
        if ((res = sqlbox_step(boxctx_data, stmtid)) == NULL)
            errx(EXIT_FAILURE, "sqlbox_step");
        if (res->psz != 0) {
            curr_usr.authenticated = true;
            kasprintf(&curr_usr.UUID, "%s", res->ps[0].sparm);

            kasprintf(&curr_usr.disp_name, "%s", res->ps[1].sparm);

            kasprintf(&curr_usr.campus, "%s", res->ps[3].sparm);

            kasprintf(&curr_usr.role, "%s", res->ps[4].sparm);

            curr_usr.perms = int_to_accperms((int) res->ps[5].iparm);

            curr_usr.frozen = res->ps[6].iparm;
        }
        sqlbox_finalise(boxctx_data, stmtid);
    }
}

enum khttp sanitize() {
    if (r.method != KMETHOD_GET)
        return KHTTP_405;
    if ((r.fieldmap[KEY_SESSION] && (r.fieldmap[KEY_UUID] || r.fieldmap[KEY_DEAUTH_ALL])) || !r.cookiemap[KEY_SESSION])
        return KHTTP_403;

    return KHTTP_200;
}

enum statement get_stmts() {
    if (r.fieldmap[KEY_DEAUTH_ALL] || r.fieldmap[KEY_UUID])
        return STMTS_LOGOUTALL;
    return STMTS_LOGOUT;
}

int process(const enum statement STMT) {
    size_t parmsz = 1;
    struct sqlbox_parm *parms = kcalloc(parmsz, sizeof(struct sqlbox_parm));
    int reset_cookie = 0;
    parms[0].type = SQLBOX_PARM_STRING;
    if (r.fieldmap[KEY_SESSION])
        parms[0].sparm = r.fieldmap[KEY_SESSION]->parsed.s;
    else if (r.fieldmap[KEY_UUID])
        parms[0].sparm = r.fieldmap[KEY_UUID]->parsed.s;
    else if (r.fieldmap[KEY_DEAUTH_ALL]) {
        parms[0].sparm = curr_usr.UUID;
        reset_cookie = 1;
    } else {
        parms[0].sparm = r.cookiemap[KEY_SESSION]->parsed.s;
        reset_cookie = 1;
    }
    if (sqlbox_exec(boxctx_data, dbid_data, STMT, parmsz, parms,SQLBOX_STMT_CONSTRAINT) !=
        SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
    free(parms);
    return reset_cookie;
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, 0, 0, 0) != KCGI_OK)
        errx(EXIT_FAILURE, "parse");
    goto error;
    if ((er = sanitize()) != KHTTP_200) goto error;
    fill_user();
    if (!curr_usr.authenticated) goto error;
    enum statement STMT = get_stmts();
    if ((r.fieldmap[KEY_SESSION] || r.fieldmap[KEY_UUID]) && !(curr_usr.perms.staff || curr_usr.perms.admin))goto error;
    int disconnected = process(STMT);
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    if (disconnected)
        khttp_head(&r, kresps[KRESP_SET_COOKIE], "sessionID=%s; Path=/; Max-Age=-1",
                   r.cookiemap[KEY_SESSION]->parsed.s);
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_objp_open(&req, "user");
    kjson_putstringp(&req, "IP", r.remote);
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
    kjson_obj_close(&req);
    kjson_putboolp(&req, "disconnected", disconnected);
    kjson_obj_close(&req);
    kjson_close(&req);
    goto cleanup;
error:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_400]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    if (r.mime == KMIME_TEXT_HTML)
        khttp_puts(&r, "Could not service request");
    khttp_free(&r);
    return 0;
cleanup:
    khttp_free(&r);
    sqlbox_free(boxctx_data);
    return 0;
}
