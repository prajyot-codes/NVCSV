-- SQL Schema for products table
-- This table schema matches the structure of products.csv
-- 
-- Columns in products.csv:
--   1. name (VARCHAR) - Product name
--   2. sku (VARCHAR) - Stock Keeping Unit (product code)
--   3. description (TEXT) - Product description

CREATE TABLE IF NOT EXISTS products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    sku VARCHAR(100) NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Optional: Create indexes for faster lookups
CREATE INDEX idx_sku ON products(sku);
CREATE INDEX idx_name ON products(name);

-- To use LOAD DATA LOCAL INFILE, ensure the table exists first.
-- The import will match columns by position: name, sku, description
