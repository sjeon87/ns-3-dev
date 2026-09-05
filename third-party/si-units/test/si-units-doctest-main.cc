#include "si-units-doctest.cc"

#include <iostream>

int
main(int argc, char** argv)
{
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    const auto res = context.run();

    if (context.shouldExit())
    {
        return res;
    }

    if (res == 0)
    {
        std::cout << "All tests passed!" << std::endl;
    }
    else
    {
        std::cout << "Some tests failed!" << std::endl;
    }
    return res;
}
