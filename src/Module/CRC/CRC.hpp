/*!
 * \file
 * \brief Adds/builds and checks a Cyclic Redundancy Check (CRC) for a set of information bits.
 *
 * \section LICENSE
 * This file is under MIT license (https://opensource.org/licenses/MIT).
 */
#ifndef CRC_HPP_
#define CRC_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <mipp.h>

#include "Tools/Exception/exception.hpp"

#include "Module/Module.hpp"

namespace aff3ct
{
namespace module
{
/*!
 * \class CRC_i
 *
 * \brief Adds/builds and checks a Cyclic Redundancy Check (CRC) for a set of information bits.
 *
 * \tparam B: type of the bits in the CRC.
 *
 * Please use CRC for inheritance (instead of CRC_i).
 */
template <typename B = int>
class CRC_i : public Module
{
protected:
	const int K; /*!< Number of information bits (the CRC bits are not included in K) */

public:
	/*!
	 * \brief Constructor.
	 *
	 * \param K:        number of information bits (the CRC bits are included in K).
	 * \param n_frames: number of frames to process in the CRC.
	 * \param name:     CRC's name.
	 */
	CRC_i(const int K, const int n_frames = 1, const std::string name = "CRC_i")
	: Module(n_frames, name), K(K)
	{
		if (K <= 0)
		{
			std::stringstream message;
			message << "'K' has to be greater than 0 ('K' = " << K << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}
	}

	/*!
	 * \brief Destructor.
	 */
	virtual ~CRC_i()
	{
	}

	int get_K() const
	{
		return this->K;
	}

	/*!
	 * \brief Gets the size of the CRC (the number of bits for the CRC signature).
	 *
	 * \return the size of the CRC.
	 */
	virtual int get_size() const = 0;

