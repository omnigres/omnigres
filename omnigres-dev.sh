#!/usr/bin/env bash

#
# Omnigres Development Script
# Complete bash script for building, testing, and managing the Omnigres project
#
# Usage:
#   ./omnigres-dev.sh [command] [options]
#
# Commands:
#   build       - Build the entire project or specific components
#   test        - Run tests
#   clean       - Clean build artifacts
#   setup       - Setup development environment
#   docker      - Docker operations
#   extensions  - Manage extensions
#   help        - Show this help message
#

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_ROOT}/build"
LOG_FILE="${BUILD_DIR}/omnigres-dev.log"

# Default values
VERBOSE=0
PARALLEL_JOBS=$(nproc)
CMAKE_BUILD_TYPE="Release"
POSTGRES_VERSION="17"

# Logging function
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    case $level in
        "INFO")
            echo -e "${GREEN}[INFO]${NC} ${message}"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} ${message}"
            ;;
        "ERROR")
            echo -e "${RED}[ERROR]${NC} ${message}"
            ;;
        "DEBUG")
            if [[ $VERBOSE -eq 1 ]]; then
                echo -e "${BLUE}[DEBUG]${NC} ${message}"
            fi
            ;;
        "SUCCESS")
            echo -e "${GREEN}[SUCCESS]${NC} ${message}"
            ;;
    esac
    
    # Also log to file if build directory exists
    if [[ -d "${BUILD_DIR}" ]]; then
        echo "[${timestamp}] [${level}] ${message}" >> "${LOG_FILE}"
    fi
}

# Error handler
error_exit() {
    log "ERROR" "$1"
    exit 1
}

# Check dependencies
check_dependencies() {
    log "INFO" "Checking dependencies..."
    
    local deps=("git" "cmake" "make" "gcc" "g++" "pkg-config")
    local missing_deps=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log "ERROR" "Missing dependencies: ${missing_deps[*]}"
        log "INFO" "Please install missing dependencies and try again"
        exit 1
    fi
    
    log "SUCCESS" "All dependencies are available"
}

# Setup development environment
setup_environment() {
    log "INFO" "Setting up development environment..."
    
    # Create build directory
    mkdir -p "${BUILD_DIR}"
    
    # Check for PostgreSQL
    if ! command -v pg_config &> /dev/null; then
        log "WARN" "PostgreSQL development files not found"
        log "INFO" "Installing PostgreSQL development packages..."
        
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y postgresql-server-dev-${POSTGRES_VERSION} postgresql-${POSTGRES_VERSION}
        elif command -v yum &> /dev/null; then
            sudo yum install -y postgresql${POSTGRES_VERSION}-devel postgresql${POSTGRES_VERSION}-server
        elif command -v apk &> /dev/null; then
            sudo apk add postgresql${POSTGRES_VERSION}-dev postgresql${POSTGRES_VERSION}
        else
            log "ERROR" "Unsupported package manager. Please install PostgreSQL development files manually."
            exit 1
        fi
    fi
    
    # Initialize git submodules
    if [[ -f "${PROJECT_ROOT}/.gitmodules" ]]; then
        log "INFO" "Initializing git submodules..."
        git submodule update --init --recursive
    fi
    
    # Install Python dependencies if needed
    if [[ -f "${PROJECT_ROOT}/requirements.txt" ]]; then
        log "INFO" "Installing Python dependencies..."
        pip install -r "${PROJECT_ROOT}/requirements.txt"
    fi
    
    log "SUCCESS" "Development environment setup complete"
}

# Clean build artifacts
clean_build() {
    log "INFO" "Cleaning build artifacts..."
    
    if [[ -d "${BUILD_DIR}" ]]; then
        rm -rf "${BUILD_DIR}"
        log "SUCCESS" "Build directory cleaned"
    fi
    
    # Clean any additional artifacts
    find "${PROJECT_ROOT}" -name "*.o" -delete 2>/dev/null || true
    find "${PROJECT_ROOT}" -name "*.so" -delete 2>/dev/null || true
    find "${PROJECT_ROOT}" -name "*.a" -delete 2>/dev/null || true
    find "${PROJECT_ROOT}" -name "CMakeCache.txt" -delete 2>/dev/null || true
    find "${PROJECT_ROOT}" -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
    
    log "SUCCESS" "Clean completed"
}

