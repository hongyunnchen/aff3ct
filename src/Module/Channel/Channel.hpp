/*!
 * \file
 * \brief The Channel is the physical transmission medium.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <mipp.h>

#include "Tools/Exception/exception.hpp"

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class Channel_i
 *
 * \brief The Channel is the physical transmission medium.
 *
 * \tparam R: type of the reals (floating-point representation) in the Channel.
 *
 * Please use Channel for inheritance (instead of Channel_i).
 */
template <typename R = float>
class Channel_i : public Module
{
protected:
	const int N;     /*!< Size of one frame (= number of bits in one frame) */
	      R   sigma; /*!< Sigma^2, the noise variance */

	mipp::vector<R> noise;

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param N:        size of one frame.
	 * \param n_frames: number of frames to process in the Channel.
	 * \param name:     Channel's name.
	 */
	Channel_i(const int N, const R sigma, const int n_frames = 1, const std::string name = "Channel_i")
	: Module(n_frames, name), N(N), sigma(sigma), noise(this->N * this->n_frames, 0)
	{
		if (N <= 0)
		{
			std::stringstream message;
			message << "'N' has to be greater than 0 ('N' = " << N << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		if (sigma <= 0)
		{
			std::stringstream message;
			message << "'sigma' has to be greater than 0 ('sigma' = " << sigma << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}
	}

	/*!
	 * \brief Destructor.
	 */
	virtual ~Channel_i()
	{
	}

	int get_N() const
	{
		return this->N;
	}

	R get_sigma() const
	{
		return this->sigma;
	}

	const mipp::vector<R>& get_noise() const
	{
		return noise;
	}

	virtual void set_sigma(const R sigma)
	{
		if (sigma <= 0)
		{
			std::stringstream message;
			message << "'sigma' has to be greater than 0 ('sigma' = " << sigma << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		this->sigma = sigma;
	}

	/*!
	 * \brief Adds the noise to a perfectly clear signal.
	 *
	 * \param X_N: a perfectly clear message.
	 * \param Y_N: a noisy signal.
	 */
	void add_noise(const mipp::vector<R>& X_N, mipp::vector<R>& Y_N)
	{
		if (X_N.size() != Y_N.size())
		{
			std::stringstream message;
			message << "'X_N.size()' has to be equal to 'Y_N.size()' ('X_N.size()' = "
			        << X_N.size() << ", 'Y_N.size()' = " << Y_N.size() << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)X_N.size())
		{
			std::stringstream message;
			message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = "
			        << X_N.size() << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)Y_N.size())
		{
			std::stringstream message;
			message << "'Y_N.size()' has to be equal to 'N' * 'n_frames' ('Y_N.size()' = "
			        << Y_N.size() << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->add_noise(X_N.data(), Y_N.data());
	}

	virtual void add_noise(const R *X_N, R *Y_N)
	{
		for (auto f = 0; f < this->n_frames; f++)
			this->_add_noise(X_N + f * this->N,
			                 Y_N + f * this->N,
			                 f);
	}

	/*!
	 * \brief Adds the noise to a perfectly clear signal.
	 *
	 * \param X_N: a perfectly clear message.
	 * \param Y_N: a noisy signal.
	 * \param H_N: the channel gains.
	 */
	void add_noise(const mipp::vector<R>& X_N, mipp::vector<R>& Y_N, mipp::vector<R>& H_N)
	{
		if (X_N.size() != Y_N.size() || Y_N.size() != H_N.size())
		{
			std::stringstream message;
			message << "'X_N.size()' has to be equal to 'Y_N.size()' and 'H_N.size()' ('X_N.size()' = "
			        << X_N.size() << ", 'Y_N.size()' = " << Y_N.size() << ", 'H_N.size()' = " << H_N.size() << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)X_N.size())
		{
			std::stringstream message;
			message << "'X_N.size()' has to be equal to 'N' * 'n_frames' ('X_N.size()' = "
			        << X_N.size() << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)Y_N.size())
		{
			std::stringstream message;
			message << "'Y_N.size()' has to be equal to 'N' * 'n_frames' ('Y_N.size()' = "
			        << Y_N.size() << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->N * this->n_frames != (int)H_N.size())
		{
			std::stringstream message;
			message << "'H_N.size()' has to be equal to 'N' * 'n_frames' ('H_N.size()' = "
			        << H_N.size() << ", 'N' = " << this->N << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->add_noise(X_N.data(), Y_N.data(), H_N.data());
	}

	virtual void add_noise(const R *X_N, R *Y_N, R *H_N)
	{
		for (auto f = 0; f < this->n_frames; f++)
			this->_add_noise(X_N + f * this->N,
			                 Y_N + f * this->N,
			                 H_N + f * this->N,
			                 f);
	}

protected:
	virtual void _add_noise(const R *X_N, R *Y_N, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}

	virtual void _add_noise(const R *X_N, R *Y_N, R *H_N, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}
};
}
}

#include "SC_Channel.hpp"

#endif /* CHANNEL_HPP_ */
