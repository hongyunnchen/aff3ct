#include <thread>
#include <string>
#include <iostream>

#include "Launcher_BFERI.hpp"

using namespace aff3ct::tools;
using namespace aff3ct::launcher;

template <typename B, typename R, typename Q>
Launcher_BFERI<B,R,Q>
::Launcher_BFERI(const int argc, const char **argv, std::ostream &stream)
: Launcher<B,R,Q>(argc, argv, stream), codec(nullptr)
{
	this->params.simulation .type             = "BFERI";
	this->params.simulation .benchs           = 0;
	this->params.simulation .debug            = false;
	this->params.simulation .debug_limit      = 0;
	this->params.simulation .debug_precision  = 5;
	this->params.simulation .time_report      = false;
	this->params.simulation .n_ite            = 15;
#if !defined(STARPU) && !defined(SYSTEMC)
	this->params.simulation .n_threads        = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
#endif
	this->params.code       .coset            = false;
	this->params.encoder    .type             = "";
	this->params.encoder    .path             = "";
	this->params.encoder    .systematic       = true;
	this->params.interleaver.type             = "RANDOM";
	this->params.interleaver.path             = "";
	this->params.interleaver.n_cols           = 4;
	this->params.interleaver.uniform          = false;
	this->params.demodulator.max              = "MAX";
	this->params.monitor    .n_frame_errors   = 100;
	this->params.monitor    .err_track_enable = false;
	this->params.monitor    .err_track_revert = false;
	this->params.monitor    .err_track_path   = "error_tracker";
	this->params.terminal   .type             = "STD";
}

template <typename B, typename R, typename Q>
void Launcher_BFERI<B,R,Q>
::build_args()
{
	Launcher<B,R,Q>::build_args();

	// ---------------------------------------------------------------------------------------------------- simulation
	this->opt_args[{"sim-benchs", "b"}] =
		{"positive_int",
		 "enable special benchmark mode with a loop around the decoder."};
	this->opt_args[{"sim-debug", "d"}] =
		{"",
		 "enable debug mode: print array values after each step."};
	this->opt_args[{"sim-debug-prec"}] =
		{"positive_int",
		 "set the precision of real elements when displayed in debug mode."};
	this->opt_args[{"sim-debug-limit"}] =
		{"positive_int",
		 "set the max number of elements to display in the debug mode."};
	this->opt_args[{"sim-time-report"}] =
		{"",
		 "display time information about the simulation chain."};
	this->opt_args[{"sim-snr-type", "E"}] =
		{"string",
		 "select the type of SNR: symbol energy or information bit energy.",
		 "ES, EB"};
	this->opt_args[{"sim-ite", "I"}] =
		{"positive_int",
		 "number of global turbo iterations between the demodulator and the decoder."};

	// ---------------------------------------------------------------------------------------------------------- code
	this->opt_args[{"cde-coset", "c"}] =
		{"",
		 "enable the coset approach."};

	// ------------------------------------------------------------------------------------------------------- encoder
	this->opt_args[{"enc-type"}] =
		{"string",
		 "select the type of encoder you want to use.",
		 "AZCW, COSET, USER" };
	this->opt_args[{"enc-path"}] =
		{"string",
		 "path to a file containing one or a set of pre-computed codewords, to use with \"--enc-type USER\"."};

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

	// ------------------------------------------------------------------------------------------------------- monitor
	this->opt_args[{"mnt-max-fe", "e"}] =
		{"positive_int",
		 "max number of frame errors for each SNR simulation."};
	this->opt_args[{"mnt-err-trk"}] =
		{"",
		 "enable the tracking of the bad frames (by default the frames are stored in the current folder)."};
	this->opt_args[{"mnt-err-trk-rev"}] =
		{"",
		 "automatically replay the saved frames."};
	this->opt_args[{"mnt-err-trk-path"}] =
		{"string",
		 "base path for the files where the bad frames will be stored or read."};

	// ------------------------------------------------------------------------------------------------------ terminal
	this->opt_args[{"term-type"}] =
		{"string",
		 "select the terminal you want.",
		 "STD, LEGACY"};
}

