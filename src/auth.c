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

enum cookie {
    COOKIE_UUID,
    COOKIE_SESSIONID,
    COOKIE__MAX
};

static const struct kvalid cookies[COOKIE__MAX] = {
    {kvalid_stringne, "userid"},
    {kvalid_stringne, "sessionid"},
};

int main() {
    struct kreq r;
    struct kjsonreq req;
    if (khttp_parse(&r, cookies, COOKIE__MAX, NULL, 0, 0) != KCGI_OK)
        return EXIT_FAILURE;

    if (r.cookiemap[COOKIE_UUID] == NULL || r.cookiemap[COOKIE_SESSIONID] == NULL) {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_head(&r, kresps[KRESP_SET_COOKIE],"userid=foo; sessionid=bar; Path=/");
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putstringp(&req, "cookie", "not found");
    } else {
        khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
        khttp_body(&r);
        kjson_open(&req, &r);
        kjson_obj_open(&req);
        kjson_putstringp(&req, "UUID", r.cookiemap[COOKIE_UUID]->parsed.s);
        kjson_putstringp(&req, "Session ID", r.cookiemap[COOKIE_SESSIONID]->parsed.s);
    }
    kjson_obj_close(&req);
    kjson_close(&req);
    khttp_free(&r);
}
