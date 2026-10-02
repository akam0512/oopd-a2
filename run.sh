#!/bin/sh
docker run --rm -v "$(pwd)":/work -w /work gcc:13 sh -c "
  g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp tests/test_main.cpp -o /tmp/t && /tmp/t &&
  g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp examples/demo.cpp -o /tmp/d && /tmp/d > /dev/null && echo demo ok
"
