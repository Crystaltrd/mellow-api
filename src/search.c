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

struct kreq r;
struct kjsonreq req;
/*
 * The Permissions struct for a role, the order in this struct is the same as
 * how it should be interpreted in binary
 */
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


enum key {
    COOKIE_SESSIONID,
    KEY_STRING,
    KEY_PAGE,
    KEY_LIMIT,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_stringne, "sessionID"},
    {kvalid_stringne, "q"},
    {kvalid_int, "page"},
    {kvalid_int, "limit"}
};

enum statment {
    STMTS_SEARCH,
    STMTS_COUNT,
    STMTS_LOGIN,
    STMTS_SAVE,
    STMTS_AUTHORS,
    STMTS_LANGS,
    STMTS__MAX
};

static struct sqlbox_pstmt pstmts[STMTS__MAX] = {
    {
        (char *)
        "SELECT BOOK.serialnum, type, category, categoryName, publisher, bootitle, bookreleaseyear, bookcover, description, hits "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "WHERE CATEGORY.categoryClass = BOOK.category "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND instr(concat(BOOK.serialnum,booktitle,lang,author,type,publisher,campus,UUID,bookreleaseyear,description,categoryName), (?)) > 0 "
        "GROUP BY BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "ORDER BY BOOK.serialnum "
        "LIMIT(? * ?),(?)"
    },
    {
        (char *)
        "SELECT COUNT(DISTINCT CONCAT(BOOK.serialnum, type, category, categoryName, publisher, bootitle, bookreleaseyear, bookcover, description, hits)) "
        "FROM (BOOK LEFT JOIN INVENTORY I ON BOOK.serialnum = I.serialnum),"
        "CATEGORY,"
        "LANGUAGES,"
        "AUTHORED,"
        "WHERE CATEGORY.categoryClass = BOOK.category "
        "AND AUTHORED.serialnum = BOOK.serialnum "
        "AND LANGUAGES.serialnum = BOOK.serialnum "
        "AND instr(concat(BOOK.serialnum,booktitle,lang,author,type,publisher,campus,UUID,bookreleaseyear,description,categoryName), (?)) > 0 "
        "GROUP BY BOOK.serialnum, type, category, categoryName, publisher, booktitle, bookreleaseyear, bookcover, hits "
        "ORDER BY BOOK.serialnum "
        "LIMIT(? * ?),(?)"
    },
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
        (char *)
        "SELECT author "
        "FROM AUTHORED "
        "WHERE serialnum = (?)"
    },
    {
        (char *)
        "SELECT lang "
        "FROM LANGUAGES "
        "WHERE serialnum = (?)"
    }
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
    if (!r.fieldmap[KEY_STRING])
        return KHTTP_400;
    return KHTTP_200;
}

static const char *rows[] ={
    "BOOK.serialnum", "type", "category", "categoryName", "publisher", "booktitle", "bookreleaseyear", "bookcover",
    "description",
    "hits",NULL
};

