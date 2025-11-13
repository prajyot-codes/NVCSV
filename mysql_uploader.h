// Minimal MySQL uploader header
#ifndef MYSQL_UPLOADER_H
#define MYSQL_UPLOADER_H

#include <cstddef>

// Upload an array of doubles to the specified table/column. Returns 0 on success.
// This function is only available when compiled with -DMYSQL_UPLOAD and linked
// against libmysqlclient. If not compiled in, the function returns -1.
extern "C" int upload_to_mysql_from_doubles(const char* host, const char* user,
                                            const char* pass, const char* db,
                                            const char* table, const char* column,
                                            const double* data, size_t n);

#endif // MYSQL_UPLOADER_H
