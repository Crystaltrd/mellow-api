#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <unistd.h> /* pledge() */
#include <err.h> /* err(), warnx() */
#include <stdlib.h> /* EXIT_FAILURE */
#include <string.h> /* memset() */
#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdbool.h>

struct accperms {
    bool admin;
    bool staff;
    bool manage_books;
    bool manage_stock;
    bool return_book;
    bool rent_book;
    bool inventory;
};

enum pg {
    PG_PUBLISHER,
    PG_AUTHOR,
    PG_ACTION,
    PG_LANG,
    PG_DOCTYPE,
    PG_CAMPUS,
    PG_ROLE,
    PG_ACCOUNT,
    PG_CATEGORY,
    PG_BOOK,
    PG_STOCK,
    PG_INVENTORY,
    PG_HISTORY,
    PG__MAX
};

enum key {
    KEY_PAGE_PUBLISHER,
    KEY_PAGE_AUTHOR,
    KEY_PAGE_ACTION,
    KEY_PAGE_LANG,
    KEY_PAGE_DOCTYPE,
    KEY_PAGE_CAMPUS,
    KEY_PAGE_ROLE,
    KEY_PAGE_ACCOUNT,
    KEY_PAGE_CATEGORY,
    KEY_PAGE_BOOK,
    KEY_PAGE_STOCK,
    KEY_PAGE_INVENTORY,
    KEY_PAGE_HISTORY,
    KEY_ROWID,
    KEY_UUID,
    KEY_TREE,
    KEY__MAX
};

static const char *pages[PG__MAX] = {
    "publisher", "author", "action", "lang", "doctype", "campus", "role", "account", "category", "book", "stock",
    "inventory", "history"
};
static const struct kvalid keys[KEY__MAX] = {
    {NULL, "publisher"},
    {NULL, "author"},
    {NULL, "action"},
    {NULL, "lang"},
    {NULL, "doctype"},
    {NULL, "campus"},
    {NULL, "role"},
    {NULL, "account"},
    {NULL, "category"},
    {NULL, "book"},
    {NULL, "stock"},
    {NULL, "inventory"},
    {NULL, "history"},
    {kvalid_int, "rowid"},
    {kvalid_int, "UUID"},
    {NULL, "tree"},
};

struct accperms int_to_accperms(int perm) {
    struct accperms perms = {
        .admin = (perm & (1 << 6)),
        .staff = (perm & (1 << 5)),
        .manage_books = (perm & (1 << 4)),
        .manage_stock = (perm & (1 << 3)),
        .return_book = (perm & (1 << 2)),
        .rent_book = (perm & (1 << 1)),
        .inventory = (perm & 1)
    };
    return perms;
}

void format_accperms(const struct accperms *perms, struct kjsonreq *req) {
    kjson_objp_open(req, "perms");
    kjson_putboolp(req, "admin", perms->admin);
    kjson_putboolp(req, "staff", perms->staff);
    kjson_putboolp(req, "manage_books", perms->manage_books);
    kjson_putboolp(req, "manage_stock", perms->manage_stock);
    kjson_putboolp(req, "return_book", perms->return_book);
    kjson_putboolp(req, "rent_book", perms->rent_book);
    kjson_putboolp(req, "inventory", perms->inventory);
    kjson_obj_close(req);
}

void format_category(const struct sqlbox_parmset *res, struct kjsonreq *req) {
    kjson_obj_open(req);
    kjson_putintp(req, "rowid", res->ps[0].iparm);
    kjson_putstringp(req, "categoryName", res->ps[1].sparm);
    kjson_putstringp(req, "categoryDesc", res->ps[2].sparm);
    kjson_obj_close(req);
}

void format_publisher(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "publisherName", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "publisherName", res->ps[0].sparm);
    }
}

void format_author(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "authorName", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "authorName", res->ps[0].sparm);
    }
}

void format_action(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "actionName", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "actionName", res->ps[0].sparm);
    }
}

void format_lang(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "langCode", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "langCode", res->ps[0].sparm);
    }
}

void format_doctype(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "typeName", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "typeName", res->ps[0].sparm);
    }
}

void format_campus(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "campusName", res->ps[1].sparm);
    } else {
        kjson_putstringp(req, "campusName", res->ps[0].sparm);
    }
}

