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

enum key {
    KEY_PG_PUBLISHER, //
    KEY_PG_AUTHOR, //
    KEY_PG_LANG, //
    KEY_PG_ACTION, //
    KEY_PG_DOCTYPE, //
    KEY_PG_CAMPUS, //
    KEY_PG_ROLE,
    KEY_PG_CATEGORY, //
    KEY_PG_ACCOUNT,
    KEY_PG_BOOK, //
    KEY_PG_LANGUAGES, //
    KEY_PG_AUTHORED, //
    KEY_PG_STOCK, //
    KEY_PG_INVENTORY, //
};

enum key_cookie {
    COOKIE_SESSIONID = KEY_PG_INVENTORY + 1,
};

enum key_sels {
    KEY_SEL_PUBLISHERNAME = COOKIE_SESSIONID + 1,
    KEY_SEL_AUTHORNAME,
    KEY_SEL_ACTIONAME,
    KEY_SEL_LANGCODE,
    KEY_SEL_TYPENAME,
    KEY_SEL_CAMPUSNAME,
    KEY_SEL_ROLENAME,
    KEY_SEL_CATEGORYCLASS,
    KEY_SEL_UUID,
    KEY_SEL_SERIALNUM,
};

enum key_mods {
    KEY_MOD_NAME = KEY_SEL_SERIALNUM + 1,
    KEY_MOD_PERMS,
    KEY_MOD_CLASS,
    KEY_MOD_PARENT,
    KEY_MOD_UUID,
    KEY_MOD_PW,
    KEY_MOD_CAMPUS,
    KEY_MOD_ROLE,
    KEY_MOD_FROZEN,
    KEY_MOD_SERIALNUM,
    KEY_MOD_TYPE,
    KEY_MOD_CATEGORY,
    KEY_MOD_PUBLISHER,
    KEY_MOD_YEAR,
    KEY_MOD_COVER,
    KEY_MOD_HITS,
    KEY_MOD_LANG,
    KEY_MOD_AUTHOR,
    KEY_MOD_STOCK,
    KEY_MOD_DURATION,
    KEY_MOD_DATE,
    KEY_MOD_EXTENDED,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {NULL, "publisher"},
    {NULL, "author"},
    {NULL, "lang"},
    {NULL, "action"},
    {NULL, "doctype"},
    {NULL, "campus"},
    {NULL, "role"},
    {NULL, "category"},
    {NULL, "account"},
    {NULL, "book"},
    {NULL, "languages"},
    {NULL, "authored"},
    {NULL, "stock"},
    {NULL, "inventory"},
    {kvalid_stringne, "sessionID"},
    {kvalid_stringne, "select_publishername"},
    {kvalid_stringne, "select_authorname"},
    {kvalid_stringne, "select_actioname"},
    {kvalid_stringne, "select_langcode"},
    {kvalid_stringne, "select_typename"},
    {kvalid_stringne, "select_campusname"},
    {kvalid_stringne, "select_rolename"},
    {kvalid_stringne, "select_categoryclass"},
    {kvalid_stringne, "select_uuid"},
    {kvalid_stringne, "select_serialnum"},
    {kvalid_stringne, "mod_name"},
    {kvalid_int, "mod_perms"},
    {kvalid_stringne, "mod_class"},
    {kvalid_stringne, "mod_parent"},
    {kvalid_stringne, "mod_uuid"},
    {kvalid_stringne, "mod_pw"},
    {kvalid_stringne, "mod_campus"},
    {kvalid_stringne, "mod_role"},
    {kvalid_int, "mod_frozen"},
    {kvalid_stringne, "mod_serialnum"},
    {kvalid_stringne, "mod_type"},
    {kvalid_stringne, "mod_category"},
    {kvalid_stringne, "mod_publisher"},
    {kvalid_int, "mod_year"},
    {kvalid_string, "mod_cover"},
    {kvalid_int, "mod_hits"},
    {kvalid_stringne, "mod_lang"},
    {kvalid_stringne, "mod_author"},
    {kvalid_int, "mod_stock"},
    {kvalid_int, "mod_duration"},
    {kvalid_date, "mod_date"},
    {kvalid_int, "mod_extended"},
};

