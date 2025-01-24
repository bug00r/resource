#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "resource_sqlite.h"
#include "defs.h"

EXTERN_BLOB(res_test_embed, db);

#ifndef DEBUG_LOG_ARGS
	#if debug != 0
		#define DEBUG_LOG_ARGS(fmt, ...) printf((fmt), __VA_ARGS__)
	#else
		#define DEBUG_LOG_ARGS(fmt, ...)
	#endif
#endif

#ifndef DEBUG_LOG
	#if debug != 0
		#define DEBUG_LOG(msg) printf((msg))
	#else
		#define DEBUG_LOG(msg)
	#endif
#endif

/*
int sqlite3_open_v2(
  const char *filename,   // Database filename (UTF-8)
  sqlite3 **ppDb,         // OUT: SQLite db handle
  int flags,              // Flags
  const char *zVfs        // Name of VFS module to use 
);
*/

static bool __resource_sqlite_opendb(const char *filename, sqlite3 **pDb, int flags)
{
    bool openOk = true;
    int rc;

    rc = sqlite3_open_v2(filename, pDb, flags, NULL);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*pDb));
      sqlite3_close(*pDb);
      openOk = false;
    }

    return openOk;
}

static int callback_one(void *pCounter, int argc, char **argv, char **azColName){
    int i;
    int *counter = (int*)pCounter;
    (*counter)++;

    DEBUG_LOG_ARGS("counter: %i\n", *counter);

    assert(argc == 3 );

    assert(strcmp("id",azColName[0]) == 0);
    assert(strcmp("key",azColName[1]) == 0);
    assert(strcmp("value",azColName[2]) == 0);

    //segfault by using this pointer 
    switch (*counter) 
    {
        case 1: assert(strcmp("1",argv[0]) == 0);
                assert(strcmp("key1",argv[1]) == 0);
                assert(strcmp("value1",argv[2]) == 0);
                break;
        case 2: assert(strcmp("2",argv[0]) == 0);
                assert(strcmp("key2",argv[1]) == 0);
                assert(strcmp("value2",argv[2]) == 0);
                break;
        case 3: assert(strcmp("3",argv[0]) == 0);
                assert(strcmp("key3",argv[1]) == 0);
                assert(strcmp("value3",argv[2]) == 0);
                break;
        case 4: assert(strcmp("4",argv[0]) == 0);
                assert(strcmp("key4",argv[1]) == 0);
                assert(strcmp("value4",argv[2]) == 0);
                break;
    }

    for(i=0; i<argc; i++){
        DEBUG_LOG_ARGS("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    DEBUG_LOG("\n");
    return 0;
}

static int callback_two(void *pCounter, int argc, char **argv, char **azColName){
    int i;
    int *counter = (int*)pCounter;
    (*counter)++;

    DEBUG_LOG_ARGS("counter: %i\n", *counter);

    assert(argc == 3 );

    assert(strcmp("id",azColName[0]) == 0);
    assert(strcmp("key",azColName[1]) == 0);
    assert(strcmp("value",azColName[2]) == 0);

    //segfault by using this pointer 
    switch (*counter) 
    {
        case 1: assert(strcmp("1",argv[0]) == 0);
                assert(strcmp("key5",argv[1]) == 0);
                assert(strcmp("value5",argv[2]) == 0);
                break;
        case 2: assert(strcmp("2",argv[0]) == 0);
                assert(strcmp("key6",argv[1]) == 0);
                assert(strcmp("value6",argv[2]) == 0);
                break;
        case 3: assert(strcmp("3",argv[0]) == 0);
                assert(strcmp("key7",argv[1]) == 0);
                assert(strcmp("value7",argv[2]) == 0);
                break;
        case 4: assert(strcmp("4",argv[0]) == 0);
                assert(strcmp("key8",argv[1]) == 0);
                assert(strcmp("value8",argv[2]) == 0);
                break;
    }

    for(i=0; i<argc; i++){
        DEBUG_LOG_ARGS("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    DEBUG_LOG("\n");
    return 0;
}

static bool __resource_sqlite_exec_stmt(sqlite3 *pDb,  const char *sql_stmt, int (*callback)(void*,int,char**,char**),
                                        void *callbackData)
{
    bool execStmtOk = true;
    char *zErrMsg = 0;
    int rc;
    int counter = 0;

    rc = sqlite3_exec(pDb, "SELECT * FROM test;", callback, (void*)&counter, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      sqlite3_close(pDb);
      execStmtOk = false;
    }

    return execStmtOk;
}

static void test_resource_sqlite_raw() {
	DEBUG_LOG_ARGS(">>> %s => %s\n", __FILE__, __func__);
	
  sqlite3 *db;

  assert( __resource_sqlite_opendb("res_test.db", &db, SQLITE_OPEN_READONLY) );

  int counter = 0;
  assert ( __resource_sqlite_exec_stmt(db, "SELECT * FROM test;", callback_one, (void*)&counter) );

  sqlite3_close(db);

	DEBUG_LOG("<<<\n");
}

/**
int sqlite3_deserialize(
  sqlite3 *db,            // The database connection 
  const char *zSchema,    // Which DB to reopen with the deserialization 
  unsigned char *pData,   // The serialized database content 
  sqlite3_int64 szDb,     // Number bytes in the deserialization 
  sqlite3_int64 szBuf,    // Total size of buffer pData[] 
  unsigned mFlags         // Zero or more SQLITE_DESERIALIZE_* flags 
);
 */
static void test_resource_sqlite_raw_embed() {
	DEBUG_LOG_ARGS(">>> %s => %s\n", __FILE__, __func__);
	
    sqlite3 *db;
    char *zErrMsg = 0;
    assert( __resource_sqlite_opendb("embedded", &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_MEMORY) );
    //&_binary_zip_resource_7z_start, (size_t)&_binary_zip_resource_7z_end - (size_t)&_binary_zip_resource_7z_start
    //res_test_embed_db
    size_t dbByteCnt = (size_t)&_binary_res_test_embed_db_end - (size_t)&_binary_res_test_embed_db_start;
    int rc = sqlite3_deserialize(db,"main", &_binary_res_test_embed_db_start,dbByteCnt, dbByteCnt, SQLITE_DESERIALIZE_READONLY);

    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL open embed error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      sqlite3_close(db);
      return;
    }

    assert(rc == SQLITE_OK);

    int counter = 0;
    assert ( __resource_sqlite_exec_stmt(db, "SELECT * FROM test;", callback_two, (void*)&counter) );

    sqlite3_close(db);

	DEBUG_LOG("<<<\n");
}


int 
main() 
{

	DEBUG_LOG(">> Start resource sqlite tests:\n");

  test_resource_sqlite_raw();

  test_resource_sqlite_raw_embed();
	
	DEBUG_LOG("<< end resource tests:\n");
	return 0;
}