void format_role(const struct sqlbox_parmset *res, struct kjsonreq *req, bool showrowid) {
    if (showrowid) {
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "roleName", res->ps[1].sparm);
        const struct accperms perms = int_to_accperms((int) res->ps[2].iparm);
        format_accperms(&perms, req);
    } else {
        kjson_putstringp(req, "roleName", res->ps[1].sparm);
        const struct accperms perms = int_to_accperms((int) res->ps[2].iparm);
        format_accperms(&perms, req);
    }
}

void format_account(const struct sqlbox_parmset *res, struct kjsonreq *req) {
    kjson_putintp(req, "UUID", res->ps[0].iparm);
    kjson_putstringp(req, "displayname", res->ps[1].sparm);
    kjson_putstringp(req, "passhash", res->ps[2].sparm); // TODO:REMOVE LATER
    kjson_objp_open(req, "campus");
    kjson_putintp(req, "campusID", res->ps[3].iparm);
    kjson_putstringp(req, "campusName", res->ps[5].sparm);
    kjson_obj_close(req);
    kjson_objp_open(req, "role");
    kjson_putintp(req, "roleID", res->ps[4].iparm);
    kjson_putstringp(req, "roleName", res->ps[6].sparm);
    const struct accperms perms = int_to_accperms((int) res->ps[2].iparm);
    format_accperms(&perms, req);
    kjson_obj_close(req);
}

