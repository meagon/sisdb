#include <sis_modules.h>

extern s_sis_modules sis_modules_memdb;
extern s_sis_modules sis_modules_sdcdb;
extern s_sis_modules sis_modules_sisdb;
extern s_sis_modules sis_modules_sisdb_client;
extern s_sis_modules sis_modules_sisdb_rsdb;
extern s_sis_modules sis_modules_sisdb_rsno;
extern s_sis_modules sis_modules_sisdb_server;
extern s_sis_modules sis_modules_sisdb_wlog;
extern s_sis_modules sis_modules_sisdb_wsdb;
extern s_sis_modules sis_modules_sisdb_wsno;

s_sis_modules *__modules[] = {
    &sis_modules_memdb,
    &sis_modules_sdcdb,
    &sis_modules_sisdb,
    &sis_modules_sisdb_client,
    &sis_modules_sisdb_rsdb,
    &sis_modules_sisdb_rsno,
    &sis_modules_sisdb_server,
    &sis_modules_sisdb_wlog,
    &sis_modules_sisdb_wsdb,
    &sis_modules_sisdb_wsno,
    0
  };

const char *__modules_name[] = {
    "memdb",
    "sdcdb",
    "sisdb",
    "sisdb_client",
    "sisdb_rsdb",
    "sisdb_rsno",
    "sisdb_server",
    "sisdb_wlog",
    "sisdb_wsdb",
    "sisdb_wsno",
    0
  };
