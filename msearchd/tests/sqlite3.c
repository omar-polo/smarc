/* public domain */

#include <stdlib.h>
#include <sqlite3.h>

int
main(void)
{
	sqlite3 *db;

	sqlite3_open_v2("dummy", &db, SQLITE_OPEN_READONLY, NULL);
	sqlite3_close(db);

	return (0);
}