enum statement_comp {
    STMTS_PUBLISHER,
    STMTS_AUTHOR,
    STMTS_LANG,
    STMTS_ACTION,
    STMTS_TYPE,
    STMTS_CAMPUS,
    STMTS_ROLE,
    STMTS_CATEGORY,
    STMTS_ACCOUNT,
    STMTS_BOOK,
    STMTS_LANGUAGES,
    STMTS_AUTHORED,
    STMTS_STOCK,
    STMTS_INVENTORY,
    STMTS__MAX
};

static const char *statement_string[STMTS__MAX] = {
    "publisher", "author", "lang", "action", "type", "campus", "role", "category", "account",
    "book", "booklang", "bookauthor", "stock", "inventory"
};
static struct sqlbox_pstmt pstmts_top[STMTS__MAX] = {
    {(char *) "UPDATE PUBLISHER SET "},
    {(char *) "UPDATE AUTHOR SET "},
    {(char *) "UPDATE ACTION SET "},
    {(char *) "UPDATE LANG SET "},
    {(char *) "UPDATE DOCTYPE SET "},
    {(char *) "UPDATE CAMPUS SET "},
    {(char *) "UPDATE ROLE SET "},
    {(char *) "UPDATE CATEGORY SET "},
    {(char *) "UPDATE ACCOUNT SET "},
    {(char *) "UPDATE BOOK SET "},
    {(char *) "UPDATE LANGUAGES SET "},
    {(char *) "UPDATE AUTHORED SET "},
    {(char *) "UPDATE STOCK SET "},
    {(char *) "UPDATE INVENTORY SET "},

};


static struct sqlbox_pstmt pstmts_switches[STMTS__MAX][8] = {
    {{(char *) "publisherName = (?)"}},
    {{(char *) "authorName = (?)"}},
    {{(char *) "actionName = (?)"}},
    {{(char *) "langcode = (?)"}},
    {{(char *) "typeName = (?)"}},
    {{(char *) "campusName = (?)"}},
    {{(char *) "roleName = (?)"}, {(char *) "perms = (?)"}},
    {{(char *) "categoryClass = (?)"}, {(char *) "categoryName = (?)"}, {(char *) "parentCategoryID = (?)"}},
    {
        {(char *) "UUID = (?)"}, {(char *) "displayname = (?)"}, {(char *) "pwhash = (?)"},
        {(char *) "campus = (?)"}, {(char *) "role = (?)"}, {(char *) "frozen = (?)"}
    },
    {
        {(char *) "serialnum = (?)"}, {(char *) "type = (?)"}, {(char *) "category = (?)"},
        {(char *) "publisher = (?)"}, {(char *) "booktitle = (?)"}, {(char *) "bookreleaseyear = (?)"},
        {(char *) "bookcover = (?)"}, {(char *) "hits = (?)"}
    },
    {{(char *) "serialnum = (?)"}, {(char *) "lang = (?)"}},
    {{(char *) "serialnum = (?)"}, {(char *) "author = (?)"}},
    {{(char *) "serialnum = (?)"}, {(char *) "campus = (?)"}, {(char *) "instock = (?)"}},
    {
        {(char *) "UUID = (?)"}, {(char *) "serialnum = (?)"}, {(char *) "rentduration = (?)"},
        {(char *) "rentdate = (?)"}, {(char *) "extended = (?)"}
    }
};
static enum key_mods switch_keys[STMTS__MAX][9] = {
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY__MAX},
    {KEY_MOD_NAME, KEY_MOD_PERMS, KEY__MAX},
    {KEY_MOD_CLASS, KEY_MOD_NAME, KEY_MOD_PARENT, KEY__MAX},
    {KEY_MOD_UUID, KEY_MOD_NAME, KEY_MOD_PW, KEY_MOD_CAMPUS, KEY_MOD_ROLE, KEY_MOD_FROZEN, KEY__MAX},
    {
        KEY_MOD_SERIALNUM, KEY_MOD_TYPE, KEY_MOD_CATEGORY, KEY_MOD_PUBLISHER, KEY_MOD_NAME, KEY_MOD_YEAR, KEY_MOD_COVER,
        KEY_MOD_HITS, KEY__MAX
    },
    {KEY_MOD_SERIALNUM, KEY_MOD_LANG, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_MOD_AUTHOR, KEY__MAX},
    {KEY_MOD_SERIALNUM, KEY_MOD_CAMPUS, KEY_MOD_STOCK, KEY__MAX},
    {KEY_MOD_UUID, KEY_MOD_SERIALNUM, KEY_MOD_DURATION, KEY_MOD_DATE, KEY_MOD_EXTENDED, KEY__MAX}
};
static struct sqlbox_pstmt pstmts_bottom[STMTS__MAX] = {
    {(char *) "WHERE publisherName = (?)"},
    {(char *) "WHERE authorName = (?)"},
    {(char *) "WHERE actionName = (?)"},
    {(char *) "WHERE langcode = (?)"},
    {(char *) "WHERE typeName = (?)"},
    {(char *) "WHERE campusName = (?)"},
    {(char *) "WHERE roleName = (?)"},
    {(char *) "WHERE categoryClass = (?)"},
    {(char *) "WHERE UUID = (?)"},
    {(char *) "WHERE serialnum = (?)"},
    {(char *) "WHERE serialnum = (?) AND lang = (?)"},
    {(char *) "WHERE serialnum = (?) AND author = (?)"},
    {(char *) "WHERE serialnum = (?) AND campus = (?)"},
    {(char *) "WHERE UUID = (?) AND serialnum = (?)"}
};


