#ifndef CODEC_RA_HPP_
#define CODEC_RA_HPP_

#include "../Codec.hpp"

namespace aff3ct
{
namespace tools
{
template <typename B = int, typename Q = float>
class Codec_RA : public Codec<B,Q>
{
public:
	Codec_RA(const parameters& params);
	virtual ~Codec_RA();

	module::Interleaver<int>* build_interleaver(const int tid = 0, const int seed = 0);
	module::Encoder    <B  >* build_encoder    (const int tid = 0, const module::Interleaver<int>* itl = nullptr);
	module::Decoder    <B,Q>* build_decoder    (const int tid = 0, const module::Interleaver<int>* itl = nullptr,
	                                                                     module::CRC        <B  >* crc = nullptr);
};
}
}

#endif /* CODEC_RA_HPP_ */
