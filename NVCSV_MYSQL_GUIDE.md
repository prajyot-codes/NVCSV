# NVCSV-MySQL: GPU-Accelerated CSV to Database Insertion

A complete CUDA-based solution that reads CSV files using GPU acceleration and inserts all data into a pre-existing MySQL table with maximum throughput.

## Overview

This program:
1. **Reads** the CSV file using fast I/O
2. **Transfers** file data to GPU memory
3. **Parses** all rows and columns (leveraging GPU for line counting and data analysis)
4. **Formats** data as CSV for MySQL bulk import
5. **Inserts** all rows into your MySQL table using `LOAD DATA LOCAL INFILE` (fastest method)

## Prerequisites

### System Requirements
- NVIDIA GPU with CUDA compute capability 3.0+
- CUDA Toolkit 9.0 or newer
- MySQL Server (5.7 or 8.0+)
- MySQL C client library (libmysqlclient)
- C++ compiler with C++11 support

### Installation

**Linux/Ubuntu:**
```bash
sudo apt update
sudo apt install build-essential nvidia-cuda-toolkit libmysqlclient-dev
```

**macOS (if using CUDA):**
```bash
brew install mysql-client cuda
```

**Windows (WSL recommended):**
Use WSL2 with Ubuntu and follow Linux instructions above.

## Preparation: Create Your MySQL Table

Before running the program, create your table. For `products.csv`:

```sql
CREATE TABLE products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    sku VARCHAR(100) NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

Adjust column types based on your actual CSV structure.

## Building

### Build NVCSV-MySQL

```bash
# Standard build with MySQL support
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql

# Or with more optimization flags
nvcc -O3 -Xptxas -v -use_fast_math -arch=sm_60 nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
```

Replace `-arch=sm_60` with your GPU's compute capability:
- RTX 30/40 series: `-arch=sm_86`
- RTX 20 series: `-arch=sm_75`
- GTX 10 series: `-arch=sm_61`
- Older GPUs: `-arch=sm_35` or `-arch=sm_30`

### Verify Build

```bash
./nvcsv_mysql --help
# (or just run with no args to see usage)
```

## Usage

### Step 1: Set Environment Variables

**Linux/WSL/macOS (Bash):**
```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypassword
export MYSQL_DB=mydatabase
export MYSQL_TABLE=products
```

**PowerShell (Windows):**
```powershell
$env:MYSQL_HOST   = '127.0.0.1'
$env:MYSQL_USER   = 'myuser'
$env:MYSQL_PASS   = 'mypassword'
$env:MYSQL_DB     = 'mydatabase'
$env:MYSQL_TABLE  = 'products'
```

**Notes on Environment Variables:**
- `MYSQL_HOST` – IP or hostname of MySQL server (required)
- `MYSQL_USER` – MySQL username (required)
- `MYSQL_PASS` – MySQL password (optional; leave blank for no password)
- `MYSQL_DB` – Database name (required)
- `MYSQL_TABLE` – Table name (required)

### Step 2: Run the Program

```bash
./nvcsv_mysql products.csv
```

### Example Output

```
NVCSV-MySQL Version 1.0
Initializing CUDA context...
Reading CSV file: products.csv
File size: 52428800 bytes
Copying file to GPU...
Counting lines...
Total lines: 1000001
Finding line boundaries...
CSV has 3 fields
Parsing CSV data...
Parsed 1000000 data rows
Preparing data for MySQL insertion...
CSV output size: 57482940 bytes
Uploading to MySQL...
  Host:  127.0.0.1
  User:  myuser
  DB:    mydatabase
  Table: products
mysql_uploader: inserted 1000000 rows
SUCCESS: Data inserted into MySQL table!
```

## Verification

After successful insertion, verify your data in MySQL:

```bash
mysql -h 127.0.0.1 -u myuser -p mydatabase
```

```sql
SELECT COUNT(*) FROM products;
SELECT * FROM products LIMIT 5;
```

## Performance Tuning

### GPU Memory Optimization

For very large files (> 2GB), adjust the program:
- The program currently loads entire file into GPU memory
- For files larger than GPU VRAM, process in chunks (modifications needed)

### MySQL Configuration

For fastest insertion, ensure MySQL is optimized:

```sql
-- Check current settings
SHOW VARIABLES LIKE 'max_allowed_packet';
SHOW VARIABLES LIKE 'local_infile';

-- Increase max_allowed_packet if needed (my.cnf or MySQL config)
SET GLOBAL max_allowed_packet = 1073741824;  -- 1GB

-- Enable LOCAL INFILE (if disabled)
SET GLOBAL local_infile = 1;
```

### Multiple Batch Uploads

For multiple CSV files, the program reuses the MySQL connection:

```bash
for file in batch1.csv batch2.csv batch3.csv; do
    ./nvcsv_mysql "$file"
done
```

## Troubleshooting

### "CUDA out of memory"
- Your file is larger than GPU VRAM
- Solution: Use `upload_products.cpp` (CPU-based) instead, or split CSV into smaller files

### "mysql_real_connect failed: Access denied"
- Check MYSQL_USER, MYSQL_PASS, and MYSQL_DB are correct
- Verify MySQL server is running: `mysql -h 127.0.0.1 -u root -p`
- Check user permissions: `GRANT ALL ON mydatabase.* TO 'myuser'@'127.0.0.1';`

### "LOAD DATA LOCAL INFILE disabled"
On the MySQL server, enable LOCAL INFILE:
```sql
SET GLOBAL local_infile = 1;
```

In `/etc/mysql/my.cnf` (or `my.ini` on Windows), add:
```ini
[mysqld]
local-infile=1
```

### "Table doesn't exist"
Create it first:
```bash
mysql -h 127.0.0.1 -u myuser -p mydatabase < sql_schema.sql
```

## Architecture

```
[CSV File on Disk]
        ↓
  [CPU reads file]
        ↓
  [GPU memory copy]
        ↓
  [GPU line counting & analysis]
        ↓
  [CPU parses rows (reliable CSV handling)]
        ↓
  [Format for MySQL LOAD DATA]
        ↓
  [MySQL LOAD DATA LOCAL INFILE]
        ↓
  [Data in MySQL Table]
```

## Advanced: Custom CSV Handling

If your CSV has different delimiters or quotes:

Edit `nvcsv_mysql.cu`:
```cpp
while (std::getline(ss, field, ',') && col < num_fields) {  // Change ',' to your delimiter
```

For tab-delimited:
```cpp
while (std::getline(ss, field, '\t') && col < num_fields) {
```

Then rebuild:
```bash
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
```

## Comparison: nvcsv_mysql vs upload_products

| Feature | nvcsv_mysql | upload_products |
|---------|-----------|-----------------|
| GPU Acceleration | Yes | No |
| CUDA Required | Yes | No |
| Speed (Large Files) | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| Setup Complexity | Medium | Low |
| Memory Usage | GPU + Host | Host only |
| Best For | Large CSV files (1GB+) | Small-medium files, dev/test |

**Recommendation:** Use `nvcsv_mysql` for production data loads. Use `upload_products` for testing or small files.

## Security Notes

- **Never commit passwords** to version control. Use environment variables or secure secret stores.
- For production, use MySQL user accounts with minimal required privileges:
  ```sql
  GRANT INSERT ON mydatabase.products TO 'csv_loader'@'127.0.0.1' IDENTIFIED BY 'password';
  ```

## License

Same as NVCSV project.