template <typename B, typename R, typename Q>
void Launcher_BFERI<B,R,Q>
::store_args()
{
	Launcher<B,R,Q>::store_args();

	// ---------------------------------------------------------------------------------------------------- simulation
	if(this->ar.exist_arg({"sim-benchs",    "b"})) this->params.simulation.benchs      = this->ar.get_arg_int({"sim-benchs",   "b"});
	if(this->ar.exist_arg({"sim-snr-type",  "E"})) this->params.simulation.snr_type    = this->ar.get_arg    ({"sim-snr-type", "E"});
	if(this->ar.exist_arg({"sim-ite",       "I"})) this->params.simulation.n_ite       = this->ar.get_arg_int({"sim-ite",      "I"});
	if(this->ar.exist_arg({"sim-time-report"   })) this->params.simulation.time_report = true;
	if(this->ar.exist_arg({"sim-debug",     "d"})) this->params.simulation.debug       = true;
	if(this->ar.exist_arg({"sim-debug-limit"   }))
	{
		this->params.simulation.debug = true;
		this->params.simulation.debug_limit = this->ar.get_arg_int({"sim-debug-limit"});
	}
	if(this->ar.exist_arg({"sim-debug-prec"}))
	{
		this->params.simulation.debug = true;
		this->params.simulation.debug_precision = this->ar.get_arg_int({"sim-debug-prec"});
	}

	if (this->params.simulation.debug &&
	    !(this->ar.exist_arg({"sim-threads", "t"}) && this->ar.get_arg_int({"sim-threads", "t"}) > 0))
		// check if debug is asked and if n_thread kept its default value
		this->params.simulation.n_threads = 1;

	// ---------------------------------------------------------------------------------------------------------- code
	if(this->ar.exist_arg({"cde-coset", "c"})) this->params.code.coset = true;
	if (this->params.code.coset)
		this->params.encoder.type = "COSET";

	// ------------------------------------------------------------------------------------------------------- encoder
	if(this->ar.exist_arg({"enc-type"})) this->params.encoder.type = this->ar.get_arg({"enc-type"});
	if (this->params.encoder.type == "COSET")
		this->params.code.coset = true;
	if (this->params.encoder.type == "AZCW")
		this->params.source.type = "AZCW";
	if (this->params.encoder.type == "USER")
		this->params.source.type = "USER";
	if(this->ar.exist_arg({"enc-path"})) this->params.encoder.path = this->ar.get_arg({"enc-path"});

	// --------------------------------------------------------------------------------------------------- interleaver
	if(this->ar.exist_arg({"itl-type"})) this->params.interleaver.type    = this->ar.get_arg    ({"itl-type"});
	if(this->ar.exist_arg({"itl-path"})) this->params.interleaver.path    = this->ar.get_arg    ({"itl-path"});
	if(this->ar.exist_arg({"itl-cols"})) this->params.interleaver.n_cols  = this->ar.get_arg_int({"itl-cols"});
	if(this->ar.exist_arg({"itl-uni" })) this->params.interleaver.uniform = true;

	// ------------------------------------------------------------------------------------------------------- monitor
	if(this->ar.exist_arg({"mnt-max-fe",  "e"})) this->params.monitor.n_frame_errors   = this->ar.get_arg_int({"mnt-max-fe", "e"});
	if(this->ar.exist_arg({"mnt-err-trk-rev" })) this->params.monitor.err_track_revert = true;
	if(this->ar.exist_arg({"mnt-err-trk"     })) this->params.monitor.err_track_enable = true;
	if(this->ar.exist_arg({"mnt-err-trk-path"})) this->params.monitor.err_track_path   = this->ar.get_arg({"mnt-err-trk-path"});

	if(this->params.monitor.err_track_revert)
	{
		this->params.monitor.err_track_enable = false;
		this->params.source     .type = "USER";
		this->params.encoder    .type = "USER";
		this->params.channel    .type = "USER";
		if (this->params.interleaver.uniform)
			this->params.interleaver.type = "USER";
		this->params.source     .path = this->params.monitor.err_track_path + std::string("_$snr.src");
		this->params.encoder    .path = this->params.monitor.err_track_path + std::string("_$snr.enc");
		this->params.channel    .path = this->params.monitor.err_track_path + std::string("_$snr.chn");
		if (this->params.interleaver.uniform)
			this->params.interleaver.path = this->params.monitor.err_track_path + std::string("_$snr.itl");
		// the paths are set in the Simulation class
	}

	if (this->params.monitor.err_track_revert &&
	    !(this->ar.exist_arg({"sim-threads", "t"}) && this->ar.get_arg_int({"sim-threads", "t"}) > 0))
		this->params.simulation.n_threads = 1;

	// ------------------------------------------------------------------------------------------------------ terminal
	if(this->ar.exist_arg({"term-type"})) this->params.terminal.type = this->ar.get_arg({"term-type"});
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_simulation()
{
	std::string threads = "unused";
	if (this->params.simulation.n_threads)
		threads = std::to_string(this->params.simulation.n_threads) + " thread(s)";

	auto p = Launcher<B,R,Q>::header_simulation();

	p.push_back(std::make_pair("Multi-threading (t)", threads));
	p.push_back(std::make_pair("Global iterations (I)", std::to_string(this->params.simulation.n_ite)));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_code()
{
	std::string coset = this->params.code.coset ? "on" : "off";

	auto p = Launcher<B,R,Q>::header_code();

	p.push_back(std::make_pair("Coset approach (c)", coset));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_encoder()
{
	std::string syst_enc = ((this->params.encoder.systematic) ? "on" : "off");

	auto p = Launcher<B,R,Q>::header_encoder();

	p.push_back(std::make_pair("Type", this->params.encoder.type));

	if (this->params.encoder.type == "USER")
		p.push_back(std::make_pair("Path", this->params.encoder.path));

	p.push_back(std::make_pair("Systematic", syst_enc));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_interleaver()
{
	auto p = Launcher<B,R,Q>::header_interleaver();

	p.push_back(std::make_pair("Type", this->params.interleaver.type));

	if (this->params.interleaver.type == "USER")
		p.push_back(std::make_pair("Path", this->params.interleaver.path));

	if (this->params.interleaver.type == "RAND_COL" || this->params.interleaver.type == "ROW_COL")
		p.push_back(std::make_pair("Number of columns", std::to_string(this->params.interleaver.n_cols)));

	p.push_back(std::make_pair("Uniform", (this->params.interleaver.uniform ? "on" : "off")));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_decoder()
{
	auto p = Launcher<B,R,Q>::header_decoder();

	p.push_back(std::make_pair("Type (D)",       this->params.decoder.type  ));
	p.push_back(std::make_pair("Implementation", this->params.decoder.implem));

	return p;
}

template <typename B, typename R, typename Q>
std::vector<std::pair<std::string,std::string>> Launcher_BFERI<B,R,Q>
::header_monitor()
{
	auto p = Launcher<B,R,Q>::header_monitor();

	p.push_back(std::make_pair("Frame error count (e)", std::to_string(this->params.monitor.n_frame_errors)));

	std::string enable_track = (this->params.monitor.err_track_enable) ? "on" : "off";
	p.push_back(std::make_pair("Bad frames tracking", enable_track));

	std::string enable_rev_track = (this->params.monitor.err_track_revert) ? "on" : "off";
	p.push_back(std::make_pair("Bad frames replay", enable_rev_track));

	if (this->params.monitor.err_track_enable || this->params.monitor.err_track_revert)
	{
		std::string path = this->params.monitor.err_track_path + std::string("_$snr.[src,enc,itl,chn]");
		p.push_back(std::make_pair("Bad frames base path", path));
	}

	return p;
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::launcher::Launcher_BFERI<B_8,R_8,Q_8>;
template class aff3ct::launcher::Launcher_BFERI<B_16,R_16,Q_16>;
template class aff3ct::launcher::Launcher_BFERI<B_32,R_32,Q_32>;
template class aff3ct::launcher::Launcher_BFERI<B_64,R_64,Q_64>;
#else
template class aff3ct::launcher::Launcher_BFERI<B,R,Q>;
#endif
// ==================================================================================== explicit template instantiation