void process() {
    size_t stmtid_data;
    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms[] = {
        {
            .type = SQLBOX_PARM_STRING,
            .sparm = r.fieldmap[KEY_STRING]->parsed.s
        },
        {
            .type = SQLBOX_PARM_INT,
            .iparm = (r.fieldmap[KEY_PAGE] ? r.fieldmap[KEY_PAGE]->parsed.i : 0)
        },
        {
            .type = SQLBOX_PARM_INT,
            .iparm = r.fieldmap[KEY_LIMIT] ? r.fieldmap[KEY_LIMIT]->parsed.i : 25
        },
        {
            .type = SQLBOX_PARM_INT,
            .iparm = r.fieldmap[KEY_LIMIT] ? r.fieldmap[KEY_LIMIT]->parsed.i : 25
        }
    }; //Array of statement parameters
    size_t parmsz = 4;
    if (!(stmtid_data = sqlbox_prepare_bind(boxctx, dbid, STMTS_SEARCH, parmsz, parms, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_objp_open(&req, "user");
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
    kjson_obj_close(&req);
    kjson_arrayp_open(&req, "res");
    while ((res = sqlbox_step(boxctx, stmtid_data)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(&req);
        for (int i = 0; i < (int) res->psz; ++i) {
            switch (res->ps[i].type) {
                case SQLBOX_PARM_INT:
                    kjson_putintp(&req, rows[i], res->ps[i].iparm);
                    break;
                case SQLBOX_PARM_STRING:
                    if (i == 0)
                        kjson_putstringp(&req, "serialnum", res->ps[i].sparm);
                    kjson_putstringp(&req, rows[i], res->ps[i].bparm);
                    break;
                case SQLBOX_PARM_FLOAT:
                    kjson_putdoublep(&req, rows[i], res->ps[i].fparm);
                    break;
                case SQLBOX_PARM_BLOB:
                    kjson_putstringp(&req, rows[i], res->ps[i].bparm);
                    break;
                case SQLBOX_PARM_NULL:
                    kjson_putnullp(&req, rows[i]);
                    break;
                default:
                    break;
            }
        }

        size_t stmtid_book_data;
        size_t parmsz_book = 1;
        const struct sqlbox_parmset *res_book;
        struct sqlbox_parm parms_book[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = res->ps[0].sparm},
        };
        if (!(stmtid_book_data =
              sqlbox_prepare_bind(boxctx, dbid, STMTS_AUTHORS, parmsz_book, parms_book,
                                  SQLBOX_STMT_MULTI)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");

        kjson_arrayp_open(&req, "authors");
        while ((res_book = sqlbox_step(boxctx, stmtid_book_data)) != NULL && res_book->code ==
               SQLBOX_CODE_OK
               && res_book->psz != 0)
            kjson_putstring(&req, res_book->ps[0].sparm);
        kjson_array_close(&req);

        if (!sqlbox_finalise(boxctx, stmtid_book_data))
            errx(EXIT_FAILURE, "sqlbox_finalise");

        if (!(stmtid_book_data =
              sqlbox_prepare_bind(boxctx, dbid, STMTS_LANGS, parmsz_book, parms_book,
                                  SQLBOX_STMT_MULTI)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");

        kjson_arrayp_open(&req, "langs");
        while ((res_book = sqlbox_step(boxctx, stmtid_book_data)) != NULL && res_book->code ==
               SQLBOX_CODE_OK
               && res_book->psz != 0)
            kjson_putstring(&req, res_book->ps[0].sparm);
        kjson_array_close(&req);

        if (!sqlbox_finalise(boxctx, stmtid_book_data))
            errx(EXIT_FAILURE, "sqlbox_finalise");


        kjson_obj_close(&req);
    }
    if (!sqlbox_finalise(boxctx, stmtid_data))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    kjson_array_close(&req);
    if (!(stmtid_data = sqlbox_prepare_bind(boxctx, dbid, STMTS_COUNT, parmsz - 3, parms, SQLBOX_STMT_MULTI)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx, stmtid_data)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    kjson_putintp(&req, "nbrres", res->ps[0].iparm);
    if (!sqlbox_finalise(boxctx, stmtid_data))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    kjson_obj_close(&req);
    kjson_close(&req);
}

int main() {
    enum khttp er;
    if (khttp_parse(&r, keys, KEY__MAX, 0, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;
    khttp_head(&r, "Access-Control-Allow-Origin", "https://seele.serveo.net");
    khttp_head(&r, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    khttp_head(&r, "Access-Control-Allow-Credentials", "true");
    if ((er = sanitize()) != KHTTP_200) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
        khttp_body(&r);
        if (r.mime == KMIME_TEXT_HTML)
            khttp_puts(&r, "Could not service request.");
        khttp_free(&r);
        return 0;
    }
    alloc_ctx_cfg();
    fill_user();
    process();
    khttp_free(&r);
    sqlbox_free(boxctx);
    return EXIT_SUCCESS;
}