	/*!
	 * \brief Computes and adds the CRC in the vector of information bits (the CRC bits are often put at the end of the
	 *        vector).
	 *
	 * \param U_K: a vector (size = K - CRC<B>::size()) containing the information bits, adds "CRC<B>::size()" bits in
	 *             U_K.
	 */
	void build(const mipp::vector<B>& U_K1, mipp::vector<B>& U_K2)
	{
		if (this->K * this->n_frames != (int)U_K1.size())
		{
			std::stringstream message;
			message << "'U_K1.size()' has to be equal to 'K' * 'n_frames' ('U_K1.size()' = " << U_K1.size()
			        << ", 'K' = " << this->K << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if ((this->K + this->get_size()) * this->n_frames != (int)U_K2.size())
		{
			std::stringstream message;
			message << "'U_K2.size()' has to be equal to ('K' + 'get_size()') * 'n_frames' ('U_K2.size()' = "
			        << U_K2.size() << ", 'K' = " << this->K << ", 'get_size()' = " << this->get_size()
			        << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->build(U_K1.data(), U_K2.data());
	}

	virtual void build(const B *U_K1, B *U_K2)
	{
		for (auto f = 0; f < this->n_frames; f++)
			this->_build(U_K1 + f *  this->K,
			             U_K2 + f * (this->K + this->get_size()),
			             f);
	}

	void extract(const mipp::vector<B>& V_K1, mipp::vector<B>& V_K2)
	{
		if ((this->K + this->get_size()) * this->n_frames != (int)V_K1.size())
		{
			std::stringstream message;
			message << "'V_K1.size()' has to be equal to ('K' + 'get_size()') * 'n_frames' ('V_K1.size()' = "
			        << V_K1.size() << ", 'K' = " << this->K << ", 'get_size()' = " << this->get_size()
			        << ", 'n_frames' = " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		if (this->K * this->n_frames != (int)V_K2.size())
		{
			std::stringstream message;
			message << "'V_K2.size()' has to be equal to 'K' * 'n_frames' ('V_K2.size()' = " << V_K2.size()
			        << ", 'K' = " << this->K << ", 'n_frames' =  " << this->n_frames << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		this->extract(V_K1.data(), V_K2.data());
	}

	virtual void extract(const B *V_K1, B *V_K2)
	{
		for (auto f = 0; f < this->n_frames; f++)
			this->_extract(V_K1 + f * (this->K + this->get_size()),
			               V_K2 + f *  this->K,
			               f);
	}

	/*!
	 * \brief Checks if the CRC is verified or not.
	 *
	 * \param V_K:      a vector containing information bits plus the CRC bits.
	 * \param n_frames: you should not use this parameter unless you know what you are doing, this parameter
	 *                  redefine the number of frames to check specifically in this method.
	 *
	 * \return true if the CRC is verified, false otherwise.
	 */
	bool check(const mipp::vector<B>& V_K, const int n_frames = -1)
	{
		if (n_frames <= 0 && n_frames != -1)
		{
			std::stringstream message;
			message << "'n_frames' has to be greater than 0 or equal to -1 ('n_frames' = " << n_frames << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		if ((this->K + (int)this->get_size()) *       n_frames != (int)V_K.size() &&
		    (this->K + (int)this->get_size()) * this->n_frames != (int)V_K.size())
		{
			std::stringstream message;
			message << "'V_K.size()' has to be equal to ('K' + 'size') * 'n_frames' ('V_K.size()' = " << V_K.size()
			        << ", 'K' = " << this->K
			        << ", 'n_frames' = " << (n_frames != -1 ? n_frames : this->n_frames) << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		return this->check(V_K.data(), n_frames);
	}

	virtual bool check(const B *V_K, const int n_frames = -1)
	{
		const int real_n_frames = (n_frames != -1) ? n_frames : this->n_frames;

		auto f = 0;
		while (f < real_n_frames && this->_check(V_K + f * (this->K + this->get_size()), f))
			f++;

		return f == real_n_frames;
	}

	/*!
	 * \brief Checks if the CRC is verified or not (works on packed bits).
	 *
	 * \param V_K:      a vector of packed bits containing information bits plus the CRC bits.
	 * \param n_frames: you should not use this parameter unless you know what you are doing, this parameter
	 *                  redefine the number of frames to check specifically in this method.
	 *
	 * \return true if the CRC is verified, false otherwise.
	 */
	bool check_packed(const mipp::vector<B>& V_K, const int n_frames = -1)
	{
		if (n_frames <= 0 && n_frames != -1)
		{
			std::stringstream message;
			message << "'n_frames' has to be greater than 0 or equal to -1 ('n_frames' = " << n_frames << ").";
			throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
		}

		if ((this->K + (int)this->get_size()) *       n_frames != (int)V_K.size() &&
		    (this->K + (int)this->get_size()) * this->n_frames != (int)V_K.size())
		{
			std::stringstream message;
			message << "'V_K.size()' has to be equal to ('K' + 'size') * 'n_frames' ('V_K.size()' = " << V_K.size()
			        << ", 'K' = " << this->K
			        << ", 'n_frames' = " << (n_frames != -1 ? n_frames : this->n_frames) << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		return this->check_packed(V_K.data(), n_frames);
	}

	bool check_packed(const B *V_K, const int n_frames = -1)
	{
		const int real_n_frames = (n_frames != -1) ? n_frames : this->n_frames;

		auto f = 0;
		while (f < real_n_frames && this->_check_packed(V_K + f * (this->K + this->get_size()), f))
			f++;

		return f == real_n_frames;
	}

protected:
	virtual void _build(const B *U_K1, B *U_K2, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}

	virtual void _extract(const B *V_K1, B *V_K2, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
	}

	virtual bool _check(const B *V_K, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
		return false;
	}

	virtual bool _check_packed(const B *V_K, const int frame_id)
	{
		throw tools::unimplemented_error(__FILE__, __LINE__, __func__);
		return false;
	}
};
}
}

#include "SC_CRC.hpp"

#endif
