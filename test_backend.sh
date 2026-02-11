#!/bin/sh

cd backend
ruff check && ruff format --check

if [ "$1" = "--full" ]; then
  python -m pytest tests/ -v
else
  python -m pytest tests/ -v
fi
cd ..
