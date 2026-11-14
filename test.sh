#!/bin/sh
set -eu
[ -x bin/restaurant_server ] && [ -x bin/student_server ] && [ -x bin/student_client ] && [ -x bin/restaurant_admin ]
echo "PASS: all four binaries exist and are executable."
echo "Run the two servers in separate terminals, then use student_client and restaurant_admin."
