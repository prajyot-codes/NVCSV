# Quick Setup Guide: CSV to MySQL with NVCSV

## 5-Minute Setup

### 1. Prerequisites
```bash
# Ubuntu/Debian
sudo apt install build-essential nvidia-cuda-toolkit libmysqlclient-dev

# macOS
brew install mysql-client
```

### 2. Prepare MySQL

Create your table (adjust columns to match your CSV):
```sql
CREATE TABLE products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255),
    sku VARCHAR(100),
    description TEXT
);
```

Verify table exists:
```bash
mysql -h 127.0.0.1 -u myuser -p mydb -e "DESCRIBE products;"
```

### 3. Build NVCSV-MySQL

```bash
cd /path/to/NVCSV
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
```

### 4. Run the Insertion

```bash
export MYSQL_HOST=127.0.0.1
export MYSQL_USER=myuser
export MYSQL_PASS=mypassword
export MYSQL_DB=mydb
export MYSQL_TABLE=products

./nvcsv_mysql products.csv
```

### 5. Verify

```bash
mysql -h 127.0.0.1 -u myuser -p mydb -e "SELECT COUNT(*) FROM products;"
```

---

## Common Issues & Fixes

| Issue | Fix |
|-------|-----|
| `nvcc: command not found` | Install CUDA: `sudo apt install nvidia-cuda-toolkit` |
| `fatal error: mysql/mysql.h: No such file` | Install MySQL dev: `sudo apt install libmysqlclient-dev` |
| `mysql_real_connect failed: Access denied` | Check username/password: `mysql -h 127.0.0.1 -u myuser -p` |
| `Table 'mydb.products' doesn't exist` | Create table first (see step 2 above) |
| `LOAD DATA LOCAL INFILE disabled` | Run: `SET GLOBAL local_infile = 1;` in MySQL |
| `CUDA out of memory` | File too large for GPU. Use `upload_products` instead |

---

## Two Options

### Option A: GPU-Fast (Recommended for Large Files)
```bash
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
./nvcsv_mysql products.csv
```
- **Speed:** ⭐⭐⭐⭐⭐ (fastest)
- **Memory:** Uses GPU + Host RAM
- **Setup:** Requires CUDA

### Option B: CPU-Only (Simpler Setup)
```bash
g++ -o upload_products upload_products.cpp mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient
./upload_products products.csv
```
- **Speed:** ⭐⭐ (CPU-bound)
- **Memory:** Host RAM only
- **Setup:** No CUDA needed

---

## Environment Variables Required

```bash
MYSQL_HOST=127.0.0.1      # MySQL server address
MYSQL_USER=myuser         # Your MySQL username
MYSQL_PASS=mypassword     # Your MySQL password (optional)
MYSQL_DB=mydb             # Database name
MYSQL_TABLE=products      # Table name
```

One-liner to set all:
```bash
export MYSQL_HOST=127.0.0.1 MYSQL_USER=myuser MYSQL_PASS=mypass MYSQL_DB=mydb MYSQL_TABLE=products
```

---

## CSV File Format

Your CSV must have:
1. **Header row** with column names (automatically skipped)
2. **Data rows** with matching number of columns
3. **Delimiters:** comma (`,`) - modify source if different

Example `products.csv`:
```
name,sku,description
Product A,SKU-001,"A great product"
Product B,SKU-002,"Another product"
```

---

## Advanced: Custom CSV Delimiters

Edit `nvcsv_mysql.cu` line ~170:
```cpp
while (std::getline(ss, field, ',') && col < num_fields) {  // Change ',' here
```

For tab-delimited CSV:
```cpp
while (std::getline(ss, field, '\t') && col < num_fields) {
```

Then rebuild:
```bash
nvcc -O3 -use_fast_math nvcsv_mysql.cu mysql_uploader.cpp -DMYSQL_UPLOAD -lmysqlclient -o nvcsv_mysql
```

---

## Performance Tips

- **Large files:** Use `nvcsv_mysql` (GPU-accelerated)
- **Many small files:** Run sequentially: `for f in *.csv; do ./nvcsv_mysql "$f"; done`
- **Slow inserts:** Check MySQL: `SET GLOBAL max_allowed_packet = 1073741824;`
- **Check progress:** Monitor MySQL: `SHOW PROCESSLIST;`

---

**Questions?** See `NVCSV_MYSQL_GUIDE.md` for comprehensive documentation.
