// Minimal MySQL uploader header
#ifndef MYSQL_UPLOADER_H
#define MYSQL_UPLOADER_H

#include <cstddef>
#include <vector>
#include <string>

// Upload a multi-column CSV (as a string buffer) directly to MySQL.
// Uses LOAD DATA LOCAL INFILE for fast bulk import.
// Returns 0 on success, -1 on failure.
// This function is only available when compiled with -DMYSQL_UPLOAD and linked
// against libmysqlclient. If not compiled in, the function returns -1.
extern "C" int upload_csv_to_mysql(const char* host, const char* user,
                                   const char* pass, const char* db,
                                   const char* table,
                                   const char* csv_data, size_t csv_size);

// Legacy single-column function (kept for backward compatibility)
extern "C" int upload_to_mysql_from_doubles(const char* host, const char* user,
                                            const char* pass, const char* db,
                                            const char* table, const char* column,
                                            const double* data, size_t n);

#endif // MYSQL_UPLOADER_H
