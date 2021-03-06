#ifdef SYSTEMC

#include <iostream>

#include "Tools/Exception/exception.hpp"
#include "Tools/Display/bash_tools.h"

#include "SC_Simulation_BFER_std.hpp"

using namespace aff3ct::module;
using namespace aff3ct::tools;
using namespace aff3ct::simulation;

template <typename B, typename R, typename Q>
SC_Simulation_BFER_std<B,R,Q>
::SC_Simulation_BFER_std(const parameters& params, Codec<B,Q> &codec)
: Simulation_BFER_std<B,R,Q>(params, codec),

  duplicator{nullptr, nullptr, nullptr},
  dbg_B     {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
  dbg_R     {nullptr, nullptr, nullptr},
  dbg_Q     {nullptr, nullptr, nullptr}
{
	if (this->params.simulation.n_threads > 1)
		throw invalid_argument(__FILE__, __LINE__, __func__, "SystemC simulation does not support multi-threading.");
	if (params.simulation.benchs)
		throw invalid_argument(__FILE__, __LINE__, __func__, "SystemC simulation does not support the bench mode.");

	if (params.simulation.time_report)
		std::clog << format_warning("The time report is not available in the SystemC simulation.") << std::endl;

#ifdef ENABLE_MPI
	std::clog << format_warning("(WW) This simulation is not MPI ready, the same computations will be launched "
	                            + "on each MPI processes.") << std::endl;
#endif
}

template <typename B, typename R, typename Q>
SC_Simulation_BFER_std<B,R,Q>
::~SC_Simulation_BFER_std()
{
}

template <typename B, typename R, typename Q>
void SC_Simulation_BFER_std<B,R,Q>
::_build_communication_chain(const int tid)
{
	Simulation_BFER_std<B,R,Q>::_build_communication_chain(tid);

	// create the sc_module inside the objects of the communication chain
	this->source   [tid]->create_sc_module            ();
	this->crc      [tid]->create_sc_module_build      ();
	this->encoder  [tid]->create_sc_module            ();
	this->puncturer[tid]->create_sc_module_puncturer  ();
	this->puncturer[tid]->create_sc_module_depuncturer();
	this->modem    [tid]->create_sc_module_modulator  ();
	this->modem    [tid]->create_sc_module_filterer   ();
	if (this->params.channel.type.find("RAYLEIGH") != std::string::npos)
	{
		this->channel[tid]->create_sc_module_wg            ();
		this->modem  [tid]->create_sc_module_demodulator_wg();
	}
	else
	{
		this->channel[tid]->create_sc_module            ();
		this->modem  [tid]->create_sc_module_demodulator();
	}
	this->quantizer[tid]->create_sc_module();
	this->decoder  [tid]->create_sc_module();
	this->monitor  [tid]->create_sc_module();
	if (this->params.code.coset)
	{
		this->coset_real[tid]->create_sc_module();
		this->coset_bit [tid]->create_sc_module();
	}
	this->crc[tid]->create_sc_module_extract();

	if (this->params.monitor.err_track_enable)
	{
		const auto &U_K = this->source [tid]->sc_module->get_U_K();
		const auto &X_N = this->encoder[tid]->sc_module->get_X_N();

		this->dumper[tid]->register_data(U_K, "src", false, {});
		this->dumper[tid]->register_data(X_N, "enc", false, {(unsigned)this->params.code.K});
		this->dumper[tid]->register_data(this->channel[tid]->get_noise(), "chn", true, {});
		if (this->interleaver[tid] != nullptr && this->interleaver[tid]->is_uniform())
			this->dumper[tid]->register_data(this->interleaver[tid]->get_lut(), "itl", false, {});
	}
}

template <typename B, typename R, typename Q>
void SC_Simulation_BFER_std<B,R,Q>
::_launch()
{
	this->duplicator[0] = new SC_Duplicator("Duplicator0");
	if (this->params.code.coset)
	{
		this->duplicator[1] = new SC_Duplicator("Duplicator1");
		this->duplicator[2] = new SC_Duplicator("Duplicator2");
	}

	if (this->params.simulation.n_threads == 1 && this->params.simulation.debug)
	{
		const auto dl = this->params.simulation.debug_limit;

		this->dbg_B[0] = new SC_Debug<B>("Generate random bits U_K1...              \nU_K1:\n", dl, "Debug_B0");
		this->dbg_B[1] = new SC_Debug<B>("Build the CRC from U_K1 into U_K2...      \nU_K2:\n", dl, "Debug_B1");
		this->dbg_B[2] = new SC_Debug<B>("Encode U_K2 in X_N1...                    \nX_N1:\n", dl, "Debug_B2");
		this->dbg_B[3] = new SC_Debug<B>("Puncture X_N1 in X_N2...                  \nX_N2:\n", dl, "Debug_B3");
		this->dbg_R[0] = new SC_Debug<R>("Modulate X_N2 in X_N3...                  \nX_N3:\n", dl, "Debug_R0");
		this->dbg_R[1] = new SC_Debug<R>("Add noise from X_N3 to Y_N1...            \nY_N1:\n", dl, "Debug_R1");
		this->dbg_R[2] = new SC_Debug<R>("Filter from Y_N1 to Y_N2...               \nY_N2:\n", dl, "Debug_R2");
		this->dbg_R[3] = new SC_Debug<R>("Demodulate from Y_N3 to Y_N3...           \nY_N3:\n", dl, "Debug_R3");
		this->dbg_Q[0] = new SC_Debug<Q>("Make the quantization from Y_N3 to Y_N4...\nY_N4:\n", dl, "Debug_Q0");
		this->dbg_Q[1] = new SC_Debug<Q>("Depuncture Y_N4 and generate Y_N5...      \nY_N5:\n", dl, "Debug_Q1");
		this->dbg_B[4] = new SC_Debug<B>("Decode Y_N5 and generate V_K1...          \nV_K1:\n", dl, "Debug_B4");
		this->dbg_B[6] = new SC_Debug<B>("Extract CRC bits from V_K1 into V_K2...   \nV_K2:\n", dl, "Debug_B6");

		if (this->params.code.coset)
		{
			this->dbg_Q[2] = new SC_Debug<Q>("Apply the coset approach on Y_N5...       \nY_N5:\n", dl, "Debug_Q2");
			this->dbg_B[5] = new SC_Debug<B>("Apply the coset approach on V_K...        \nV_K: \n", dl, "Debug_B5");
		}
		if (this->params.channel.type.find("RAYLEIGH") != std::string::npos)
			this->dbg_R[4] = new SC_Debug<R>("Channel gains...                          \nH_N: \n", dl, "Debug_R4");

		this->bind_sockets_debug();
		sc_core::sc_start(); // start simulation

		for (auto i = 0; i < 7; i++)
			if (this->dbg_B[i] != nullptr)
			{
				delete this->dbg_B[i];
				this->dbg_B[i] = nullptr;
			}

		for (auto i = 0; i < 5; i++)
			if (this->dbg_R[i] != nullptr)
			{
				delete this->dbg_R[i];
				this->dbg_R[i] = nullptr;
			}

		for (auto i = 0; i < 3; i++)
			if (this->dbg_Q[i] != nullptr)
			{
				delete this->dbg_Q[i];
				this->dbg_Q[i] = nullptr;
			}
	}
	else
	{
		this->bind_sockets();
		sc_core::sc_report_handler::set_actions(sc_core::SC_INFO, sc_core::SC_DO_NOTHING);
		sc_core::sc_start(); // start simulation
	}

	for (auto i = 0; i < 3; i++)
		if (this->duplicator[i] != nullptr)
		{
			delete this->duplicator[i];
			this->duplicator[i] = nullptr;
		}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// /!\ VERY DIRTY WAY TO CREATE A NEW SIMULATION CONTEXT IN SYSTEMC, BE CAREFUL THIS IS NOT IN THE STD! /!\ //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	sc_core::sc_curr_simcontext = new sc_core::sc_simcontext();
	sc_core::sc_default_global_context = sc_core::sc_curr_simcontext;
}

template <typename B, typename R, typename Q>
void SC_Simulation_BFER_std<B,R,Q>
::bind_sockets()
{
	if (this->params.code.coset)
	{
		this->source       [0]->sc_module         ->s_out (this->duplicator[0]                    ->s_in );
		this->duplicator   [0]                    ->s_out1(this->monitor   [0]->sc_module         ->s_in1);
		this->duplicator   [0]                    ->s_out2(this->crc       [0]->sc_module_build   ->s_in );
		this->crc          [0]->sc_module_build   ->s_out (this->duplicator[1]                    ->s_in );
		this->duplicator   [1]                    ->s_out1(this->coset_bit [0]->sc_module         ->s_in1);
		this->duplicator   [1]                    ->s_out2(this->encoder   [0]->sc_module         ->s_in );
		this->encoder      [0]->sc_module         ->s_out (this->duplicator[2]                    ->s_in );
		this->duplicator   [2]                    ->s_out1(this->coset_real[0]->sc_module         ->s_in1);
		this->duplicator   [2]                    ->s_out2(this->puncturer [0]->sc_module_punct   ->s_in );
		this->puncturer    [0]->sc_module_punct   ->s_out (this->modem     [0]->sc_module_mod     ->s_in );
		if (this->params.channel.type.find("RAYLEIGH") != std::string::npos) { // Rayleigh channel
			this->modem    [0]->sc_module_mod     ->s_out (this->channel   [0]->sc_module_wg      ->s_in );
			this->channel  [0]->sc_module_wg      ->s_out1(this->modem     [0]->sc_module_demod_wg->s_in1);
			this->channel  [0]->sc_module_wg      ->s_out2(this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->modem     [0]->sc_module_demod_wg->s_in2);
			this->modem    [0]->sc_module_demod_wg->s_out (this->quantizer [0]->sc_module         ->s_in );
		} else { // additive channel (AWGN, USER, NO)
			this->modem    [0]->sc_module_mod     ->s_out (this->channel   [0]->sc_module         ->s_in );
			this->channel  [0]->sc_module         ->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->modem     [0]->sc_module_demod   ->s_in );
			this->modem    [0]->sc_module_demod   ->s_out (this->quantizer [0]->sc_module         ->s_in );
		}
		this->quantizer    [0]->sc_module         ->s_out (this->puncturer [0]->sc_module_depunct ->s_in );
		this->puncturer    [0]->sc_module_depunct ->s_out (this->coset_real[0]->sc_module         ->s_in2);
		this->coset_real   [0]->sc_module         ->s_out (this->decoder   [0]->sc_module         ->s_in );
		this->decoder      [0]->sc_module         ->s_out (this->coset_bit [0]->sc_module         ->s_in2);
		this->coset_bit    [0]->sc_module         ->s_out (this->crc       [0]->sc_module_extract ->s_in );
		this->crc          [0]->sc_module_extract ->s_out (this->monitor   [0]->sc_module         ->s_in2);
	}
	else // standard simulation
	{
		this->source       [0]->sc_module         ->s_out (this->duplicator[0]                    ->s_in );
		this->duplicator   [0]                    ->s_out1(this->monitor   [0]->sc_module         ->s_in1);
		this->duplicator   [0]                    ->s_out2(this->crc       [0]->sc_module_build   ->s_in );
		this->crc          [0]->sc_module_build   ->s_out (this->encoder   [0]->sc_module         ->s_in );
		this->encoder      [0]->sc_module         ->s_out (this->puncturer [0]->sc_module_punct   ->s_in );
		this->puncturer    [0]->sc_module_punct   ->s_out (this->modem     [0]->sc_module_mod     ->s_in );
		if (this->params.channel.type.find("RAYLEIGH") != std::string::npos) { // Rayleigh channel
			this->modem    [0]->sc_module_mod     ->s_out (this->channel   [0]->sc_module_wg      ->s_in );
			this->channel  [0]->sc_module_wg      ->s_out1(this->modem     [0]->sc_module_demod_wg->s_in1);
			this->channel  [0]->sc_module_wg      ->s_out2(this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->modem     [0]->sc_module_demod_wg->s_in2);
			this->modem    [0]->sc_module_demod_wg->s_out (this->quantizer [0]->sc_module         ->s_in );
		} else { // additive channel (AWGN, USER, NO)
			this->modem    [0]->sc_module_mod     ->s_out (this->channel   [0]->sc_module         ->s_in );
			this->channel  [0]->sc_module         ->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->modem     [0]->sc_module_demod   ->s_in );
			this->modem    [0]->sc_module_demod   ->s_out (this->quantizer [0]->sc_module         ->s_in );
		}
		this->quantizer    [0]->sc_module         ->s_out (this->puncturer [0]->sc_module_depunct ->s_in );
		this->puncturer    [0]->sc_module_depunct ->s_out (this->decoder   [0]->sc_module         ->s_in );
		this->decoder      [0]->sc_module         ->s_out (this->crc       [0]->sc_module_extract ->s_in );
		this->crc          [0]->sc_module_extract ->s_out (this->monitor   [0]->sc_module         ->s_in2);
	}
}

