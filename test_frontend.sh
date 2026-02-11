#!/bin/sh

cd frontend
npx tsc --noEmit
npm run lint
npm run test:run
cd ..
