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

enum key {
    KEY_CAT,
    KEY__MAX
};

static const struct kvalid keys[KEY__MAX] = {
    {kvalid_string, "filter"},
};

int main(void) {
    struct kreq r;
    struct kjsonreq req;
    struct kpair *rowid;
    if (khttp_parse(&r, keys, KEY__MAX, 0, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;
    size_t dbid, stmtid;
    struct sqlbox *boxctx;
    struct sqlbox_cfg cfg;
    struct sqlbox_src srcs[] = {
        {
            .fname = (char *) "db/database.db",
            .mode = SQLBOX_SRC_RO
        }
    };
    struct sqlbox_pstmt pstmts[] = {
        {.stmt = (char *) "SELECT * FROM CATEGORY WHERE categoryName = (?)"},
    };

    memset(&cfg, 0, sizeof(struct sqlbox_cfg));
    cfg.msg.func_short = warnx;
    cfg.srcs.srcsz = 1;
    cfg.srcs.srcs = srcs;
    cfg.stmts.stmtsz = 0;
    cfg.stmts.stmts = pstmts;
    if ((boxctx = sqlbox_alloc(&cfg)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_alloc");
    if (!(dbid = sqlbox_open(boxctx, 0)))
        errx(EXIT_FAILURE, "sqlbox_open");

    const struct sqlbox_parmset *res;
    struct sqlbox_parm parms[] = {
        {
            .sparm = (char *) "Biblio\0",
            .type = SQLBOX_PARM_STRING
        },
    };
    if (!(stmtid = sqlbox_prepare_bind(boxctx, dbid, 0, 1, parms, 0)))
        errx(EXIT_FAILURE, "sqlbox_prepare_bind");
    if ((res = sqlbox_step(boxctx, stmtid)) == NULL)
        errx(EXIT_FAILURE, "sqlbox_step");
    khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
               "%s", kmimetypes[KMIME_APP_JSON]);
    khttp_head(&r, kresps[KRESP_ACCESS_CONTROL_ALLOW_ORIGIN], "%s", "*");
    khttp_head(&r, kresps[KRESP_VARY], "%s", "Origin");
    khttp_body(&r);
    kjson_open(&req, &r);
    kjson_obj_open(&req);
    if (res->psz != 0) {
        kjson_putstringp(&req, "categoryName", res->ps[0].sparm);
        kjson_putintp(&req, "parentcategoryID", res->ps[1].iparm);
    }
    kjson_putintp(&req, "status", 200);
    if (!sqlbox_finalise(boxctx, stmtid))
        errx(EXIT_FAILURE, "sqlbox_finalise");
    sqlbox_free(boxctx);
    kjson_obj_close(&req);
    kjson_close(&req);
    khttp_free(&r);
    return EXIT_SUCCESS;
}
