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


enum pg {
    PG_PUBLISHER,
    PG_AUTHOR,
    PG_LANG,
    PG_ACTION,
    PG_DOCTYPE,
    PG_CAMPUS,
    PG_ROLE,
    PG_CATEGORY,
    PG_ACCOUNT,
    PG_BOOK,
    PG_LANGUAGES,
    PG_AUTHORED,
    PG_STOCK,
    PG_INVENTORY,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "publisher", "author", "lang", "action", "doctype", "campus", "role", "category", "account", "book", "languages",
    "authored", "stock", "inventory"
};


enum keys {
    KEY_NAME,
    KEY_PERMS,
    KEY_CLASS,
    KEY_PARENT,
    KEY_UUID,
    KEY_PW,
    KEY_CAMPUS,
    KEY_ROLE,
    KEY_FROZEN,
    KEY_SERIALNUM,
    KEY_TYPE,
    KEY_CATEGORY,
    KEY_PUBLISHER,
    KEY_RELEASEYEAR,
    KEY_COVER,
    KEY_DESCRIPTION,
    KEY_LANG,
    KEY_AUTHOR,
    KEY_INSTOCK,
    KEY_DURATION,
    KEY_DATE,
    KEY_EXTENDED,
    COOKIE_SESSIONID,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "name"},
    {kvalid_int, "perms"},
    {kvalid_stringne, "class"},
    {kvalid_stringne, "parent"},
    {kvalid_stringne, "uuid"},
    {kvalid_stringne, "pw"},
    {kvalid_stringne, "campus"},
    {kvalid_stringne, "role"},
    {kvalid_int, "frozen"},
    {kvalid_stringne, "serialnum"},
    {kvalid_stringne, "type"},
    {kvalid_stringne, "category"},
    {kvalid_stringne, "publisher"},
    {kvalid_int, "year"},
    {NULL, "cover"},
    {kvalid_string, "description"},
    {kvalid_stringne, "lang"},
    {kvalid_stringne, "author"},
    {kvalid_int, "stock"},
    {kvalid_int, "duration"},
    {kvalid_date, "date"},
    {kvalid_int, "extended"},
    {kvalid_stringne, "sessionID"}

};

enum statement {
    STMTS_DELETE_PUBLISHER,
    STMTS_DELETE_AUTHOR,
    STMTS_DELETE_LANG,
    STMTS_DELETE_ACTION,
    STMTS_DELETE_DOCTYPE,
    STMTS_DELETE_CAMPUS,
    STMTS_DELETE_ROLE,
    STMTS_DELETE_CATEGORY,
    STMTS_DELETE_ACCOUNT,
    STMTS_DELETE_BOOK,
    STMTS_DELETE_LANGUAGES,
    STMTS_DELETE_AUTHORED,
    STMTS_DELETE_STOCK,
    STMTS_DELETE_INVENTORY,
    STMTS_LOGIN,
    STMTS_SAVE,
    STMTS_DEFAULT_STOCK,
    STMTS_CHANGES,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {(char *) "DELETE FROM PUBLISHER WHERE publisherName = (?)"},
    {(char *) "DELETE FROM AUTHOR WHERE authorName = (?)"},
    {(char *) "DELETE FROM LANG WHERE langCode = (?)"},
    {(char *) "DELETE FROM ACTION WHERE actionName = (?)"},
    {(char *) "DELETE FROM DOCTYPE WHERE typeName = (?)"},
    {(char *) "DELETE FROM CAMPUS WHERE campusName = (?)"},
    {(char *) "DELETE FROM ROLE WHERE roleName = (?)"},
    {(char *) "DELETE FROM CATEGORY WHERE categoryClass = (?)"},
    {(char *) "DELETE FROM ACCOUNT WHERE UUID = (?)"},
    {(char *) "DELETE FROM BOOK WHERE serialnum = (?)"},
    {(char *) "DELETE FROM LANGUAGES WHERE (serialnum,lang) = (?,?)"},
    {(char *) "DELETE FROM AUTHORED WHERE (serialnum,author) = (?,?)"},
    {(char *) "DELETE FROM STOCK WHERE (serialnum,campus) = (?,?)"},
    {(char *) "DELETE FROM INVENTORY WHERE (UUID,serialnum) =(?,?)"},
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
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'EDIT',datetime('now','localtime'),(?))"
    },
    {
        (char *) "INSERT INTO STOCK(serialnum, campus) SELECT(?) AS serialnum ,campusName AS campus FROM CAMPUS"
    },
    {(char *) "SELECT changes()"}
};

static enum keys switch_keys[STMTS__MAX][10] = {
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_CLASS, KEY__MAX},
    {KEY_UUID, KEY__MAX},
    {KEY_SERIALNUM, KEY__MAX},
    {KEY_SERIALNUM, KEY_LANG, KEY__MAX},
    {KEY_SERIALNUM, KEY_AUTHOR, KEY__MAX},
    {KEY_SERIALNUM, KEY_CAMPUS, KEY__MAX},
    {KEY_UUID, KEY_SERIALNUM, KEY__MAX}
};
struct sqlbox_src srcs[] = {
    {
        .fname = (char *) "db/database.db",
        .mode = SQLBOX_SRC_RW
    }
};
struct sqlbox *boxctx;
struct sqlbox_cfg cfg;
size_t dbid; // Database associated with a config and a context for query results

