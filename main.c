#include <err.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgijson.h>
#include <sqlbox.h>

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

int
main(void) {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, NULL, 0, pages, PG__MAX, PG_QUERY) != KCGI_OK)
        return EXIT_FAILURE;

    if (pledge("stdio", NULL) == -1)
        err(EXIT_FAILURE, "pledge");
    if (r.mime == KMIME_APP_JSON) {
        khttp_head(&r, kresps[KRESP_STATUS],
                   "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
                   "%s", kmimetypes[KMIME_APP_JSON]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_objp_open(&req, "me_fr");
        kjson_putstringp(&req, "gate", "keeping");
        kjson_putstringp(&req, "gas", "lighting");
        kjson_putstringp(&req, "girl", "bossing");
        kjson_obj_close(&req);
        kjson_obj_close(&req);
        kjson_close(&req);
        khttp_free(&r);
    }
    return EXIT_SUCCESS;
}