void handle_err(struct kreq *r, struct kjsonreq *req, enum khttp status, int err) {
    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[status]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);

    khttp_head(r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_putintp(req, "status", err);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

void handle_simple(struct kreq *r, struct kjsonreq *req, const int rowid, enum pg page) {
    size_t dbid, stmtid;
    struct sqlbox *p2;
    struct sqlbox_cfg cfg;
    struct sqlbox_src srcs[] = {
        {
            .fname = (char *) "db/database.db",
            .mode = SQLBOX_SRC_RO
        }
    };
    struct sqlbox_pstmt pstmts[2];
    switch (page) {
        case PG_PUBLISHER:
            pstmts[0].stmt = (char *) "SELECT * FROM PUBLISHER WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM PUBLISHER";
            break;
        case PG_AUTHOR:
            pstmts[0].stmt = (char *) "SELECT * FROM AUTHOR WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM AUTHOR";
            break;
        case PG_ACTION:
            pstmts[0].stmt = (char *) "SELECT * FROM ACTION WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM ACTION";
            break;
        case PG_LANG:
            pstmts[0].stmt = (char *) "SELECT * FROM LANG WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM LANG";
            break;
        case PG_DOCTYPE:
            pstmts[0].stmt = (char *) "SELECT * FROM DOCTYPE WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM DOCTYPE";
            break;
        case PG_CAMPUS:
            pstmts[0].stmt = (char *) "SELECT * FROM CAMPUS WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM CAMPUS";
            break;
        case PG_ROLE:
            pstmts[0].stmt = (char *) "SELECT * FROM ROLE WHERE ROWID = (?)";
            pstmts[1].stmt = (char *) "SELECT ROWID,* FROM ROLE";
            break;
        case PG_ACCOUNT:
            pstmts[0].stmt = (char *)
                    "SELECT * FROM ACCOUNT,CAMPUS,ROLE WHERE roleID = ROLE.ROWID AND campusID = CAMPUS.ROWID AND UUID = (?)";
            pstmts[1].stmt = (char *)
                    "SELECT * FROM ACCOUNT,CAMPUS,ROLE WHERE roleID = ROLE.ROWID AND campusID = CAMPUS.ROWID";
            break;
        default:
            handle_err(r, req, KHTTP_400, 400);
            break;
    }
    struct sqlbox_parm parms[] = {
        {
            .iparm = rowid,
            .type = SQLBOX_PARM_INT
        },
    };
    const struct sqlbox_parmset *res;

    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = 2;
    cfg.stmts.stmts = pstmts;
    if ((p2 = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(p2, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");
    if (rowid > 0) {
        if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 0, 1, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    } else {
        if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 1, 0, 0, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    }

    if (rowid <= 0) {
        khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_head(r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_head(r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
        khttp_head(r, kresps[KRESP_VARY], "%s", "Origin");
        khttp_body(r);
        kjson_open(req, r);
        kjson_obj_open(req);
        kjson_arrayp_open(req, pages[page]);
        while ((res = sqlbox_step(p2, stmtid)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
            kjson_obj_open(req);
            switch (page) {
                case PG_PUBLISHER:
                    format_publisher(res, req,true);
                    break;
                case PG_AUTHOR:
                    format_author(res, req,true);
                    break;
                case PG_ACTION:
                    format_action(res, req,true);
                    break;
                case PG_LANG:
                    format_lang(res, req,true);
                    break;
                case PG_DOCTYPE:
                    format_doctype(res, req,true);
                    break;
                case PG_CAMPUS:
                    format_campus(res, req,true);
                    break;
                case PG_ROLE:
                    format_role(res, req,true);
                    break;
                case PG_ACCOUNT:
                    format_account(res, req);
                    break;
                default:
                    errx(EXIT_FAILURE, "format");
            }
            kjson_obj_close(req);
        }
        kjson_array_close(req);
        kjson_putintp(req, "status", 200);
    } else {
        if ((res = sqlbox_step(p2, stmtid)) == NULL)
            errx(EXIT_FAILURE, "sqlbox_step");
        if (res->psz != 0) {
            khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
            khttp_head(r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
            khttp_head(r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
            khttp_head(r, kresps[KRESP_VARY], "%s", "Origin");
            khttp_body(r);
            kjson_open(req, r);
            kjson_obj_open(req);
            switch (page) {
                case PG_PUBLISHER:
                    format_publisher(res, req,false);
                    break;
                case PG_AUTHOR:
                    format_author(res, req,false);
                    break;
                case PG_ACTION:
                    format_action(res, req,false);
                    break;
                case PG_LANG:
                    format_lang(res, req,false);
                    break;
                case PG_DOCTYPE:
                    format_doctype(res, req,false);
                    break;
                case PG_CAMPUS:
                    format_campus(res, req,false);
                    break;
                case PG_ROLE:
                    format_role(res, req,false);
                    break;
                case PG_ACCOUNT:
                    format_account(res, req);
                    break;
                default:
                    errx(EXIT_FAILURE, "format");
            }
            kjson_putintp(req, "status", 200);
        } else {
            khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_404]);
            khttp_head(r, kresps[KRESP_CONTENT_TYPE],
                       "%s", kmimetypes[KMIME_APP_JSON]);
            khttp_body(r);
            kjson_open(req, r);
            kjson_obj_open(req);
            kjson_putintp(req, "status", 404);
        }
    }
    if (!sqlbox_finalise(p2, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    sqlbox_free(p2);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

void get_flat_category(struct kreq *r, struct kjsonreq *req, struct sqlbox *p2, size_t dbid) {
    size_t stmtid;
    const struct sqlbox_parmset *res;
    if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 0, 0, 0, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(r);
    kjson_open(req, r);
    kjson_obj_open(req);
    kjson_arrayp_open(req, "categories");
    while ((res = sqlbox_step(p2, stmtid)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(req);
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "categoryName", res->ps[1].sparm);
        kjson_putstringp(req, "categoryDesc", res->ps[2].sparm);
        kjson_putintp(req, "parentCategoryID", res->ps[3].iparm);
        kjson_obj_close(req);
    }
    kjson_array_close(req);
    kjson_putintp(req, "status", 200);
    if (!sqlbox_finalise(p2, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    sqlbox_free(p2);
    kjson_obj_close(req);
    kjson_close(req);
    khttp_free(r);
}

void get_tree_category(struct kjsonreq *req, struct sqlbox *p2, size_t dbid, const int64_t rowid) {
    size_t stmtid;
    const struct sqlbox_parmset *res;
    if (rowid == 0) {
        if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 1, 0, 0, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    } else {
        struct sqlbox_parm parms[] = {
            {
                .iparm = rowid,
                .type = SQLBOX_PARM_INT
            },
        };
        if (!(stmtid = sqlbox_prepare_bind(p2, dbid, 3, 1, parms, 0)))
            errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    }
    while ((res = sqlbox_step(p2, stmtid)) != NULL && res->code == SQLBOX_CODE_OK && res->psz != 0) {
        kjson_obj_open(req);
        kjson_putintp(req, "rowid", res->ps[0].iparm);
        kjson_putstringp(req, "categoryName", res->ps[1].sparm);
        kjson_putstringp(req, "categoryDesc", res->ps[2].sparm);
        kjson_arrayp_open(req, "children");
        if (!sqlbox_finalise(p2, stmtid))
            errx(EXIT_FAILURE, "sqlbox_finalise");
        get_tree_category(req, p2, dbid, res->ps[0].iparm);
        kjson_array_close(req);
        kjson_obj_close(req);
    }
}

void handle_category(struct kreq *r, struct kjsonreq *req, const int rowid) {
    size_t dbid;
    struct sqlbox *p2;
    struct sqlbox_cfg cfg;
    struct sqlbox_src srcs[] = {
        {
            .fname = (char *) "db/database.db",
            .mode = SQLBOX_SRC_RO
        }
    };

    struct sqlbox_pstmt pstmts[4] = {
        {.stmt = (char *) "SELECT ROWID,* FROM CATEGORY"}, // Flat format
        {.stmt = (char *) "SELECT ROWID,* FROM CATEGORY WHERE parentCategoryID IS NULL "}, // Tree format
        {.stmt = (char *) "SELECT * FROM CATEGORY WHERE ROWID = (?)"},
        {.stmt = (char *) "SELECT ROWID,* FROM CATEGORY WHERE parentCategoryID = (?)"}
    };


    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = 4;
    cfg.stmts.stmts = pstmts;
    if ((p2 = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(p2, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");

    switch (rowid) {
        case -1:
            get_flat_category(r, req, p2, dbid);
            break;
        case -2:
            khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
            khttp_head(r, kresps[KRESP_CONTENT_TYPE],
                       "%s", kmimetypes[KMIME_APP_JSON]);
            khttp_head(r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
            khttp_head(r, kresps[KRESP_VARY], "%s", "Origin");
            khttp_body(r);
            kjson_open(req, r);
            kjson_obj_open(req);
            kjson_arrayp_open(req, "categories");
            get_tree_category(req, p2, dbid, 0);
            kjson_array_close(req);
            kjson_putintp(req, "status", 200);
            sqlbox_free(p2);
            kjson_obj_close(req);
            kjson_close(req);
            khttp_free(r);
            break;
        default:
            break;
    }
}

int main(void) {
    struct kreq r;
    struct kjsonreq req;
    struct kpair *rowid;
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG_BOOK) != KCGI_OK)
        return EXIT_FAILURE;
    if (r.method != KMETHOD_GET) {
        handle_err(&r, &req, KHTTP_405, 405);
    } else {
        switch (r.page) {
            case PG_PUBLISHER:
            case PG_AUTHOR:
            case PG_LANG:
            case PG_DOCTYPE:
            case PG_CAMPUS:
            case PG_ROLE:
                if ((rowid = r.fieldmap[(r.page == KEY_PAGE_ACCOUNT) ? KEY_UUID : KEY_ROWID]))
                    handle_simple(&r, &req, (int) rowid->parsed.i, r.page);
                else if (r.fieldnmap[(r.page == KEY_PAGE_ACCOUNT) ? KEY_UUID : KEY_ROWID])
                    handle_err(&r, &req, KHTTP_400, 400);
                else
                    handle_simple(&r, &req, -1, r.page);
                break;
            case PG_CATEGORY:
                if ((rowid = r.fieldmap[KEY_ROWID]))
                    handle_category(&r, &req, (int) rowid->parsed.i);
                else if (r.fieldnmap[KEY_ROWID])
                    handle_err(&r, &req, KHTTP_400, 400);
                else
                    handle_category(&r, &req, (r.fieldmap[KEY_TREE]) ? -2 : -1);
                break;

            default:
                // For httpd configurations that don't treat .cgi files well, and consider them folders
                if (r.fieldmap[KEY_PAGE_CATEGORY]) {
                    if ((rowid = r.fieldmap[KEY_ROWID]))
                        handle_category(&r, &req, (int) rowid->parsed.i);
                    else if (r.fieldnmap[KEY_ROWID])
                        handle_err(&r, &req, KHTTP_400, 400);
                    else
                        handle_category(&r, &req, (r.fieldmap[KEY_TREE]) ? -2 : -1);
                } else {
                    enum key i;
                    for (i = KEY_PAGE_PUBLISHER; i < KEY_PAGE_CATEGORY && !(r.fieldmap[i]); i++) {
                    }
                    if (i == KEY_PAGE_CATEGORY)
                        handle_err(&r, &req, KHTTP_403, 403);
                    else {
                        if ((rowid = r.fieldmap[(i == KEY_PAGE_ACCOUNT) ? KEY_UUID : KEY_ROWID]))
                            handle_simple(&r, &req, (int) rowid->parsed.i, (enum pg) i);
                        else if (r.fieldnmap[(i == KEY_PAGE_ACCOUNT) ? KEY_UUID : KEY_ROWID])
                            handle_err(&r, &req, KHTTP_400, 400);
                        else
                            handle_simple(&r, &req, -1, (enum pg) i);
                        break;
                    }
                }
                break;
        }
    }
    return EXIT_SUCCESS;
}
