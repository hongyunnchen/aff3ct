#ifndef SC_QUANTIZER_HPP_
#define SC_QUANTIZER_HPP_

#ifdef SYSTEMC_MODULE
#include <vector>
#include <string>
#include <sstream>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <mipp.h>

#include "Tools/Exception/exception.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float, typename Q = int>
class SC_Quantizer;

template <typename R, typename Q>
class SC_Quantizer_module : public sc_core::sc_module
{
public:
	tlm_utils::simple_target_socket   <SC_Quantizer_module> s_in;
	tlm_utils::simple_initiator_socket<SC_Quantizer_module> s_out;

private:
	SC_Quantizer<R,Q> &quantizer;
	mipp::vector<Q> Y_N2;

public:
	SC_Quantizer_module(SC_Quantizer<R,Q> &quantizer, const sc_core::sc_module_name name = "SC_Quantizer_module")
	: sc_module(name), s_in("s_in"), s_out("s_out"),
	  quantizer(quantizer),
	  Y_N2(quantizer.get_N() * quantizer.get_n_frames())
	{
		s_in.register_b_transport(this, &SC_Quantizer_module::b_transport);
	}

	const mipp::vector<Q>& get_Y_N()
	{
		return Y_N2;
	}

private:
	void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& t)
	{
		if (quantizer.get_N() * quantizer.get_n_frames() != (int)(trans.get_data_length() / sizeof(R)))
		{
			std::stringstream message;
			message << "'quantizer.get_N()' * 'quantizer.get_n_frames()' has to be equal to "
			        << "'trans.get_data_length()' / 'sizeof(R)' ('quantizer.get_N()' = " << quantizer.get_N()
			        << ", 'quantizer.get_n_frames()' = " << quantizer.get_n_frames()
			        << ", 'trans.get_data_length()' = " << trans.get_data_length()
			        << ", 'sizeof(R)' = " << sizeof(R) << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		const auto Y_N1 = (R*)trans.get_data_ptr();

		quantizer.process(Y_N1, Y_N2.data());

		tlm::tlm_generic_payload payload;
		payload.set_data_ptr((unsigned char*)Y_N2.data());
		payload.set_data_length(Y_N2.size() * sizeof(Q));

		sc_core::sc_time zero_time(sc_core::SC_ZERO_TIME);
		s_out->b_transport(payload, zero_time);
	}
};

template <typename R, typename Q>
class SC_Quantizer : public Quantizer_i<R,Q>
{
public:
	SC_Quantizer_module<R,Q> *sc_module;

public:
	SC_Quantizer(const int N, const int n_frames = 1, const std::string name = "SC_Quantizer")
	: Quantizer_i<R,Q>(N, n_frames, name), sc_module(nullptr) {}

	virtual ~SC_Quantizer()
	{
		if (sc_module != nullptr) { delete sc_module; sc_module = nullptr; }
	};

	void create_sc_module()
	{
		if (sc_module != nullptr) { delete sc_module; sc_module = nullptr; }
		this->sc_module = new SC_Quantizer_module<R,Q>(*this, this->name.c_str());
	}
};

template <typename R = float, typename Q = int>
using Quantizer = SC_Quantizer<R,Q>;
}
}
#else
#include "SPU_Quantizer.hpp"
#endif

#endif /* SC_QUANTIZER_HPP_ */
