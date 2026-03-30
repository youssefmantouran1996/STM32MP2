/**
 * @file  test_main.cpp
 * @brief Unit tests for the A35 application (host-compiled, no board needed).
 *
 * Uses the Catch2 framework (header-only v2). To enable:
 *   cmake -B build/test -S cortex-a35 -DBUILD_TESTS=ON
 *   cmake --build build/test
 *   ctest --test-dir build/test
 */

#define CATCH_CONFIG_MAIN
// #include <catch2/catch.hpp>   // Uncomment once Catch2 is added as a dependency

// Placeholder test — replace with real unit tests for your application logic
int main() {
    // Catch2 auto-generates main when CATCH_CONFIG_MAIN is defined.
    return 0;
}

// TEST_CASE("RPMsg channel reports failure when device is missing") {
//     ipc::RpmsgChannel ch("/dev/ttyRPMSG_NONEXISTENT");
//     REQUIRE_FALSE(ch.open());
// }
