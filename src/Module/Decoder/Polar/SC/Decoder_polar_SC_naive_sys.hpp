#ifndef DECODER_POLAR_SC_NAIVE_SYS_
#define DECODER_POLAR_SC_NAIVE_SYS_

#include <vector>
#include <mipp.h>

#include "Tools/Algo/Tree/Binary_tree.hpp"
#include "Tools/Code/Polar/decoder_polar_functions.h"

#include "Decoder_polar_SC_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename B = int, typename R = float, tools::proto_f<  R> F = tools::f_LLR,
                                                tools::proto_g<B,R> G = tools::g_LLR,
                                                tools::proto_h<B,R> H = tools::h_LLR>
class Decoder_polar_SC_naive_sys : public Decoder_polar_SC_naive<B,R,F,G,H>
{
public:
	Decoder_polar_SC_naive_sys(const int& K, const int& N, const mipp::vector<B>& frozen_bits, 
	                           const int n_frames = 1, const std::string name = "Decoder_polar_SC_naive_sys");
	virtual ~Decoder_polar_SC_naive_sys();

protected:
	void _store(B *V_K) const;
};
}
}

#include "Decoder_polar_SC_naive_sys.hxx"

#endif /* DECODER_POLAR_SC_NAIVE_SYS_ */
