#include <err.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>
#include <stdio.h>

enum apis {
    PG_QUERY,
    PG_CREATE,
    PG_SET,
    PG__MAX
};

static const char *pages[PG__MAX] = {
    "query",
    "create",
    "set"
};
static const int SerialNumbers[5] = {
    12345,
    56753,
    9999,
    23434,
    345,
};
static const char *Titles[5] = {
    "Livre A",
    "Livre B",
    "Bim's sins",
    "50 ways to torture Bim",
    "The Anti Bim manifesto"
};
static const char *Authors[5] = {
    "Author A",
    "Author B",
    "Anti-Bim United",
    "Anti-Bim United",
    "Anti-Bim United"
};

int handle_book_query(struct kjsonreq *req) {
    kjson_arrayp_open(req, "books");
    for (int i = 0; i < 5; ++i) {
        kjson_obj_open(req);
        kjson_putintp(req, "SerialNumber", SerialNumbers[i]);
        kjson_putstringp(req, "Title", Titles[i]);
        kjson_putstringp(req, "Authors", Authors[i]);
        kjson_obj_close(req);
    }
    kjson_array_close(req);
    return 0;
}

int
main(void) {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, NULL, 0, pages, PG__MAX, PG_QUERY) != KCGI_OK)
        return EXIT_FAILURE;
    if (pledge("stdio", NULL) == -1)
        err(EXIT_FAILURE, "pledge");
    if (r.method != KMETHOD_GET && r.method != KMETHOD_HEAD) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_405]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putintp(&req, "code", 405);
        kjson_putstringp(&req, "details", khttps[KHTTP_405]);
        kjson_obj_close(&req);
        kjson_close(&req);
        khttp_free(&r);
    } else if (r.page == PG__MAX) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_404]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putintp(&req, "code", 404);
        kjson_putstringp(&req, "details", khttps[KHTTP_404]);
        kjson_obj_close(&req);
        kjson_close(&req);
        khttp_free(&r);
    } else {
        khttp_head(&r, kresps[KRESP_STATUS],
                   "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putintp(&req, "code", 200);
        kjson_putstringp(&req, "details", khttps[KHTTP_200]);
        kjson_putstringp(&req,"subpath", r.path);
        switch (r.page) {
            case PG_QUERY:
                handle_book_query(&req);
                break;
            default:
                break;
        }
        kjson_obj_close(&req);
        kjson_close(&req);
        khttp_free(&r);
    }
    return EXIT_SUCCESS;
}
