#!/usr/bin/env bash

#
# Performance Benchmark Script for omni_schema views
# Tests the optimized dependency and acl views
# 
# Usage: ./benchmark_schema_views.sh [host] [port] [database] [user]
#

set -euo pipefail

# Default connection parameters
HOST="${1:-localhost}"
PORT="${2:-5432}"
DATABASE="${3:-omnigres}"
USER="${4:-omnigres}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[$(date '+%H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

benchmark_query() {
    local query="$1"
    local description="$2"
    local iterations="${3:-5}"
    
    log "Testing: $description"
    echo "Query: $query"
    echo ""
    
    # Run EXPLAIN ANALYZE multiple times and capture timing
    local total_time=0
    local min_time=99999
    local max_time=0
    
    for i in $(seq 1 $iterations); do
        local timing=$(psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "
            \\timing on
            EXPLAIN (ANALYZE, BUFFERS, FORMAT TEXT) $query;
        " 2>&1 | grep "Time:" | tail -1 | sed 's/Time: //' | sed 's/ ms//')
        
        if [[ ! -z "$timing" ]]; then
            local time_ms=$(echo "$timing" | cut -d'.' -f1)
            total_time=$((total_time + time_ms))
            
            if (( time_ms < min_time )); then
                min_time=$time_ms
            fi
            if (( time_ms > max_time )); then
                max_time=$time_ms
            fi
            
            echo "  Run $i: ${time_ms}ms"
        fi
    done
    
    if (( total_time > 0 )); then
        local avg_time=$((total_time / iterations))
        echo "  Average: ${avg_time}ms"
        echo "  Min: ${min_time}ms"
        echo "  Max: ${max_time}ms"
    fi
    
    echo ""
    echo "----------------------------------------"
    echo ""
}

# Check if we can connect to PostgreSQL
log "Testing connection to PostgreSQL..."
if ! psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "SELECT 1;" > /dev/null 2>&1; then
    error "Cannot connect to PostgreSQL. Please check connection parameters."
    exit 1
fi

log "Connected to PostgreSQL successfully"

# Check if omni_schema extension is available
log "Checking for omni_schema extension..."
if ! psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "SELECT 1 FROM pg_extension WHERE extname = 'omni_schema';" | grep -q "1"; then
    error "omni_schema extension not found. Please install it first."
    exit 1
fi

log "omni_schema extension found"

# Get database statistics for context
log "Database context:"
psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "
SELECT 
    'Tables' as object_type, 
    count(*) as count 
FROM pg_tables 
WHERE schemaname NOT IN ('information_schema', 'pg_catalog')
UNION ALL
SELECT 
    'Functions' as object_type, 
    count(*) as count 
FROM pg_proc p 
JOIN pg_namespace n ON n.oid = p.pronamespace 
WHERE n.nspname NOT IN ('information_schema', 'pg_catalog')
UNION ALL
SELECT 
    'Types' as object_type, 
    count(*) as count 
FROM pg_type t 
JOIN pg_namespace n ON n.oid = t.typnamespace 
WHERE n.nspname NOT IN ('information_schema', 'pg_catalog')
ORDER BY object_type;
"

echo ""
log "Starting performance benchmarks..."
echo ""

# Test dependency view performance
benchmark_query "SELECT count(*) FROM omni_schema.dependency" "Dependency view - full count"
benchmark_query "SELECT * FROM omni_schema.dependency LIMIT 1000" "Dependency view - first 1000 rows"
benchmark_query "SELECT id, count(*) as deps FROM omni_schema.dependency GROUP BY id LIMIT 100" "Dependency view - grouped by object"

# Test ACL view performance  
benchmark_query "SELECT count(*) FROM omni_schema.acl" "ACL view - full count"
benchmark_query "SELECT * FROM omni_schema.acl LIMIT 1000" "ACL view - first 1000 rows"
benchmark_query "SELECT privilege_type, count(*) FROM omni_schema.acl GROUP BY privilege_type" "ACL view - grouped by privilege type"

# Test combined queries that might be used in schema diffing
benchmark_query "SELECT d.id, count(d.dependent_on) as dep_count, count(a.privilege_type) as acl_count FROM omni_schema.dependency d LEFT JOIN omni_schema.acl a ON d.id = a.id GROUP BY d.id LIMIT 100" "Combined dependency and ACL analysis"

# Get query plans for analysis
log "Analyzing query plans..."
psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "
EXPLAIN (ANALYZE, BUFFERS, FORMAT JSON) 
SELECT count(*) FROM omni_schema.dependency;
" > dependency_plan.json

psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "
EXPLAIN (ANALYZE, BUFFERS, FORMAT JSON) 
SELECT count(*) FROM omni_schema.acl;
" > acl_plan.json

log "Query plans saved to dependency_plan.json and acl_plan.json"

# Test index recommendations
log "Checking for missing indexes that could improve performance..."
psql -h "$HOST" -p "$PORT" -d "$DATABASE" -U "$USER" -c "
-- Check if recommended indexes exist
SELECT 
    schemaname,
    tablename,
    indexname,
    indexdef
FROM pg_indexes 
WHERE tablename IN ('pg_depend', 'pg_class', 'pg_proc', 'pg_type', 'pg_attribute')
    AND indexdef LIKE '%classid%' OR indexdef LIKE '%objid%' OR indexdef LIKE '%relnamespace%'
ORDER BY tablename, indexname;
"

log "Performance benchmark completed!"
log "Review the timing results above to measure performance improvements."
echo ""
echo "Expected improvements:"
echo "- dependency view: 2x-5x faster (target: <100ms for typical workloads)"
echo "- acl view: 2x-5x faster (target: <50ms for typical workloads)"
echo ""
echo "Key optimizations implemented:"
echo "1. Early filtering of system objects (pg_catalog, information_schema)"
echo "2. Materialized CTEs for better query planning"
echo "3. Namespace caching to reduce repeated scans"
echo "4. Conditional ACL processing (only when ACLs exist)"
echo "5. Better join strategies and index utilization"
echo "6. Elimination of redundant UNION operations"