/*
 * Allocates the context and source for the current operations
 */
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

struct usr {
    char *UUID;
    char *disp_name;
    char *campus;
    char *role;
    char *sessionID;
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
    .sessionID = NULL,
    .perms = {0, 0, 0, 0, 0, 0, 0, 0}
};

void fill_user() {
    struct kpair *field;

    if ((field = r.cookiemap[COOKIE_SESSIONID])) {
        size_t stmtid;
        size_t parmsz = 1;
        const struct sqlbox_parmset *res;
        struct sqlbox_parm parms[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s},
        };
        if (!(stmtid = sqlbox_prepare_bind(boxctx, dbid, STMTS_LOGIN, parmsz, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
        if ((res = sqlbox_step(boxctx, stmtid)) == NULL)
            errx(EXIT_FAILURE, "sqlbox_step");
        if (res->psz != 0) {
            curr_usr.authenticated = true;
            kasprintf(&curr_usr.UUID, "%s", res->ps[0].sparm);

            kasprintf(&curr_usr.disp_name, "%s", res->ps[1].sparm);

            kasprintf(&curr_usr.campus, "%s", res->ps[3].sparm);

            kasprintf(&curr_usr.role, "%s", res->ps[4].sparm);

            kasprintf(&curr_usr.sessionID, "%s", field->parsed.s);

            curr_usr.perms = int_to_accperms((int) res->ps[5].iparm);

            curr_usr.frozen = res->ps[6].iparm;
        }
        sqlbox_finalise(boxctx, stmtid);
    }
}


enum khttp sanitize() {
    if (r.method != KMETHOD_GET)
        return KHTTP_405;
    if (r.page == PG__MAX)
        return KHTTP_404;
    return KHTTP_200;
}

enum khttp second_pass() {
    if (!curr_usr.authenticated)
        return KHTTP_403;
    //TODO: ROBUST PERMISSIONS
    if (!(curr_usr.perms.admin || curr_usr.perms.staff))
        return KHTTP_403;
    return KHTTP_200;
}

enum khttp process() {
    struct sqlbox_parm parms[8];
    size_t parmsz = 0;
    enum sqlbox_code err;
    struct kpair *field;
    for (int i = 0; switch_keys[r.page][i] != KEY__MAX; ++i) {
        if ((field = r.fieldmap[switch_keys[r.page][i]])) {
            switch (field->type) {
                case KPAIR_INTEGER:
                    parms[parmsz++] = (struct sqlbox_parm){.type = SQLBOX_PARM_INT, .iparm = field->parsed.i};
                    break;
                case KPAIR_STRING:
                    parms[parmsz++] = (struct sqlbox_parm){.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s};
                    break;
                default:
                    break;
            }
        } else {
            parms[parmsz++] = (struct sqlbox_parm){.type = SQLBOX_PARM_NULL};
        }
    }

    if ((err = sqlbox_exec(boxctx, dbid, r.page, parmsz, parms,SQLBOX_STMT_CONSTRAINT)) !=
        SQLBOX_CODE_OK)
        return KHTTP_400;
    size_t stmtid_count;
    const struct sqlbox_parmset *res;
    if (!(stmtid_count = sqlbox_prepare_bind(boxctx, dbid, STMTS_CHANGES, 0, 0, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx, stmtid_count)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    const int nbr = (int) res->ps[0].iparm;
    sqlbox_finalise(boxctx, stmtid_count);

    return (nbr != 0) ? KHTTP_200 : KHTTP_400;
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG__MAX) != KCGI_OK)
        return EXIT_FAILURE;
    khttp_head(&r, "Access-Control-Allow-Origin", "https://seele.serveo.net");
    khttp_head(&r, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    khttp_head(&r, "Access-Control-Allow-Credentials", "true");
    if ((er = sanitize()) != KHTTP_200)goto error;
    alloc_ctx_cfg();
    fill_user();
    //if ((er = second_pass()) != KHTTP_200)goto access_denied;
    if ((er = process()) != KHTTP_200) goto access_denied;
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    if (curr_usr.authenticated) {
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

    kjson_putstringp(&req, "status", "Ressource deleted successfully!");
    kjson_obj_close(&req);
    kjson_close(&req);
    goto cleanup;
access_denied:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    if (curr_usr.authenticated) {
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

    kjson_putstringp(&req, "error", "You don't have the permissions to edit this ressource or ressource non-existant");
    kjson_obj_close(&req);
    kjson_close(&req);
cleanup:
    khttp_free(&r);
    sqlbox_free(boxctx);
    return 0;
error:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
    khttp_body(&r);
    if (r.mime == KMIME_TEXT_HTML)
        khttp_puts(&r, "Could not service request");
    khttp_free(&r);
    return 0;
}
