# NVCSV
**Command-line based CSV parser using CUDA.**

Forked from antonmks' [nvParse.](https://github.com/antonmks/nvParse) Developed to rapidly parse huge CSV files
harnessing the power of Nvidia's CUDA processing. Currently only supports Linux, but potential Windows support in the future.

**Compiling:** Run the makefile:
```bash
make
```

MySQL upload support (optional):

The project includes an optional uploader that writes the parsed column values to a temporary CSV and performs a fast bulk import into MySQL using LOAD DATA LOCAL INFILE. To enable and build this feature you must have the MySQL C client library (libmysqlclient) and headers installed on your system.

On Linux / WSL, build with:

```bash
make MYSQL_UPLOAD=1 LDFLAGS='-lmysqlclient'
```

Before running, set the following environment variables to instruct NVCSV where to upload the parsed column:

- MYSQL_HOST - MySQL host (required to trigger upload)
- MYSQL_USER - MySQL username
- MYSQL_PASS - MySQL password (optional; empty string allowed)
- MYSQL_DB - target database name
- MYSQL_TABLE - target table name
- MYSQL_COLUMN - target column name (single column import)

Example (Linux / WSL):

```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypass
export MYSQL_DB=mydb
export MYSQL_TABLE=mytable
export MYSQL_COLUMN=colname
./nvcsv big.csv 2 25
```

Notes:
- The uploader is intentionally optional and opt-in (via `MYSQL_UPLOAD=1`) to avoid adding a mandatory dependency for users who don't need MySQL.
- If you build without `MYSQL_UPLOAD=1`, the program will still run as before and simply won't attempt any upload.
- On Windows you can use WSL or MinGW/MSYS to build; linking details depend on how the MySQL client is installed.

