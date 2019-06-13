#include "LexStop2LAnalysis/addFakeFactor.h"

//stl
#include <iostream>
using std::cout;

int main() {
    cout << "INFO :: Running tests on addFakeFactor\n";
    ////////////////////////////////////////////////////////////////////////////
    // Setup
    bool failedTests = false;
    int n_fails = 0;
    int n_tests = 0;

    ////////////////////////////////////////////////////////////////////////////
    // Test argument parser
    bool failedTest1 = false;

    if (failedTest1) {
        cout << "WARNING :: failed test " << ++n_tests << '\n';
        failedTests = true;
        n_fails++;
    } else {
        cout << "INFO :: passed test " << ++n_tests << '\n';
    }

    ////////////////////////////////////////////////////////////////////////////
    // Conclusion
    if (failedTests) {
        cout << "WARNING :: Failed " << n_fails << " out of " << n_tests << '\n';
        return 1;
    } else {
        cout << "INFO :: Passed all " << n_tests << " tests\n";
        return 0;
    }
}