# Configure CMake
configure_cmake() {
    log "INFO" "Configuring CMake..."
    
    mkdir -p "${BUILD_DIR}"
    
    local cmake_args=(
        "-S" "${PROJECT_ROOT}"
        "-B" "${BUILD_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )
    
    # Add additional CMake arguments if needed
    if [[ $VERBOSE -eq 1 ]]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    log "DEBUG" "Running: cmake ${cmake_args[*]}"
    
    if cmake "${cmake_args[@]}"; then
        log "SUCCESS" "CMake configuration completed"
    else
        error_exit "CMake configuration failed"
    fi
}

# Build the project
build_project() {
    local target="${1:-all}"
    
    log "INFO" "Building target: ${target}"
    
    if [[ ! -d "${BUILD_DIR}" ]] || [[ ! -f "${BUILD_DIR}/Makefile" ]]; then
        configure_cmake
    fi
    
    local build_args=(
        "--build" "${BUILD_DIR}"
        "--parallel" "${PARALLEL_JOBS}"
        "--target" "${target}"
    )
    
    if [[ $VERBOSE -eq 1 ]]; then
        build_args+=("--verbose")
    fi
    
    log "DEBUG" "Running: cmake ${build_args[*]}"
    
    if cmake "${build_args[@]}"; then
        log "SUCCESS" "Build completed successfully"
    else
        error_exit "Build failed"
    fi
}

# Build extensions separately
build_extensions_separately() {
    log "INFO" "Building extensions separately..."
    
    # Prime so that Postgres is always there
    if [[ ! -d "${PROJECT_ROOT}/.pg" ]]; then
        rm -rf "${BUILD_DIR}/_"
        cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}/_"
        rm -rf "${BUILD_DIR}/_"
    fi
    
    for dir in "${PROJECT_ROOT}/extensions"/*; do
        if [[ -f "$dir/CMakeLists.txt" ]]; then
            local ext_name=$(basename "${dir}")
            local build_dir="${BUILD_DIR}/_${ext_name}"
            
            log "INFO" "Building extension: ${ext_name}"
            
            rm -rf "${build_dir}"
            
            if cmake -S "${dir}" -B "${build_dir}" && \
               cmake --build "${build_dir}" --parallel --target all --target package_extensions; then
                log "SUCCESS" "Extension ${ext_name} built successfully"
            else
                error_exit "Failed to build extension: ${ext_name}"
            fi
        fi
    done
    
    log "SUCCESS" "All extensions built successfully"
}

# Run tests
run_tests() {
    local test_pattern="${1:-}"
    
    log "INFO" "Running tests..."
    
    if [[ ! -d "${BUILD_DIR}" ]]; then
        error_exit "Build directory not found. Please build the project first."
    fi
    
    cd "${BUILD_DIR}"
    
    local ctest_args=("--output-on-failure" "--parallel" "${PARALLEL_JOBS}")
    
    if [[ -n "${test_pattern}" ]]; then
        ctest_args+=("-R" "${test_pattern}")
    fi
    
    if [[ $VERBOSE -eq 1 ]]; then
        ctest_args+=("--verbose")
    fi
    
    log "DEBUG" "Running: ctest ${ctest_args[*]}"
    
    if ctest "${ctest_args[@]}"; then
        log "SUCCESS" "All tests passed"
    else
        error_exit "Some tests failed"
    fi
    
    cd "${PROJECT_ROOT}"
}

# Docker operations
docker_operations() {
    local operation="${1:-}"
    
    case $operation in
        "build")
            log "INFO" "Building Docker image..."
            if DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres; then
                log "SUCCESS" "Docker image built successfully"
            else
                error_exit "Docker build failed"
            fi
            ;;
        "run")
            log "INFO" "Running Docker container..."
            docker volume create omnigres 2>/dev/null || true
            docker run --name omnigres-dev \
                --mount source=omnigres,target=/var/lib/postgresql/data \
                -p 127.0.0.1:5432:5432 -p 127.0.0.1:8080:8080 -p 127.0.0.1:8081:8081 \
                --rm -d ghcr.io/omnigres/omnigres
            log "SUCCESS" "Docker container started"
            log "INFO" "PostgreSQL: localhost:5432 (user: omnigres, password: omnigres)"
            log "INFO" "HTTP server: http://localhost:8081"
            ;;
        "stop")
            log "INFO" "Stopping Docker container..."
            docker stop omnigres-dev 2>/dev/null || true
            log "SUCCESS" "Docker container stopped"
            ;;
        "pull")
            log "INFO" "Pulling latest Docker image..."
            docker pull ghcr.io/omnigres/omnigres-17:latest
            log "SUCCESS" "Docker image pulled"
            ;;
        *)
            log "ERROR" "Unknown docker operation: ${operation}"
            log "INFO" "Available operations: build, run, stop, pull"
            exit 1
            ;;
    esac
}

# Extension management
manage_extensions() {
    local action="${1:-list}"
    local extension="${2:-}"
    
    case $action in
        "list")
            log "INFO" "Available extensions:"
            for dir in "${PROJECT_ROOT}/extensions"/*; do
                if [[ -d "$dir" ]]; then
                    local ext_name=$(basename "${dir}")
                    echo "  - ${ext_name}"
                fi
            done
            ;;
        "build")
            if [[ -z "$extension" ]]; then
                error_exit "Extension name required for build action"
            fi
            
            local ext_dir="${PROJECT_ROOT}/extensions/${extension}"
            if [[ ! -d "$ext_dir" ]]; then
                error_exit "Extension not found: ${extension}"
            fi
            
            log "INFO" "Building extension: ${extension}"
            local build_dir="${BUILD_DIR}/ext_${extension}"
            
            rm -rf "${build_dir}"
            
            if cmake -S "${ext_dir}" -B "${build_dir}" && \
               cmake --build "${build_dir}" --parallel; then
                log "SUCCESS" "Extension ${extension} built successfully"
            else
                error_exit "Failed to build extension: ${extension}"
            fi
            ;;
        "test")
            if [[ -z "$extension" ]]; then
                error_exit "Extension name required for test action"
            fi
            
            log "INFO" "Testing extension: ${extension}"
            run_tests "*${extension}*"
            ;;
        *)
            log "ERROR" "Unknown extension action: ${action}"
            log "INFO" "Available actions: list, build, test"
            exit 1
            ;;
    esac
}

# Show help
show_help() {
    cat << EOF
${WHITE}Omnigres Development Script${NC}

${GREEN}USAGE:${NC}
    $0 [COMMAND] [OPTIONS]

${GREEN}COMMANDS:${NC}
    ${CYAN}build${NC}           Build the entire project
        build all       Build all components (default)
        build ext       Build extensions separately
        build clean     Clean and rebuild

    ${CYAN}test${NC}            Run tests
        test            Run all tests
        test [pattern]  Run tests matching pattern

    ${CYAN}clean${NC}           Clean build artifacts

    ${CYAN}setup${NC}           Setup development environment

    ${CYAN}docker${NC}          Docker operations
        docker build    Build Docker image
        docker run      Run Docker container
        docker stop     Stop Docker container
        docker pull     Pull latest image

    ${CYAN}extensions${NC}      Manage extensions
        ext list        List all extensions
        ext build [ext] Build specific extension
        ext test [ext]  Test specific extension

    ${CYAN}help${NC}            Show this help message

${GREEN}OPTIONS:${NC}
    ${CYAN}-v, --verbose${NC}       Enable verbose output
    ${CYAN}-j, --jobs N${NC}        Number of parallel jobs (default: $(nproc))
    ${CYAN}-t, --type TYPE${NC}     CMake build type (default: Release)
    ${CYAN}-h, --help${NC}          Show this help message

${GREEN}EXAMPLES:${NC}
    $0 setup                    # Setup development environment
    $0 build                    # Build entire project
    $0 build ext                # Build extensions separately
    $0 test                     # Run all tests
    $0 test omni_http           # Run tests matching 'omni_http'
    $0 docker build             # Build Docker image
    $0 docker run               # Run container
    $0 ext list                 # List all extensions
    $0 ext build omni_http      # Build specific extension
    $0 clean                    # Clean build artifacts

${GREEN}ENVIRONMENT VARIABLES:${NC}
    ${CYAN}POSTGRES_VERSION${NC}    PostgreSQL version (default: 17)
    ${CYAN}CMAKE_BUILD_TYPE${NC}    CMake build type (default: Release)
    ${CYAN}PARALLEL_JOBS${NC}       Number of parallel jobs (default: nproc)

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -t|--type)
                CMAKE_BUILD_TYPE="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            -*)
                error_exit "Unknown option: $1"
                ;;
            *)
                break
                ;;
        esac
    done
    
    # Store remaining arguments
    COMMAND="${1:-help}"
    shift || true
    ARGS=("$@")
}

# Main function
main() {
    parse_args "$@"
    
    log "INFO" "Omnigres Development Script"
    log "DEBUG" "Project root: ${PROJECT_ROOT}"
    log "DEBUG" "Build directory: ${BUILD_DIR}"
    log "DEBUG" "Parallel jobs: ${PARALLEL_JOBS}"
    log "DEBUG" "CMake build type: ${CMAKE_BUILD_TYPE}"
    
    case $COMMAND in
        "setup")
            check_dependencies
            setup_environment
            ;;
        "build")
            local build_target="${ARGS[0]:-all}"
            case $build_target in
                "all")
                    check_dependencies
                    build_project
                    ;;
                "ext"|"extensions")
                    check_dependencies
                    build_extensions_separately
                    ;;
                "clean")
                    clean_build
                    check_dependencies
                    build_project
                    ;;
                *)
                    check_dependencies
                    build_project "$build_target"
                    ;;
            esac
            ;;
        "test")
            run_tests "${ARGS[0]:-}"
            ;;
        "clean")
            clean_build
            ;;
        "docker")
            docker_operations "${ARGS[0]:-}"
            ;;
        "ext"|"extensions")
            manage_extensions "${ARGS[0]:-list}" "${ARGS[1]:-}"
            ;;
        "help"|"--help"|"-h")
            show_help
            ;;
        *)
            log "ERROR" "Unknown command: $COMMAND"
            show_help
            exit 1
            ;;
    esac
}

# Trap to handle script interruption
trap 'log "WARN" "Script interrupted"; exit 130' INT TERM

# Run main function with all arguments
main "$@"