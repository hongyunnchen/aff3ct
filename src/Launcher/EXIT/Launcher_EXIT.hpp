#ifndef LAUNCHER_EXIT_HPP_
#define LAUNCHER_EXIT_HPP_

#include "Tools/Codec/Codec_SISO.hpp"

#include "../Launcher.hpp"

namespace aff3ct
{
namespace launcher
{
template <typename B = int, typename R = float, typename Q = R>
class Launcher_EXIT : public Launcher<B,R,Q>
{
protected:
	tools::Codec_SISO<B,R> *codec;

public:
	Launcher_EXIT(const int argc, const char **argv, std::ostream &stream = std::cout);
	virtual ~Launcher_EXIT();

protected:
	virtual void build_args();
	virtual void store_args();

	virtual std::vector<std::pair<std::string,std::string>> header_simulation();
	virtual std::vector<std::pair<std::string,std::string>> header_encoder   ();
	virtual std::vector<std::pair<std::string,std::string>> header_decoder   ();
};
}
}

#endif /* LAUNCHER_EXIT_HPP_ */
