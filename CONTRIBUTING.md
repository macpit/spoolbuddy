# Contributing to SpoolBuddy

Thank you for your interest in contributing to SpoolBuddy! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Code Style](#code-style)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md) to keep our community welcoming and respectful.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/spoolbuddy.git
   cd spoolbuddy
   ```
3. **Add the upstream remote**:
   ```bash
   git remote add upstream https://github.com/maziggy/spoolbuddy.git
   ```

## Development Setup

### Prerequisites

- Python 3.10+ (3.11/3.12 recommended)
- Node.js 18+
- npm
- Rust (for firmware development)

### Backend Setup

```bash
cd backend

# Create virtual environment
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Install pre-commit hooks
pip install pre-commit
pre-commit install

# Run backend
python main.py
```

### Frontend Setup

```bash
cd frontend

# Install dependencies
npm install

# Run development server
npm run dev
```

The frontend will be available at `http://localhost:5173` and will proxy API requests to the backend.

### Firmware Setup

```bash
cd firmware

# Install Rust and ESP-IDF toolchain
# See firmware/README.md for detailed instructions

# Build firmware
cargo build --release
```

## Making Changes

1. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bug-fix
   ```

2. **Make your changes** following our code style guidelines

3. **Test your changes** thoroughly

4. **Commit your changes** with clear, descriptive messages:
   ```bash
   git commit -m "Add feature: description of what you added"
   ```

### Branch Naming

- `feature/` — New features
- `fix/` — Bug fixes
- `docs/` — Documentation changes
- `refactor/` — Code refactoring
- `test/` — Test additions or fixes

## Code Style

### Backend (Python)

We use [Ruff](https://github.com/astral-sh/ruff) for linting and formatting:

```bash
cd backend

# Check linting
ruff check .

# Auto-fix issues
ruff check --fix .

# Format code
ruff format .
```

### Frontend (TypeScript/Preact)

We use ESLint:

```bash
cd frontend

# Lint
npm run lint

# Type check (if available)
npm run type-check
```

### Firmware (Rust)

We use standard Rust formatting:

```bash
cd firmware

# Format code
cargo fmt

# Check lints
cargo clippy
```

### Pre-commit Hooks

Pre-commit hooks run automatically on `git commit`. To run manually:

```bash
pre-commit run --all-files
```

## Testing

### Backend Tests

```bash
cd backend

# Run all tests
python -m pytest tests/ -v

# Run with coverage
python -m pytest tests/ --cov=. --cov-report=html

# Run specific test file
python -m pytest tests/unit/test_example.py -v
```

### Frontend Tests

```bash
cd frontend

# Run tests
npm run test:run

# Run with watch mode
npm run test

# Run with coverage
npm run test:coverage
```

## Submitting Changes

1. **Push your branch** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Create a Pull Request** on GitHub:
   - Use a clear, descriptive title
   - Fill out the PR template completely
   - Link any related issues
   - Include screenshots for UI changes

3. **Wait for review** — maintainers will review your PR and may request changes

### PR Guidelines

- Keep PRs focused and reasonably sized
- One feature or fix per PR
- Update documentation if needed
- Add tests for new functionality
- Ensure all tests pass
- Follow the existing code style

## Reporting Bugs

Open an issue and include:

- Clear description of the bug
- Steps to reproduce
- Expected vs actual behavior
- Your environment (OS, Python version, browser)
- Printer model and firmware version (if applicable)
- Relevant logs

## Requesting Features

Open an issue and include:

- Clear description of the feature
- Use case / problem it solves
- Proposed solution
- Alternatives considered

## Questions?

- Check the [Documentation](https://wiki.spoolbuddy.cool)
- Open a [Discussion](https://github.com/maziggy/spoolbuddy/discussions)
- Join our [Discord](https://discord.gg/3XFdHBkF)
- Review existing [Issues](https://github.com/maziggy/spoolbuddy/issues)

---

Thank you for contributing to SpoolBuddy!
