#include "mysql_uploader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef MYSQL_UPLOAD
#include <mysql/mysql.h>

static std::string make_temp_filename() {
    std::string tmpl = "/tmp/nvcsv_mysql_XXXXXX.csv";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), 4); // keep .csv suffix
    if (fd == -1) return std::string();
    close(fd);
    return std::string(buf.data());
}

int upload_to_mysql_from_doubles(const char* host, const char* user,
                                  const char* pass, const char* db,
                                  const char* table, const char* column,
                                  const double* data, size_t n) {
    if (!host || !user || !db || !table || !column || (!data && n>0)) {
        fprintf(stderr, "mysql_uploader: invalid args\n");
        return -1;
    }

    std::string tmp = make_temp_filename();
    if (tmp.empty()) {
        fprintf(stderr, "mysql_uploader: failed to create temp file\n");
        return -1;
    }

    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) {
        fprintf(stderr, "mysql_uploader: fopen failed for %s\n", tmp.c_str());
        return -1;
    }

    // Write one value per line
    for (size_t i = 0; i < n; ++i) {
        // Use high precision but compact representation
        if (fprintf(f, "%0.10g\n", data[i]) < 0) {
            fclose(f);
            unlink(tmp.c_str());
            fprintf(stderr, "mysql_uploader: write failed\n");
            return -1;
        }
    }
    fflush(f);
    fclose(f);

    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_uploader: mysql_init failed\n");
        unlink(tmp.c_str());
        return -1;
    }

    // Enable LOCAL INFILE
    unsigned int local_infile = 1;
    mysql_options(conn, MYSQL_OPT_LOCAL_INFILE, (const char*)&local_infile);

    if (!mysql_real_connect(conn, host, user, pass, db, 0, NULL, 0)) {
        fprintf(stderr, "mysql_uploader: mysql_real_connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        unlink(tmp.c_str());
        return -1;
    }

    // Build LOAD DATA LOCAL INFILE query
    std::string query = "LOAD DATA LOCAL INFILE '" + tmp + "' INTO TABLE `" + std::string(table) + "` FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (`" + std::string(column) + "`);";

    if (mysql_query(conn, query.c_str())) {
        fprintf(stderr, "mysql_uploader: mysql_query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        unlink(tmp.c_str());
        return -1;
    }

    mysql_close(conn);
    unlink(tmp.c_str());
    return 0;
}

#else

int upload_to_mysql_from_doubles(const char* host, const char* user,
                                  const char* pass, const char* db,
                                  const char* table, const char* column,
                                  const double* data, size_t n) {
    (void)host; (void)user; (void)pass; (void)db; (void)table; (void)column; (void)data; (void)n;
    fprintf(stderr, "mysql_uploader: compiled without MYSQL_UPLOAD support. Rebuild with MYSQL_UPLOAD=1 and link libmysqlclient.\n");
    return -1;
}

#endif
