#!/usr/bin/bash
find . -name "*.sh" -exec dos2unix {} + >/dev/null 2>&1
