/**
 * upload_products.cpp
 * Simple example: read products.csv and upload to MySQL
 * 
 * Build with:
 *   g++ -o upload_products upload_products.cpp mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient
 * 
 * Run with environment variables set:
 *   export MYSQL_HOST=127.0.0.1
 *   export MYSQL_USER=myuser
 *   export MYSQL_PASS=mypass
 *   export MYSQL_DB=mydb
 *   export MYSQL_TABLE=products
 *   ./upload_products products.csv
 */

#include "mysql_uploader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv_file>\n", argv[0]);
        fprintf(stderr, "Environment variables required:\n");
        fprintf(stderr, "  MYSQL_HOST   - MySQL host\n");
        fprintf(stderr, "  MYSQL_USER   - MySQL username\n");
        fprintf(stderr, "  MYSQL_PASS   - MySQL password (optional)\n");
        fprintf(stderr, "  MYSQL_DB     - Database name\n");
        fprintf(stderr, "  MYSQL_TABLE  - Table name\n");
        return 1;
    }

    const char* csv_file = argv[1];
    
    // Check environment variables
    const char* mysql_host = getenv("MYSQL_HOST");
    const char* mysql_user = getenv("MYSQL_USER");
    const char* mysql_pass = getenv("MYSQL_PASS");
    const char* mysql_db = getenv("MYSQL_DB");
    const char* mysql_table = getenv("MYSQL_TABLE");

    if (!mysql_host || !mysql_user || !mysql_db || !mysql_table) {
        fprintf(stderr, "Error: Missing required environment variables.\n");
        fprintf(stderr, "Required: MYSQL_HOST, MYSQL_USER, MYSQL_DB, MYSQL_TABLE\n");
        return 1;
    }

    // Read CSV file into memory
    FILE* f = fopen(csv_file, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        fprintf(stderr, "Error: File is empty or unreadable\n");
        fclose(f);
        return 1;
    }

    // Allocate buffer and read file
    char* csv_data = (char*)malloc(file_size);
    if (!csv_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(f);
        return 1;
    }

    size_t bytes_read = fread(csv_data, 1, file_size, f);
    fclose(f);

    if (bytes_read != file_size) {
        fprintf(stderr, "Error: Failed to read entire file\n");
        free(csv_data);
        return 1;
    }

    fprintf(stdout, "Read %ld bytes from %s\n", file_size, csv_file);
    fprintf(stdout, "Uploading to MySQL host=%s user=%s db=%s table=%s...\n",
            mysql_host, mysql_user, mysql_db, mysql_table);

    // Upload to MySQL
    int rc = upload_csv_to_mysql(mysql_host, mysql_user, mysql_pass ? mysql_pass : "",
                                 mysql_db, mysql_table, csv_data, bytes_read);

    free(csv_data);

    if (rc == 0) {
        fprintf(stdout, "Upload successful!\n");
        return 0;
    } else {
        fprintf(stderr, "Upload failed!\n");
        return 1;
    }
}