static enum key_sels bottom_keys[STMTS__MAX][3] = {
    {KEY_SEL_PUBLISHERNAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_AUTHORNAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_ACTIONAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_LANGCODE, (enum key_sels) KEY__MAX},
    {KEY_SEL_TYPENAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_CAMPUSNAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_ROLENAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_CATEGORYCLASS, (enum key_sels) KEY__MAX},
    {KEY_SEL_UUID, (enum key_sels) KEY__MAX},
    {KEY_SEL_SERIALNUM, (enum key_sels) KEY__MAX},
    {KEY_SEL_SERIALNUM, KEY_SEL_LANGCODE, (enum key_sels) KEY__MAX},
    {KEY_SEL_SERIALNUM, KEY_SEL_AUTHORNAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_SERIALNUM, KEY_SEL_CAMPUSNAME, (enum key_sels) KEY__MAX},
    {KEY_SEL_UUID, KEY_SEL_SERIALNUM, (enum key_sels) KEY__MAX}
};

enum statement {
    STMT_EDIT,
    __STMT_SAVE__,
    __STMT_LOGIN__,
    STMT__REAL__MAX
};

static struct sqlbox_pstmt pstmts[STMT__REAL__MAX] = {
    {NULL},
    {
        (char *)
        "INSERT INTO HISTORY (UUID, IP, action, actiondate, details) "
        "VALUES ((?),(?),'EDIT',datetime('now','localtime'),(?))"
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
    cfg_data.stmts.stmtsz = STMT__REAL__MAX;
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
    if ((field = r.cookiemap[COOKIE_SESSIONID])) {
        size_t stmtid;
        size_t parmsz = 1;
        const struct sqlbox_parmset *res;
        struct sqlbox_parm parms[] = {
            {.type = SQLBOX_PARM_STRING, .sparm = field->parsed.s},
        };
        if (!(stmtid = sqlbox_prepare_bind(boxctx_data, dbid_data, __STMT_LOGIN__, parmsz, parms, 0)))
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
    if (r.method != KMETHOD_GET) //TODO: SET TO PUT
        return KHTTP_405;
    if (r.page == PG__MAX) {
        enum key i;
        for (i = KEY_PG_PUBLISHER; i < COOKIE_SESSIONID && !(r.fieldmap[i]); i++) {
        }
        if (i == COOKIE_SESSIONID)
            return KHTTP_404;
    }
    return KHTTP_200;
}

enum khttp second_pass(enum statement_comp STMT) {
    for (int i = 0; bottom_keys[STMT][i] != KEY__MAX; ++i) {
        if (!(r.fieldmap[bottom_keys[STMT][i]]))
            return KHTTP_400;
    }
    return KHTTP_200;
}

enum khttp third_pass(enum statement_comp STMT) {
    kasprintf(&pstmts[STMT_EDIT].stmt, "%s", pstmts_top[STMT].stmt);
    bool found = false;
    if ((found = (r.fieldmap[switch_keys[STMT][0]])))
        kasprintf(&pstmts[STMT_EDIT].stmt, "%s%s", pstmts[STMT_EDIT].stmt, pstmts_switches[STMT][0].stmt);
    for (int i = 1; switch_keys[STMT][i] != KEY__MAX; ++i) {
        if (r.fieldmap[switch_keys[STMT][i]]) {
            found = true;
            kasprintf(&pstmts[STMT_EDIT].stmt, "%s,%s", pstmts[STMT_EDIT].stmt, pstmts_switches[STMT][i].stmt);
        }
    }
    kasprintf(&pstmts[STMT_EDIT].stmt, "%s %s", pstmts[STMT_EDIT].stmt, pstmts_bottom[STMT].stmt);
    return (found) ? KHTTP_200 : KHTTP_400;
}

enum khttp forth_pass(enum statement_comp STMT) {
    if (!curr_usr.authenticated)
        return KHTTP_403;
    if (STMT == STMTS_ACTION && curr_usr.perms.admin)
        return KHTTP_200;
    if (STMT == STMTS_CAMPUS && (curr_usr.perms.admin || curr_usr.perms.staff))
        return KHTTP_200;
    if (STMT == STMTS_CATEGORY || STMT == STMTS_TYPE || (STMT <= STMTS_LANG) || (
            STMT >= STMTS_BOOK && STMT <= STMTS_STOCK) && (
            curr_usr.perms.manage_stock || curr_usr.perms.admin || curr_usr.perms.staff))
        return KHTTP_200;
    if (STMT == STMTS_ROLE && curr_usr.perms.admin)
        return KHTTP_200;
    if (STMT == STMTS_INVENTORY && (
            curr_usr.perms.manage_inventories || curr_usr.perms.admin || curr_usr.perms.staff))
        return KHTTP_200;
    if (STMT == STMTS_ACCOUNT && (curr_usr.perms.admin || curr_usr.perms.staff) && !r.fieldmap[KEY_MOD_ROLE] && strcmp(
            r.fieldmap[KEY_SEL_UUID]->parsed.s, curr_usr.UUID) != 0)
        return KHTTP_200;

    if (STMT == STMTS_ACCOUNT && !r.fieldmap[KEY_MOD_ROLE] && strcmp(
            r.fieldmap[KEY_SEL_UUID]->parsed.s, curr_usr.UUID) == 0)
        return KHTTP_200;


    return KHTTP_403;
}

enum statement_comp get_stmts() {
    if (r.page != PG__MAX)
        return (enum statement_comp) r.page;
    enum key i;
    for (i = KEY_PG_PUBLISHER; i < COOKIE_SESSIONID && !(r.fieldmap[i]); i++) {
    }
    return (enum statement_comp) i;
}

int main() {
    enum khttp er;
    // Parse the http request and match the keys to the keys, and pages to the pages, default to
    // querying the PG__MAX if no page was found
    if (khttp_parse(&r, keys, KEY__MAX, pages, PG__MAX, PG__MAX) != KCGI_OK)
        return EXIT_FAILURE;
    if ((er = sanitize()) != KHTTP_200) goto error;
    const enum statement_comp STMT = get_stmts();
    if ((er = second_pass(STMT)) != KHTTP_200) goto error;
    if ((er = third_pass(STMT)) != KHTTP_200) goto error;
    alloc_ctx_cfg();
    fill_user();
    if ((er = forth_pass(STMT)) != KHTTP_200) goto access_denied;
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
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
    kjson_putstringp(&req, "string", pstmts[STMT_EDIT].stmt);
    kjson_obj_close(&req);
    kjson_close(&req);
    goto cleanup;
access_denied:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    kjson_putstringp(&req, "IP", r.remote);
    kjson_putboolp(&req, "authenticated", curr_usr.authenticated);
    kjson_putstringp(&req, "error", "You don't have the permissions to edit this ressource");
    kjson_obj_close(&req);
    kjson_close(&req);
cleanup:
    khttp_free(&r);
    sqlbox_free(boxctx_data);
    return 0;
error:
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[er]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    if (r.mime == KMIME_TEXT_HTML)
        khttp_puts(&r, "Could not service request");
    khttp_free(&r);
    return 0;
}
