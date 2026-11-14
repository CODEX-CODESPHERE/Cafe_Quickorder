#!/bin/sh
set -eu
CXX="${CXX:-c++}"
mkdir -p bin
FLAGS="-std=c++17 -Wall -Wextra -pedantic -pthread"
$CXX $FLAGS restaurant_server.cpp -o bin/restaurant_server
$CXX $FLAGS student_server.cpp -o bin/student_server
$CXX $FLAGS student_client.cpp -o bin/student_client
$CXX $FLAGS restaurant_admin.cpp -o bin/restaurant_admin
echo "Build successful. Executables are in ./bin/"
