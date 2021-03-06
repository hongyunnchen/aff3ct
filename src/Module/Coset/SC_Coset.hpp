#ifndef SC_COSET_HPP_
#define SC_COSET_HPP_

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
template <typename B, typename D>
class SC_Coset;

template <typename B, typename D>
class SC_Coset_module : public sc_core::sc_module
{
public:
	tlm_utils::simple_target_socket   <SC_Coset_module> s_in1;
	tlm_utils::simple_target_socket   <SC_Coset_module> s_in2;
	tlm_utils::simple_initiator_socket<SC_Coset_module> s_out;

private:
	SC_Coset<B,D> &coset;
	B *ref;
	mipp::vector<D> out_data;

public:
	SC_Coset_module(SC_Coset<B,D> &coset, const sc_core::sc_module_name name = "SC_Coset_module")
	: sc_module(name), s_in1("s_in1"), s_in2("s_in2"), s_out("s_out"),
	  coset   (coset),
	  ref     (nullptr),
	  out_data(coset.get_size() * coset.get_n_frames())
	{
		s_in1.register_b_transport(this, &SC_Coset_module::b_transport_ref);
		s_in2.register_b_transport(this, &SC_Coset_module::b_transport_data);
	}

	const mipp::vector<D>& get_out_data()
	{
		return out_data;
	}

private:
	void b_transport_ref(tlm::tlm_generic_payload& trans, sc_core::sc_time& t)
	{
		if (coset.get_size() * coset.get_n_frames() != (int)(trans.get_data_length() / sizeof(B)))
		{
			std::stringstream message;
			message << "'coset.get_size()' * 'coset.get_n_frames()' has to be equal to 'trans.get_data_length()' / "
			           "'sizeof(B)' ('coset.get_size()' = " << coset.get_size()
			        << ", 'coset.get_n_frames()' = " << coset.get_n_frames() << ", 'trans.get_data_length()' = "
			        << trans.get_data_length() << ", 'sizeof(B)' = " << sizeof(B) << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		ref = (B*)trans.get_data_ptr();
	}

	void b_transport_data(tlm::tlm_generic_payload& trans, sc_core::sc_time& t)
	{
		if (ref == nullptr)
			throw tools::runtime_error(__FILE__, __LINE__, __func__, "'ref' pointer can't be NULL.");

		if (coset.get_size() * coset.get_n_frames() != (int)(trans.get_data_length() / sizeof(D)))
		{
			std::stringstream message;
			message << "'coset.get_size()' * 'coset.get_n_frames()' has to be equal to 'trans.get_data_length()' / "
			           "'sizeof(D)' ('coset.get_size()' = " << coset.get_size()
			        << ", 'coset.get_n_frames()' = " << coset.get_n_frames() << ", 'trans.get_data_length()' = "
			        << trans.get_data_length() << ", 'sizeof(D)' = " << sizeof(D) << ").";
			throw tools::length_error(__FILE__, __LINE__, __func__, message.str());
		}

		const auto in_data = (D*)trans.get_data_ptr();

		coset.apply(ref, in_data, out_data.data());

		tlm::tlm_generic_payload payload;
		payload.set_data_ptr((unsigned char*)out_data.data());
		payload.set_data_length(out_data.size() * sizeof(D));

		sc_core::sc_time zero_time(sc_core::SC_ZERO_TIME);
		s_out->b_transport(payload, zero_time);
	}
};

template <typename B, typename D>
class SC_Coset : public Coset_i<B,D>
{
public:
	SC_Coset_module<B,D> *sc_module;

public:
	SC_Coset(const int size, const int n_frames = 1, const std::string name = "SC_Coset")
	: Coset_i<B,D>(size, n_frames, name), sc_module(nullptr) {}

	virtual ~SC_Coset()
	{ 
		if (sc_module != nullptr) { delete sc_module; sc_module = nullptr; }
	}

	void create_sc_module()
	{
		if (sc_module != nullptr) { delete sc_module; sc_module = nullptr; }
		this->sc_module = new SC_Coset_module<B,D>(*this, this->name.c_str());
	}
};

template <typename B, typename D>
using Coset = SC_Coset<B,D>;
}
}
#else
#include "SPU_Coset.hpp"
#endif

#endif /* SC_COSET_HPP_ */
