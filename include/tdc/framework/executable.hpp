#pragma once

#include <tdc/framework/algorithm.hpp>

namespace tdc::framework {

class Executable : public Algorithm {
public:
    // execute algorithm as a standalone application
	virtual void execute(int& in, int& out) = 0;
};

}
