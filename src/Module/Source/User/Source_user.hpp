#ifndef SOURCE_USER_HPP_
#define SOURCE_USER_HPP_

#include <string>
#include <random>
#include <vector>
#include <mipp.h>

#include "../Source.hpp"

namespace aff3ct
{
namespace module
{
template <typename B>
class Source_user : public Source<B>
{
private:
	mipp::vector<mipp::vector<B>> source;
	int src_counter;

public:
	Source_user(const int K, std::string filename, const int n_frames = 1, const std::string name = "Source_user");
	virtual ~Source_user();

protected:
	void _generate(B *U_K, const int frame_id);
};
}
}

#endif /* SOURCE_USER_HPP_ */
