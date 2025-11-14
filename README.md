# NVCSV
**Command-line based CSV parser using CUDA.**

Forked from antonmks' [nvParse.](https://github.com/antonmks/nvParse) Developed to rapidly parse huge CSV files
harnessing the power of Nvidia's CUDA processing. Currently only supports Linux, but potential Windows support in the future.

**Compiling:** Run the makefile:
```bash
make
```

## Quick Start: CSV to MySQL Database

**NEW:** GPU-accelerated CSV insertion into MySQL!

We've added two methods to insert CSV data into your MySQL database:

### 1. NVCSV-MySQL (GPU-Accelerated) ⚡ **Recommended for large files**

The fastest way to insert data. Uses CUDA GPU acceleration for parsing and bulk insert.

**Build:**
```bash
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
```

**Run:**
```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypass
export MYSQL_DB=mydb
export MYSQL_TABLE=products
./nvcsv_mysql products.csv
```

**See `NVCSV_MYSQL_GUIDE.md` for full documentation.**

### 2. CPU-Based Upload (upload_products) 

Simpler, no CUDA required. Good for testing or small files.

**Build:**
```bash
g++ -o upload_products upload_products.cpp mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient
```

**Run:**
```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypass
export MYSQL_DB=mydb
export MYSQL_TABLE=products
./upload_products products.csv
```

## MySQL Upload Support (Optional)

The project includes an optional uploader for fast bulk import of CSV data into MySQL using LOAD DATA LOCAL INFILE.

### Installation & Build

**Requirements:**
- MySQL C client library (libmysqlclient) with development headers
- On Linux/Ubuntu: `sudo apt install libmysqlclient-dev`
- On macOS: `brew install mysql-client`

**Build NVCSV with MySQL uploader enabled:**

```bash
make MYSQL_UPLOAD=1 LDFLAGS='-lmysqlclient'
```

**Build the simple products uploader example:**

```bash
g++ -o upload_products upload_products.cpp mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient
```

### SQL Schema

Create the products table in your MySQL database. See `sql_schema.sql` for the full schema:

```sql
CREATE TABLE IF NOT EXISTS products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    sku VARCHAR(100) NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

Or import directly:
```bash
mysql -u youruser -p yourdb < sql_schema.sql
```

### Usage Example: Upload products.csv to MySQL

**Step 1:** Create the table in your database

```bash
mysql -h 127.0.0.1 -u myuser -p mydb < sql_schema.sql
```

**Step 2:** Set environment variables and run the uploader

Linux / WSL / macOS:
```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypass
export MYSQL_DB=mydb
export MYSQL_TABLE=products
./upload_products products.csv
```

PowerShell (Windows):
```powershell
$env:MYSQL_HOST   = '127.0.0.1'
$env:MYSQL_USER   = 'myuser'
$env:MYSQL_PASS   = 'mypass'
$env:MYSQL_DB     = 'mydb'
$env:MYSQL_TABLE  = 'products'
.\upload_products.exe products.csv
```

**Step 3:** Verify upload

```bash
mysql -h 127.0.0.1 -u myuser -p mydb -e "SELECT COUNT(*) FROM products;"
```

### Notes

- The uploader is opt-in (via `MYSQL_UPLOAD=1`) to avoid mandatory MySQL dependency for users who don't need it.
- Uses `LOAD DATA LOCAL INFILE` for maximum throughput (single-phase load).
- Automatically skips the CSV header row (IGNORE 1 ROWS).
- Supports multi-column CSV import with proper field/line delimiters.
- For backward compatibility, the legacy single-column `upload_to_mysql_from_doubles()` function is also available.

