#include "tests/test_support.hpp"

namespace {

    using namespace devdock::test_support;

    bool& reached_end_of_required_test() {
        static bool reached = false;

        return reached;
    }

    bool& reached_end_of_checked_test() {
        static bool reached = false;

        return reached;
    }

    void abandons_the_test_after_a_failed_requirement() {
        REQUIRE(1 == 2);

        reached_end_of_required_test() = true;
    }

    void continues_the_test_after_a_failed_check() {
        CHECK(1 == 2);

        reached_end_of_checked_test() = true;
    }

    void a_failed_requirement_stops_the_test_and_is_reported() {
        const int before = failure_count();

        abandons_the_test_after_a_failed_requirement();

        const int recorded = failure_count() - before;

        failure_count() = before;

        CHECK(!reached_end_of_required_test());
        CHECK(recorded == 1);
    }

    void a_failed_check_lets_the_test_continue() {
        const int before = failure_count();

        continues_the_test_after_a_failed_check();

        const int recorded = failure_count() - before;

        failure_count() = before;

        CHECK(reached_end_of_checked_test());
        CHECK(recorded == 1);
    }

}

int main() {
    std::cerr
        << "Note: the two failure lines above or below are deliberate; "
           "this suite exercises the assertion failure paths.\n";

    return run_suite(
        "test support",
        {
            a_failed_requirement_stops_the_test_and_is_reported,
            a_failed_check_lets_the_test_continue,
        }
    );
}