template <typename B, typename R, typename Q>
void SC_Simulation_BFER_std<B,R,Q>
::bind_sockets_debug()
{
	if (this->params.code.coset)
	{
		this->source       [0]->sc_module         ->s_out (this->dbg_B[0]->s_in); this->dbg_B[0]->s_out (this->duplicator[0]                    ->s_in );
		this->duplicator   [0]                                                                  ->s_out1(this->monitor   [0]->sc_module         ->s_in1);
		this->duplicator   [0]                                                                  ->s_out2(this->crc       [0]->sc_module_build   ->s_in );
		this->crc          [0]->sc_module_build   ->s_out (this->dbg_B[1]->s_in); this->dbg_B[1]->s_out (this->duplicator[1]                    ->s_in );
		this->duplicator   [1]                                                                  ->s_out1(this->coset_bit [0]->sc_module         ->s_in1);
		this->duplicator   [1]                                                                  ->s_out2(this->encoder   [0]->sc_module         ->s_in );
		this->encoder      [0]->sc_module         ->s_out (this->dbg_B[2]->s_in); this->dbg_B[2]->s_out (this->duplicator[2]                    ->s_in );
		this->duplicator   [2]                                                                  ->s_out1(this->coset_real[0]->sc_module         ->s_in1);
		this->duplicator   [2]                                                                  ->s_out2(this->puncturer [0]->sc_module_punct   ->s_in );
		this->puncturer    [0]->sc_module_punct   ->s_out (this->dbg_B[3]->s_in); this->dbg_B[3]->s_out (this->modem     [0]->sc_module_mod     ->s_in );
		if (this->params.channel.type.find("RAYLEIGH") != std::string::npos) { // Rayleigh channel
 			this->modem    [0]->sc_module_mod     ->s_out (this->dbg_R[0]->s_in); this->dbg_R[0]->s_out (this->channel   [0]->sc_module_wg      ->s_in );
			this->channel  [0]->sc_module_wg      ->s_out1(this->dbg_R[4]->s_in); this->dbg_R[4]->s_out (this->modem     [0]->sc_module_demod_wg->s_in1);
			this->channel  [0]->sc_module_wg      ->s_out2(this->dbg_R[1]->s_in); this->dbg_R[1]->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->dbg_R[2]->s_in); this->dbg_R[2]->s_out (this->modem     [0]->sc_module_demod_wg->s_in2);
			this->modem    [0]->sc_module_demod_wg->s_out (this->dbg_R[3]->s_in); this->dbg_R[3]->s_out (this->quantizer [0]->sc_module         ->s_in );
		} else { // additive channel (AWGN, USER, NO)
			this->modem    [0]->sc_module_mod     ->s_out (this->dbg_R[0]->s_in); this->dbg_R[0]->s_out (this->channel   [0]->sc_module         ->s_in );
			this->channel  [0]->sc_module         ->s_out (this->dbg_R[1]->s_in); this->dbg_R[1]->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->dbg_R[2]->s_in); this->dbg_R[2]->s_out (this->modem     [0]->sc_module_demod   ->s_in );
			this->modem    [0]->sc_module_demod   ->s_out (this->dbg_R[3]->s_in); this->dbg_R[3]->s_out (this->quantizer [0]->sc_module         ->s_in );
		}
		this->quantizer    [0]->sc_module         ->s_out (this->dbg_Q[0]->s_in); this->dbg_Q[0]->s_out (this->puncturer [0]->sc_module_depunct ->s_in );
		this->puncturer    [0]->sc_module_depunct ->s_out (this->dbg_Q[1]->s_in); this->dbg_Q[1]->s_out (this->coset_real[0]->sc_module         ->s_in2);
		this->coset_real   [0]->sc_module         ->s_out (this->dbg_Q[2]->s_in); this->dbg_Q[2]->s_out (this->decoder   [0]->sc_module         ->s_in );
		this->decoder      [0]->sc_module         ->s_out (this->dbg_B[4]->s_in); this->dbg_B[4]->s_out (this->coset_bit [0]->sc_module         ->s_in2);
		this->coset_bit    [0]->sc_module         ->s_out (this->dbg_B[5]->s_in); this->dbg_B[5]->s_out (this->crc       [0]->sc_module_extract ->s_in );
		this->crc          [0]->sc_module_extract ->s_out (this->dbg_B[6]->s_in); this->dbg_B[6]->s_out (this->monitor   [0]->sc_module         ->s_in2);
	}
	else // standard simulation
	{
		this->source       [0]->sc_module         ->s_out (this->dbg_B[0]->s_in); this->dbg_B[0]->s_out (this->duplicator[0]                    ->s_in );
		this->duplicator   [0]                    ->s_out1                                              (this->monitor   [0]->sc_module         ->s_in1);
		this->duplicator   [0]                    ->s_out2                                              (this->crc       [0]->sc_module_build   ->s_in );
		this->crc          [0]->sc_module_build   ->s_out (this->dbg_B[1]->s_in); this->dbg_B[1]->s_out (this->encoder   [0]->sc_module         ->s_in );
		this->encoder      [0]->sc_module         ->s_out (this->dbg_B[2]->s_in); this->dbg_B[2]->s_out (this->puncturer [0]->sc_module_punct   ->s_in );
		this->puncturer    [0]->sc_module_punct   ->s_out (this->dbg_B[3]->s_in); this->dbg_B[3]->s_out (this->modem     [0]->sc_module_mod     ->s_in );
		if (this->params.channel.type.find("RAYLEIGH") != std::string::npos) { // Rayleigh channel
			this->modem    [0]->sc_module_mod     ->s_out (this->dbg_R[0]->s_in); this->dbg_R[0]->s_out (this->channel   [0]->sc_module_wg      ->s_in );
			this->channel  [0]->sc_module_wg      ->s_out1(this->dbg_R[4]->s_in); this->dbg_R[4]->s_out (this->modem     [0]->sc_module_demod_wg->s_in1);
			this->channel  [0]->sc_module_wg      ->s_out2(this->dbg_R[1]->s_in); this->dbg_R[1]->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->dbg_R[2]->s_in); this->dbg_R[2]->s_out (this->modem     [0]->sc_module_demod_wg->s_in2);
			this->modem    [0]->sc_module_demod_wg->s_out (this->dbg_R[3]->s_in); this->dbg_R[3]->s_out (this->quantizer [0]->sc_module         ->s_in );
		} else { // additive channel (AWGN, USER, NO)
			this->modem    [0]->sc_module_mod     ->s_out (this->dbg_R[0]->s_in); this->dbg_R[0]->s_out (this->channel   [0]->sc_module         ->s_in );
			this->channel  [0]->sc_module         ->s_out (this->dbg_R[1]->s_in); this->dbg_R[1]->s_out (this->modem     [0]->sc_module_filt    ->s_in );
			this->modem    [0]->sc_module_filt    ->s_out (this->dbg_R[2]->s_in); this->dbg_R[2]->s_out (this->modem     [0]->sc_module_demod   ->s_in );
			this->modem    [0]->sc_module_demod   ->s_out (this->dbg_R[3]->s_in); this->dbg_R[3]->s_out (this->quantizer [0]->sc_module         ->s_in );
		}
		this->quantizer    [0]->sc_module         ->s_out (this->dbg_Q[0]->s_in); this->dbg_Q[0]->s_out (this->puncturer [0]->sc_module_depunct ->s_in );
		this->puncturer    [0]->sc_module_depunct ->s_out (this->dbg_Q[1]->s_in); this->dbg_Q[1]->s_out (this->decoder   [0]->sc_module         ->s_in );
		this->decoder      [0]->sc_module         ->s_out (this->dbg_B[4]->s_in); this->dbg_B[4]->s_out (this->crc       [0]->sc_module_extract ->s_in );
		this->crc          [0]->sc_module_extract ->s_out (this->dbg_B[6]->s_in); this->dbg_B[6]->s_out (this->monitor   [0]->sc_module         ->s_in2);
	}
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::simulation::SC_Simulation_BFER_std<B_8,R_8,Q_8>;
template class aff3ct::simulation::SC_Simulation_BFER_std<B_16,R_16,Q_16>;
template class aff3ct::simulation::SC_Simulation_BFER_std<B_32,R_32,Q_32>;
template class aff3ct::simulation::SC_Simulation_BFER_std<B_64,R_64,Q_64>;
#else
template class aff3ct::simulation::SC_Simulation_BFER_std<B,R,Q>;
#endif
// ==================================================================================== explicit template instantiation

#endif /* SYSTEMC*/
