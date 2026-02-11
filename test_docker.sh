#!/bin/bash
#
# Docker Test Suite for SpoolBuddy
# Runs build verification, unit tests, and integration tests in Docker
#

set -e

# Configuration
PORT=${PORT:-3000}

# Enable BuildKit for better caching and parallel builds
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Track results
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=""

print_header() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

print_failure() {
    echo -e "${RED}✗ $1${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
    FAILED_TESTS="${FAILED_TESTS}\n  - $1"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

cleanup() {
    print_info "Cleaning up test containers..."
    sudo docker compose -f docker-compose.test.yml down -v --remove-orphans 2>/dev/null || true
    sudo docker compose down -v --remove-orphans 2>/dev/null || true
}

# Cleanup on exit
trap cleanup EXIT

# Parse arguments
RUN_BUILD=true
RUN_BACKEND=true
RUN_FRONTEND=true
RUN_INTEGRATION=true
FRESH_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            RUN_BACKEND=false
            RUN_FRONTEND=false
            RUN_INTEGRATION=false
            shift
            ;;
        --backend-only)
            RUN_BUILD=false
            RUN_FRONTEND=false
            RUN_INTEGRATION=false
            shift
            ;;
        --frontend-only)
            RUN_BUILD=false
            RUN_BACKEND=false
            RUN_INTEGRATION=false
            shift
            ;;
        --integration-only)
            RUN_BUILD=false
            RUN_BACKEND=false
            RUN_FRONTEND=false
            shift
            ;;
        --skip-build)
            RUN_BUILD=false
            shift
            ;;
        --skip-integration)
            RUN_INTEGRATION=false
            shift
            ;;
        --fresh)
            FRESH_BUILD=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --build-only        Only run build test"
            echo "  --backend-only      Only run backend tests"
            echo "  --frontend-only     Only run frontend tests"
            echo "  --integration-only  Only run integration tests"
            echo "  --skip-build        Skip build test"
            echo "  --skip-integration  Skip integration tests"
            echo "  --fresh             Force fresh build (no cache)"
            echo "  -h, --help          Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Set cache flag based on --fresh option
CACHE_FLAG=""
if [ "$FRESH_BUILD" = true ]; then
    CACHE_FLAG="--no-cache"
    print_info "Fresh build enabled (--no-cache)"
fi

print_header "SpoolBuddy Docker Test Suite"

# ============================================
# Pre-build: Build all test images in parallel
# ============================================
print_header "Pre-building Docker Images"

# Determine which images to build
IMAGES_TO_BUILD=""
if [ "$RUN_BACKEND" = true ]; then
    IMAGES_TO_BUILD="$IMAGES_TO_BUILD backend-test"
fi
if [ "$RUN_FRONTEND" = true ]; then
    IMAGES_TO_BUILD="$IMAGES_TO_BUILD frontend-test"
fi
if [ "$RUN_INTEGRATION" = true ]; then
    IMAGES_TO_BUILD="$IMAGES_TO_BUILD integration"
fi

if [ -n "$IMAGES_TO_BUILD" ]; then
    print_info "Building test images in parallel:$IMAGES_TO_BUILD"
    if sudo docker compose -f docker-compose.test.yml build --parallel $CACHE_FLAG $IMAGES_TO_BUILD; then
        print_success "Test images built successfully"
    else
        print_failure "Test image build failed"
        exit 1
    fi
fi

# ============================================
# Test 1: Docker Build (Production Image)
# ============================================
if [ "$RUN_BUILD" = true ]; then
    print_header "Test 1: Docker Build (Production)"
    print_info "Building production Docker image..."

    if sudo docker build -t spoolbuddy:test . $CACHE_FLAG --progress=plain; then
        print_success "Production image builds successfully"

        # Verify image has expected labels/structure
        print_info "Verifying image structure..."
        if sudo docker run --rm spoolbuddy:test python -c "import main; print('Backend imports OK')"; then
            print_success "Backend module imports correctly"
        else
            print_failure "Backend module import failed"
        fi

        if sudo docker run --rm spoolbuddy:test test -d /app/frontend/dist; then
            print_success "Static files directory exists"
        else
            print_failure "Static files directory missing"
        fi
    else
        print_failure "Production image build failed"
    fi
fi

