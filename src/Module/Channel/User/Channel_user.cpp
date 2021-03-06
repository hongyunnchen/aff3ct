#include <fstream>
#include <sstream>

#include "Tools/Exception/exception.hpp"

#include "Channel_user.hpp"

using namespace aff3ct::module;
using namespace aff3ct::tools;

template <typename R>
Channel_user<R>
::Channel_user(const int N, const std::string filename, const bool add_users, const int n_frames,
               const std::string name)
: Channel<R>(N, (R)1, n_frames, name), add_users(add_users), noise_buff(), noise_counter(0)
{
	if (filename.empty())
		throw invalid_argument(__FILE__, __LINE__, __func__, "'filename' should not be empty.");

	std::ifstream file(filename.c_str(), std::ios::binary);
	if (file.is_open())
	{
		unsigned n_fra = 0;
		int fra_size = 0;

		file.read((char*)&n_fra,    sizeof(n_fra));
		file.read((char*)&fra_size, sizeof(fra_size));

		if (n_fra <= 0 || fra_size <= 0)
		{
			std::stringstream message;
			message << "'n_fra' and 'fra_size' have to be bigger than 0 ('n_fra' = "
			        << n_fra << ", 'fra_size' = " << fra_size << ").";
			throw runtime_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->noise_buff.resize(n_fra);
		for (unsigned i = 0; i < (unsigned)n_fra; i++)
			this->noise_buff[i].resize(fra_size);

		if (fra_size == this->N)
		{
			for (unsigned i = 0; i < (unsigned)n_fra; i++)
				file.read(reinterpret_cast<char*>(&this->noise_buff[i][0]), fra_size * sizeof(R));
		}
		else
		{
			file.close();

			std::stringstream message;
			message << "The frame size is wrong (read: " << fra_size << ", expected: " << this->N << ").";
			throw runtime_error(__FILE__, __LINE__, __func__, message.str());
		}

		file.close();
	}
	else
	{
		throw invalid_argument(__FILE__, __LINE__, __func__, "Can't open '" + filename + "' file");
	}
}

template <typename R>
Channel_user<R>
::~Channel_user()
{
}

template <typename R>
void Channel_user<R>
::add_noise(const R *X_N, R *Y_N)
{
	if (add_users && this->n_frames > 1)
	{
		std::fill(Y_N, Y_N + this->N, (R)0);
		for (auto f = 0; f < this->n_frames; f++)
			for (auto i = 0; i < this->N; i++)
				Y_N[i] += X_N[f * this->N +i];

		for (auto f = 0; f < this->n_frames; f++)
		{
			std::copy(this->noise_buff[this->noise_counter].begin(),
			          this->noise_buff[this->noise_counter].end  (),
			          this->noise.data() + f * this->N);

			this->noise_counter = (this->noise_counter +1) % (int)this->noise_buff.size();
		}

		for (auto i = 0; i < this->N; i++)
			Y_N[i] += this->noise[i];
	}
	else
		for (auto f = 0; f < this->n_frames; f++)
		{
			std::copy(this->noise_buff[this->noise_counter].begin(),
			          this->noise_buff[this->noise_counter].end  (),
			          this->noise.data() + f * this->N);

			for (auto i = 0; i < this->N; i++)
				Y_N[f * this->N +i] = X_N[f * this->N +i] + this->noise[f * this->N +i];

			this->noise_counter = (this->noise_counter +1) % (int)this->noise_buff.size();
		}
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::module::Channel_user<R_32>;
template class aff3ct::module::Channel_user<R_64>;
#else
template class aff3ct::module::Channel_user<R>;
#endif
// ==================================================================================== explicit template instantiation
