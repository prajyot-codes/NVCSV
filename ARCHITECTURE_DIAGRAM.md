# NVCSV-MySQL: Visual Architecture & Flow

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CSV File on Disk                             │
│                      (e.g., products.csv)                           │
│                    (100MB, 1M rows, 3 columns)                      │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ▼
        ┌──────────────────────────────────────┐
        │   Two Options Available:              │
        │                                      │
        │ ◆ Option 1: nvcsv_mysql (GPU)        │
        │   - Fast (⭐⭐⭐⭐⭐)                  │
        │   - Uses CUDA acceleration           │
        │   - Best for large files             │
        │                                      │
        │ ◆ Option 2: upload_products (CPU)    │
        │   - Simple (⭐⭐)                     │
        │   - No CUDA needed                   │
        │   - Good for testing                 │
        └──────────────────────────────────────┘
                           │
        ┌──────────────────┴──────────────────┐
        │                                     │
        ▼                                     ▼
┌──────────────────┐             ┌──────────────────┐
│   GPU PATH       │             │    CPU PATH      │
├──────────────────┤             ├──────────────────┤
│ (nvcsv_mysql)    │             │(upload_products) │
│                  │             │                  │
│ 1. Read CSV      │             │ 1. Read CSV      │
│ 2. GPU transfer  │             │ 2. CPU parse     │
│ 3. GPU count     │             │ 3. Format rows   │
│    lines         │             │ 4. Create CSV    │
│ 4. GPU analysis  │             │    string        │
│ 5. CPU parse     │             │                  │
│    (reliable)    │             │                  │
│ 6. Format CSV    │             │                  │
│    output        │             │                  │
└──────────┬───────┘             └────────┬─────────┘
           │                              │
           └──────────────┬───────────────┘
                          │
                          ▼
        ┌────────────────────────────────────┐
        │  Format as CSV String Buffer       │
        │  (proper quoting, escaping)        │
        │                                    │
        │  name,sku,description              │
        │  "Product A","SKU-001","Desc"      │
        │  "Product B","SKU-002","Desc"      │
        │  ... (1M rows)                     │
        └────────────────┬───────────────────┘
                         │
                         ▼
        ┌────────────────────────────────────┐
        │  Write Temp File (/tmp/*.csv)      │
        │                                    │
        │  mysql_uploader.cpp:               │
        │  - Create temp file                │
        │  - Write CSV data                  │
        │  - Connect to MySQL                │
        └────────────────┬───────────────────┘
                         │
                         ▼
        ┌────────────────────────────────────┐
        │  MySQL LOAD DATA LOCAL INFILE      │
        │                                    │
        │  LOAD DATA LOCAL INFILE            │
        │    '/tmp/file.csv'                 │
        │  INTO TABLE products               │
        │  FIELDS TERMINATED BY ','          │
        │  ENCLOSED BY '"'                   │
        │  LINES TERMINATED BY '\n'          │
        │  IGNORE 1 ROWS;                    │
        └────────────────┬───────────────────┘
                         │
                         ▼
        ┌────────────────────────────────────┐
        │  Pre-Existing MySQL Table          │
        │                                    │
        │  products table:                   │
        │  ├─ id (INT, auto-increment)       │
        │  ├─ name (VARCHAR)                 │
        │  ├─ sku (VARCHAR)                  │
        │  ├─ description (TEXT)             │
        │  ├─ created_at (TIMESTAMP)         │
        │  │                                 │
        │  └─ [1,000,000 rows inserted] ✅   │
        └────────────────────────────────────┘
```

## Data Flow: Step by Step

```
STEP 1: File Reading
───────────────────
CSV File (52 MB)
    │
    └──→ Read into Host Memory
        (char buffer: 52 MB)
        
STEP 2: GPU Transfer (GPU PATH ONLY)
────────────────────────
Host Memory (52 MB)
    │
    └──→ cudaMemcpy()
        GPU Memory (52 MB)

STEP 3: GPU Analysis (GPU PATH ONLY)
─────────────────────────
GPU Memory
    │
    ├──→ thrust::count('\n')  = 1,000,000 newlines
    │
    ├──→ Line position tracking
    │
    └──→ Data statistics

STEP 4: CPU Parsing (Both Paths)
────────────────────────
Line 1: name,sku,description      (SKIP - header)
Line 2: Widget A,SKU-001,..."     → {name, sku, desc}
Line 3: Widget B,SKU-002,..."     → {name, sku, desc}
        ...
Line N: Widget Z,SKU-999,..."     → {name, sku, desc}

STEP 5: CSV Formatting
──────────────────────
Parsed Row Data
    │
    ├──→ Escape quotes (→ "")
    ├──→ Wrap fields in quotes
    ├──→ Join with commas
    └──→ Add newlines
    
Output: CSV String Buffer (57 MB)

STEP 6: Temp File Creation
───────────────────────────
CSV String Buffer (57 MB)
    │
    └──→ /tmp/nvcsv_mysql_XXXXXX.csv (57 MB file)

STEP 7: MySQL Connection & Load
───────────────────────────────
MySQL Server (127.0.0.1:3306)
    │
    ├──→ mysql_init()
    ├──→ mysql_real_connect()
    │
    └──→ mysql_query(LOAD DATA LOCAL INFILE)
        
        Server reads temp file → Parses CSV → Inserts rows
        
        1,000,000 rows → [INSERT INTO products VALUES (...)]

STEP 8: Cleanup
───────────────
Temp File (/tmp/nvcsv_mysql_XXXXXX.csv)
    │
    └──→ unlink() → Deleted

Database ✅
    │
    └──→ [1,000,000 rows in products table]
```

## Comparison: GPU vs CPU Path

```
┌─────────────────────────────────────────────────────────┐
│            GPU Path (nvcsv_mysql)                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Host                    GPU              Host          │
│  ────                    ───              ────          │
│   │                       │                 │           │
│   ├─ Read CSV ────→ [Transfer] ────→ Count lines       │
│   │                       │                 │           │
│   ├─ Memory                  Parallel      ← Faster!   │
│   │  (52 MB)           Processing          │           │
│   │                     (1000s of          │           │
│   │                      threads)           │           │
│   │                       │                 │           │
│   └─────────────── ←────  │                 │           │
│        Parse rows ←──────  GPU Memory       │           │
│        (CPU, reliable)                      │           │
│                                             │           │
│        Format CSV ────────────────────────→ Buffer      │
│        Insert (LOAD DATA)                   │           │
│                                             ▼           │
│                                        MySQL (1M rows)  │
│                                                         │
│  Time for 100MB file: ~2-5 seconds ⭐⭐⭐⭐⭐           │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│           CPU Path (upload_products)                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Host                                                   │
│  ────                                                   │
│   │                                                     │
│   ├─ Read CSV file                                     │
│   │  (52 MB)                                           │
│   │                                                     │
│   ├─ CPU Parse rows (single thread mostly)             │
│   │  ├─ Line 1: nope, header                           │
│   │  ├─ Line 2: Widget A, SKU-001                      │
│   │  ├─ Line 3: Widget B, SKU-002                      │
│   │  └─ ... (sequential)                               │
│   │                                                     │
│   ├─ Format CSV output                                 │
│   │  (quote escaping, newlines)                        │
│   │                                                     │
│   └─ Insert (LOAD DATA) ───────────→ MySQL (1M rows)   │
│                                                         │
│  Time for 100MB file: ~5-10 seconds ⭐⭐                │
└─────────────────────────────────────────────────────────┘
```

## Program Flow Diagram

### nvcsv_mysql.cu Flow

```
START
  │
  ├─ Parse command line args
  │
  ├─ Check MYSQL_* env variables
  │  (HOST, USER, PASS, DB, TABLE)
  │
  ├─ Initialize CUDA context
  │
  ├─ Open CSV file
  │
  ├─ Read entire file into memory
  │  │
  │  └─ malloc(file_size)
  │     fopen() → fread()
  │
  ├─ Copy to GPU memory
  │  │
  │  └─ thrust::device_vector
  │     thrust::copy()
  │
  ├─ Count lines on GPU
  │  │
  │  └─ thrust::count(dev.begin(), dev.end(), '\n')
  │
  ├─ Parse CSV (CPU, for reliability)
  │  │
  │  ├─ std::ifstream
  │  ├─ std::getline(stream, field, ',')
  │  ├─ Remove quotes
  │  └─ Store in field_data_host[][]
  │
  ├─ Format CSV output
  │  │
  │  ├─ Escape quotes in fields
  │  ├─ Wrap fields in quotes
  │  ├─ Join with commas
  │  └─ Create CSV string
  │
  ├─ Upload to MySQL
  │  │
  │  └─ upload_csv_to_mysql()
  │     ├─ Create temp file
  │     ├─ Write CSV data
  │     ├─ Connect to MySQL
  │     ├─ LOAD DATA LOCAL INFILE
  │     ├─ Cleanup temp file
  │     └─ Return status
  │
  ├─ Print results
  │
  ├─ Free memory
  │
  └─ EXIT (0 = success, 1 = error)
```

### upload_products.cpp Flow

```
START
  │
  ├─ Check command line args
  │
  ├─ Check MYSQL_* env variables
  │
  ├─ Open CSV file
  │
  ├─ Get file size
  │
  ├─ Allocate host memory
  │
  ├─ Read file into memory
  │
  ├─ Call upload_csv_to_mysql()
  │  │
  │  └─ (same as nvcsv_mysql from here on)
  │     ├─ Create temp file
  │     ├─ Write CSV
  │     ├─ Connect MySQL
  │     ├─ LOAD DATA
  │     └─ Cleanup
  │
  ├─ Free memory
  │
  └─ EXIT (0 = success, 1 = error)
```

## MySQL LOAD DATA Process

```
Your Program (nvcsv_mysql or upload_products)
    │
    ├─ Create /tmp/nvcsv_mysql_XXXXXX.csv (57 MB)
    │
    ├─ Connect to MySQL server
    │  └─ mysql_real_connect(host, user, pass, db)
    │
    ├─ Enable LOCAL INFILE
    │  └─ mysql_options(MYSQL_OPT_LOCAL_INFILE, 1)
    │
    ├─ Execute SQL:
    │  │
    │  └─ LOAD DATA LOCAL INFILE '/tmp/nvcsv_mysql_XXXXXX.csv'
    │     INTO TABLE products
    │     FIELDS TERMINATED BY ','
    │     ENCLOSED BY '"'
    │     LINES TERMINATED BY '\n'
    │     IGNORE 1 ROWS;
    │
    ▼
MySQL Server (on same or remote host)
    │
    ├─ Receive command from client
    │
    ├─ Verify LOCAL INFILE enabled
    │  └─ local_infile = ON
    │
    ├─ Read temp file in one go
    │  (fast: sequential read, not byte-by-byte)
    │
    ├─ Parse CSV format
    │  ├─ Skip 1st row (header)
    │  ├─ Extract 3 fields per row
    │  └─ Handle quoted fields
    │
    ├─ Insert 1,000,000 rows
    │  └─ INSERT statements executed in batch
    │
    ├─ Commit transaction
    │
    ├─ Return: affected_rows = 1,000,000
    │
    └─ Return to client

Your Program
    │
    ├─ Receive result
    │  └─ "inserted 1,000,000 rows"
    │
    ├─ Delete temp file
    │
    ├─ Close MySQL connection
    │
    ├─ Print SUCCESS
    │
    └─ EXIT 0
```

## Memory Layout During Execution

```
BEFORE RUNNING:
┌─────────────────────────────┐
│  Disk: products.csv (52 MB) │
└─────────────────────────────┘

DURING GPU PATH (nvcsv_mysql):
┌─────────────────────────────────────┐
│  Host Memory:                       │
│  ├─ CSV data (52 MB)                │
│  ├─ Parsed fields (20 MB)           │
│  └─ CSV output (57 MB)              │
│  Total: ~130 MB                     │
├─────────────────────────────────────┤
│  GPU Memory:                        │
│  ├─ Copy of CSV data (52 MB)        │
│  ├─ Line position array (16 MB)     │
│  └─ Working space (10 MB)           │
│  Total: ~78 MB (with overhead)      │
├─────────────────────────────────────┤
│  Temp File:                         │
│  └─ /tmp/nvcsv_mysql_XXXXXX.csv     │
│     (57 MB, deleted after insert)   │
└─────────────────────────────────────┘

DURING CPU PATH (upload_products):
┌─────────────────────────────────────┐
│  Host Memory Only:                  │
│  ├─ CSV data (52 MB)                │
│  ├─ Parsed fields (20 MB)           │
│  └─ CSV output (57 MB)              │
│  Total: ~130 MB                     │
├─────────────────────────────────────┤
│  GPU Memory: Unused                 │
├─────────────────────────────────────┤
│  Temp File:                         │
│  └─ /tmp/nvcsv_mysql_XXXXXX.csv     │
│     (57 MB, deleted after insert)   │
└─────────────────────────────────────┘

AFTER COMPLETION:
┌─────────────────────────────────────┐
│  Disk: products table in MySQL      │
│  (1,000,000 rows in database)       │
│                                     │
│  products/                          │
│  ├─ id (auto)                       │
│  ├─ name                            │
│  ├─ sku                             │
│  ├─ description                     │
│  └─ created_at (auto)               │
└─────────────────────────────────────┘
```

## Environment Variable Flow

```
Shell Environment
    │
    ├─ MYSQL_HOST="127.0.0.1"
    │   └─ getenv("MYSQL_HOST") → host variable
    │       └─ mysql_real_connect(host, ...)
    │
    ├─ MYSQL_USER="root"
    │   └─ getenv("MYSQL_USER") → user variable
    │       └─ mysql_real_connect(..., user, ...)
    │
    ├─ MYSQL_PASS="password"
    │   └─ getenv("MYSQL_PASS") → pass variable
    │       └─ mysql_real_connect(..., pass, ...)
    │
    ├─ MYSQL_DB="mydb"
    │   └─ getenv("MYSQL_DB") → db variable
    │       └─ mysql_real_connect(..., db, ...)
    │
    └─ MYSQL_TABLE="products"
        └─ getenv("MYSQL_TABLE") → table variable
            └─ LOAD DATA INTO TABLE <table>
```

## Success Workflow (Checklist)

```
✅ Prerequisites
   ├─ MySQL installed and running
   ├─ Database created
   └─ Table created (products)

✅ Environment Setup
   ├─ MYSQL_HOST set
   ├─ MYSQL_USER set
   ├─ MYSQL_DB set
   └─ MYSQL_TABLE set

✅ Build
   ├─ CUDA installed (for GPU path)
   ├─ libmysqlclient-dev installed
   └─ Compile succeeds

✅ Execution
   ├─ Program starts
   ├─ Reads CSV
   ├─ Parses rows
   ├─ Connects to MySQL
   ├─ Loads data
   └─ Completes with "inserted N rows"

✅ Verification
   ├─ SELECT COUNT(*) shows row count
   └─ SELECT * LIMIT shows data
```

---

This visual guide shows how all pieces fit together!
