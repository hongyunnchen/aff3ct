#include <iostream>

#if defined(SYSTEMC)
#include "Simulation/BFER/Standard/SystemC/SC_Simulation_BFER_std.hpp"
#elif defined(STARPU)
#include "Simulation/BFER/Standard/StarPU/SPU_Simulation_BFER_std.hpp"
#else
#include "Simulation/BFER/Standard/Threads/Simulation_BFER_std_threads.hpp"
#endif
#include "Tools/Codec/RA/Codec_RA.hpp"

#include "Launcher_BFER_RA.hpp"

using namespace aff3ct::tools;
using namespace aff3ct::simulation;
using namespace aff3ct::launcher;

template <typename B, typename R, typename Q>
Launcher_BFER_RA<B,R,Q>
::Launcher_BFER_RA(const int argc, const char **argv, std::ostream &stream)
: Launcher_BFER<B,R,Q>(argc, argv, stream)
{
	this->params.code       .type       = "RA";
	this->params.encoder    .type       = "RA";
	this->params.interleaver.type       = "RANDOM";
	this->params.interleaver.path       = "";
	this->params.interleaver.n_cols     = 4;
	this->params.interleaver.uniform    = false;
	this->params.quantizer  .n_bits     = 7;
	this->params.quantizer  .n_decimals = 2;
	this->params.decoder    .type       = "RA";
	this->params.decoder    .implem     = "STD";
	this->params.decoder    .n_ite      = 10;
}

template <typename B, typename R, typename Q>
void Launcher_BFER_RA<B,R,Q>
::build_args()
{
	Launcher_BFER<B,R,Q>::build_args();

	// ------------------------------------------------------------------------------------------------------- encoder
	this->opt_args[{"enc-type"}][2] += ", RA";

	// --------------------------------------------------------------------------------------------------- interleaver
	this->opt_args[{"itl-type"}] =
		{"string",
		 "specify the type of the interleaver.",
		 "LTE, CCSDS, RANDOM, GOLDEN, USER, RAND_COL, ROW_COL, NO"};

	this->opt_args[{"itl-path"}] =
		{"string",
		 "specify the path to the interleaver file (to use with \"--itl-type USER\"."};

	this->opt_args[{"itl-cols"}] =
		{"positive_int",
		 "specify the number of columns used for the COLUMNS interleaver."};

	this->opt_args[{"itl-uni"}] =
		{"",
		 "enable the regeneration of the interleaver for each new frame."};

	// ------------------------------------------------------------------------------------------------------- decoder
	this->opt_args[{"dec-ite", "i"}] =
		{"positive_int",
		 "maximal number of iterations in the decoder."};
}

template <typename B, typename R, typename Q>
void Launcher_BFER_RA<B,R,Q>
::store_args()
{
	Launcher_BFER<B,R,Q>::store_args();

	// --------------------------------------------------------------------------------------------------- interleaver
	if(this->ar.exist_arg({"itl-type"})) this->params.interleaver.type    = this->ar.get_arg    ({"itl-type"});
	if(this->ar.exist_arg({"itl-path"})) this->params.interleaver.path    = this->ar.get_arg    ({"itl-path"});
	if(this->ar.exist_arg({"itl-cols"})) this->params.interleaver.n_cols  = this->ar.get_arg_int({"itl-cols"});
	if(this->ar.exist_arg({"itl-uni" })) this->params.interleaver.uniform = true;

	if(this->params.monitor.err_track_revert)
	{
		this->params.monitor.err_track_enable = false;
		if (this->params.interleaver.uniform)
		{
			this->params.interleaver.type = "USER";
			this->params.interleaver.path = this->params.monitor.err_track_path + std::string("_$snr.itl");
		}
	}

	// ------------------------------------------------------------------------------------------------------- decoder
	this->opt_args[{"dec-type", "D"}].push_back("RA" );
	this->opt_args[{"dec-implem"   }].push_back("STD");
	if(this->ar.exist_arg({"dec-ite", "i"})) this->params.decoder.n_ite = this->ar.get_arg_int({"dec-ite", "i"});
}

template <typename B, typename R, typename Q>
Simulation* Launcher_BFER_RA<B,R,Q>
::build_simu()
{
	this->codec = new Codec_RA<B,Q>(this->params);
#if defined(SYSTEMC)
	return new SC_Simulation_BFER_std     <B,R,Q>(this->params, *this->codec);
#elif defined(STARPU)
	return new SPU_Simulation_BFER_std    <B,R,Q>(this->params, *this->codec);
#else
	return new Simulation_BFER_std_threads<B,R,Q>(this->params, *this->codec);
#endif
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFER_RA<B,R,Q>
::header_interleaver()
{
	auto p = Launcher_BFER<B,R,Q>::header_interleaver();

	p.push_back(std::make_pair("Type", this->params.interleaver.type));

	if (this->params.interleaver.type == "USER")
		p.push_back(std::make_pair("Path", this->params.interleaver.path));

	if (this->params.interleaver.type == "RAND_COL" || this->params.interleaver.type == "ROW_COL")
		p.push_back(std::make_pair("Number of columns", std::to_string(this->params.interleaver.n_cols)));

	p.push_back(std::make_pair("Uniform", (this->params.interleaver.uniform ? "on" : "off")));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFER_RA<B,R,Q>
::header_decoder()
{
	auto p = Launcher_BFER<B,R,Q>::header_decoder();

	p.push_back(std::make_pair("Num. of iterations (i)", std::to_string(this->params.decoder.n_ite)));

	return p;
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::launcher::Launcher_BFER_RA<B_8,R_8,Q_8>;
template class aff3ct::launcher::Launcher_BFER_RA<B_16,R_16,Q_16>;
template class aff3ct::launcher::Launcher_BFER_RA<B_32,R_32,Q_32>;
template class aff3ct::launcher::Launcher_BFER_RA<B_64,R_64,Q_64>;
#else
template class aff3ct::launcher::Launcher_BFER_RA<B,R,Q>;
#endif
// ==================================================================================== explicit template instantiation
