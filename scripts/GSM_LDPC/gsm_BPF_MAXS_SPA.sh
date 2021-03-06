#!/bin/bash

./bin/aff3ct                                              \
	--sim-type     BFERI                                  \
	--sim-prec     32                                     \
	--sim-snr-min  1.0                                    \
	--sim-snr-max  3.0                                    \
	--sim-snr-step 0.1                                    \
	--mnt-max-fe   100                                    \
	--src-type     RAND_FAST                              \
	-K             2112                                   \
	--cde-type     LDPC                                   \
	--cde-coset                                           \
	-N             4224                                   \
	--itl-type     RANDOM                                 \
	--mod-type     CPM                                    \
	--dmod-max     MAXS                                   \
	--chn-type     AWGN_FAST                              \
	--dec-type     BP_FLOODING                            \
	--dec-implem   SPA                                    \
	--dec-no-synd                                         \
	--cde-alist-path ../conf/dec/LDPC/GSM_2112_4224.alist \
	--mod-cpm-std  GSM                                    \
	"$@"
