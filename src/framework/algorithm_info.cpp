#include <tdc/framework/algorithm_info.hpp>

namespace tdc::framework {

nlohmann::json AlgorithmInfo::signature() const {
    nlohmann::json sig;
    sig["__name"] = name_; // TODO: use constant for "__name"
    for(auto& sub : sub_algorithms_) {
        sig[sub.param_name] = sub.info->signature();
    }
    return sig;
}

}