# ============================================
# Test 2: Backend Unit Tests
# ============================================
if [ "$RUN_BACKEND" = true ]; then
    print_header "Test 2: Backend Unit Tests"
    print_info "Running backend tests..."
    if sudo docker compose -f docker-compose.test.yml run --rm backend-test; then
        print_success "Backend unit tests passed"
    else
        print_failure "Backend unit tests failed"
    fi
fi

# ============================================
# Test 3: Frontend Unit Tests
# ============================================
if [ "$RUN_FRONTEND" = true ]; then
    print_header "Test 3: Frontend Unit Tests"
    print_info "Running frontend tests..."
    if sudo docker compose -f docker-compose.test.yml run --rm frontend-test; then
        print_success "Frontend unit tests passed"
    else
        print_failure "Frontend unit tests failed"
    fi
fi

# ============================================
# Test 4: Integration Tests
# ============================================
if [ "$RUN_INTEGRATION" = true ]; then
    print_header "Test 4: Integration Tests"
    print_info "Starting application container..."

    # Start the integration container
    sudo docker compose -f docker-compose.test.yml up --remove-orphans -d integration

    # Wait for health check
    print_info "Waiting for application to be healthy..."
    RETRIES=30
    while [ $RETRIES -gt 0 ]; do
        if sudo docker compose -f docker-compose.test.yml ps integration | grep -q "healthy"; then
            break
        fi
        sleep 2
        ((RETRIES--))
    done

    if [ $RETRIES -eq 0 ]; then
        print_failure "Application failed to become healthy"
        sudo docker compose -f docker-compose.test.yml logs integration
    else
        print_success "Application is healthy"

        # Run integration tests
        print_info "Running integration tests..."
        echo ""

        # Helper function for API tests with verbose output
        test_api_endpoint() {
            local name="$1"
            local endpoint="$2"
            local expected_pattern="$3"

            echo -n "  GET ${endpoint} ... "
            local start_time=$(date +%s%3N)
            local response=$(curl -s --max-time 5 -w "\n%{http_code}" "http://localhost:${PORT}${endpoint}" 2>/dev/null)
            local end_time=$(date +%s%3N)
            local duration=$((end_time - start_time))

            local http_code=$(echo "$response" | tail -n1)
            local body=$(echo "$response" | sed '$d')
            local body_len=${#body}

            if [ "$http_code" = "200" ] && echo "$body" | grep -q "$expected_pattern"; then
                echo -e "${GREEN}PASSED${NC} (${http_code}, ${body_len} bytes, ${duration}ms)"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                echo -e "${RED}FAILED${NC} (HTTP ${http_code}, ${body_len} bytes)"
                TESTS_FAILED=$((TESTS_FAILED + 1))
                FAILED_TESTS="${FAILED_TESTS}\n  - ${name}: HTTP ${http_code}"
            fi
        }

        # API endpoint tests
        test_api_endpoint "Spools API" "/api/spools" "\["
        test_api_endpoint "Printers API" "/api/printers" "\["
        test_api_endpoint "Catalog API" "/api/catalog" "\["
        test_api_endpoint "Colors API" "/api/colors" "\["

        # Static files test
        echo -n "  GET / (static) ... "
        start_time=$(date +%s%3N)
        STATIC_RESPONSE=$(curl -s --max-time 5 -o /dev/null -w "%{http_code}" "http://localhost:${PORT}/" 2>/dev/null || echo "000")
        end_time=$(date +%s%3N)
        duration=$((end_time - start_time))
        if [ "$STATIC_RESPONSE" = "200" ]; then
            echo -e "${GREEN}PASSED${NC} (${STATIC_RESPONSE}, ${duration}ms)"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}FAILED${NC} (HTTP ${STATIC_RESPONSE})"
            TESTS_FAILED=$((TESTS_FAILED + 1))
            FAILED_TESTS="${FAILED_TESTS}\n  - Static files: HTTP ${STATIC_RESPONSE}"
        fi

        echo ""
    fi

    # Cleanup integration containers
    sudo docker compose -f docker-compose.test.yml down -v
fi

# ============================================
# Summary
# ============================================
print_header "Test Summary"

echo ""
echo -e "Tests Passed: ${GREEN}${TESTS_PASSED}${NC}"
echo -e "Tests Failed: ${RED}${TESTS_FAILED}${NC}"

if [ $TESTS_FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed tests:${NC}"
    echo -e "$FAILED_TESTS"
    echo ""
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    echo ""
    exit 0
fi
