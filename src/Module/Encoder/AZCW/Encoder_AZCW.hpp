#ifndef ENCODER_AZCW_HPP_
#define ENCODER_AZCW_HPP_

#include <random>

#include "../Encoder_sys.hpp"

namespace aff3ct
{
namespace module
{
template <typename B = int>
class Encoder_AZCW : public Encoder_sys<B>
{
public:
	Encoder_AZCW(const int K, const int N, const int n_frames = 1, const std::string name = "Encoder_AZCW");
	virtual ~Encoder_AZCW();

	void encode    (const B *U_K, B *X_N); using Encoder_sys<B>::encode;
	void encode_sys(const B *U_K, B *par); using Encoder_sys<B>::encode_sys;
};
}
}

#endif /* ENCODER_AZCW_HPP_ */
