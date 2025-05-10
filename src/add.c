#include <argon2.h>
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
#define HASHLEN 32
#define SALTLEN 16
#ifndef __BSD_VISIBLE
#define	_PASSWORD_LEN		128
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
    STMTS_ADD_PUBLISHER,
    STMTS_ADD_AUTHOR,
    STMTS_ADD_LANG,
    STMTS_ADD_ACTION,
    STMTS_ADD_DOCTYPE,
    STMTS_ADD_CAMPUS,
    STMTS_ADD_ROLE,
    STMTS_ADD_CATEGORY,
    STMTS_ADD_ACCOUNT,
    STMTS_ADD_BOOK,
    STMTS_ADD_LANGUAGES,
    STMTS_ADD_AUTHORED,
    STMTS_ADD_STOCK,
    STMTS_ADD_INVENTORY,
    STMTS_LOGIN,
    STMTS_SAVE,
    STMTS_DEFAULT_STOCK,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {(char *) "INSERT INTO PUBLISHER VALUES (?)"},
    {(char *) "INSERT INTO AUTHOR VALUES (?)"},
    {(char *) "INSERT INTO LANG VALUES (?)"},
    {(char *) "INSERT INTO ACTION VALUES (?)"},
    {(char *) "INSERT INTO DOCTYPE VALUES (?)"},
    {(char *) "INSERT INTO CAMPUS VALUES (?)"},
    {(char *) "INSERT INTO ROLE VALUES (?,?)"},
    {(char *) "INSERT INTO CATEGORY VALUES (?,?,?)"},
    {(char *) "INSERT INTO ACCOUNT VALUES (?,?,?,?,?,?)"},
    {(char *) "INSERT INTO BOOK VALUES (?,?,?,?,?,?,?,?,0)"},
    {(char *) "INSERT INTO LANGUAGES VALUES(?,?)"},
    {(char *) "INSERT INTO AUTHORED VALUES(?,?)"},
    {(char *) "INSERT INTO STOCK VALUES(?,?,?)"},
    {(char *) "INSERT INTO INVENTORY VALUES(?,?,?,?,?)"},
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
    }
};

static enum keys switch_keys[STMTS__MAX][10] = {
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY__MAX},
    {KEY_NAME, KEY_PERMS, KEY__MAX},
    {KEY_CLASS, KEY_NAME, KEY_PARENT, KEY__MAX},
    {
        KEY_UUID, KEY_NAME, KEY_PW, KEY_CAMPUS, KEY_ROLE, KEY_FROZEN,
        KEY__MAX
    },
    {
        KEY_SERIALNUM, KEY_TYPE, KEY_CATEGORY, KEY_PUBLISHER, KEY_NAME,
        KEY_RELEASEYEAR, KEY_COVER, KEY_DESCRIPTION, KEY__MAX
    },
    {KEY_SERIALNUM, KEY_LANG, KEY__MAX},
    {KEY_SERIALNUM, KEY_AUTHOR, KEY__MAX},
    {KEY_SERIALNUM, KEY_CAMPUS, KEY_INSTOCK, KEY__MAX},
    {KEY_UUID, KEY_SERIALNUM, KEY_DURATION, KEY_DATE, KEY_EXTENDED, KEY__MAX}
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

void process() {
    struct sqlbox_parm parms[8];
    size_t parmsz = 0;
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
    if (sqlbox_exec(boxctx, dbid, r.page, parmsz, parms,SQLBOX_STMT_CONSTRAINT) !=
        SQLBOX_CODE_OK)
        errx(EXIT_FAILURE, "sqlbox_exec");
    if (r.page == PG_BOOK) {
        if (sqlbox_exec(boxctx, dbid, STMTS_DEFAULT_STOCK, 1, parms,SQLBOX_STMT_CONSTRAINT) !=
            SQLBOX_CODE_OK)

            errx(EXIT_FAILURE, "sqlbox_exec");
        struct kpair *temp_field = r.fieldmap[KEY_LANG];
        while (temp_field) {
            struct sqlbox_parm temp_parms[] = {
                {.type = SQLBOX_PARM_STRING, .sparm = r.fieldmap[KEY_SERIALNUM]->parsed.s},
                {.type = SQLBOX_PARM_STRING, .sparm = temp_field->parsed.s}
            };

            if (sqlbox_exec(boxctx, dbid, STMTS_ADD_LANGUAGES, 2, temp_parms,SQLBOX_STMT_CONSTRAINT) !=
                SQLBOX_CODE_OK)
                errx(EXIT_FAILURE, "sqlbox_exec");
            temp_field = temp_field->next;
        }
        temp_field = r.fieldmap[KEY_AUTHOR];
        while (temp_field) {
            struct sqlbox_parm temp_parms[] = {
                {.type = SQLBOX_PARM_STRING, .sparm = r.fieldmap[KEY_SERIALNUM]->parsed.s},
                {.type = SQLBOX_PARM_STRING, .sparm = temp_field->parsed.s}
            };

            if (sqlbox_exec(boxctx, dbid, STMTS_ADD_AUTHORED, 2, temp_parms,SQLBOX_STMT_CONSTRAINT) !=
                SQLBOX_CODE_OK)
                errx(EXIT_FAILURE, "sqlbox_exec");
            temp_field = temp_field->next;
        }
    }
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
    //if ((er = second_pass()) != KHTTP_200)goto error;
    process();
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_body(&r);
    if (r.mime == KMIME_TEXT_HTML)
        khttp_puts(&r, "Successful");
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
