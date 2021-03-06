#ifndef FACTORY_CHANNEL_HPP
#define FACTORY_CHANNEL_HPP

#include <string>

#include "Module/Channel/Channel.hpp"

#include "Factory.hpp"

namespace aff3ct
{
namespace tools
{
template <typename R = float>
struct Factory_channel : public Factory
{
	static module::Channel<R>* build(const std::string type,
	                                 const int         N,
	                                 const bool        complex,
	                                 const bool        add_users = false,
	                                 const std::string path      = "",
	                                 const int         seed      = 0,
	                                 const R           sigma     = (R)1,
	                                 const int         n_frames  = 1);
};
}
}

#endif /* FACTORY_CHANNEL_HPP */
