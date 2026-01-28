#!/bin/bash

cd ../spoolbuddy-website
git add .
git commit -m "Updated website"
git push

cd ../spoolbuddy-wiki
git add .
git commit -m "Updated website"
git push

cd ../SpoolStation
