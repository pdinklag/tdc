#pragma once

#include <tdc/framework/algorithm.hpp>

namespace tdc::framework {

/**
 * @brief Base for algorithms that can serve as an entry point for an @ref Application.
 * 
 */
class Executable : public Algorithm {
public:
    /**
     * @brief Executes the algorithm for a given input and output.
     * 
     * @param in the algorithm input
     * @param out the algorithm output
     * @return the return code of the algorithm; zero indicates success
     */
	virtual int execute(int& in, int& out) = 0;
};

}
