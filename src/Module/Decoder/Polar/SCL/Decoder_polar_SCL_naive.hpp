#ifndef DECODER_POLAR_SCL_NAIVE
#define DECODER_POLAR_SCL_NAIVE

#include <set>
#include <vector>
#include <mipp.h>

#include "Tools/Algo/Tree/Binary_tree_metric.hpp"
#include "Tools/Code/Polar/decoder_polar_functions.h"

#include "../../Decoder.hpp"

namespace aff3ct
{
namespace module
{
template <typename B = int, typename R = float>
class Contents_SCL
{
public:
	mipp::vector<R> lambda;
	mipp::vector<B> s;
	B               is_frozen_bit;

	Contents_SCL(int size) : lambda(size), s(size), is_frozen_bit(0) {}
	virtual ~Contents_SCL() {}
};

template <typename B, typename R, tools::proto_f<R> F = tools::f_LLR, tools::proto_g<B,R> G = tools::g_LLR>
class Decoder_polar_SCL_naive : public Decoder<B,R>
{
protected:
	const int m;           // graph depth
	const R   metric_init; // init value of the metrics in the trees

	const mipp::vector<B>& frozen_bits;

	const int     L; // maximum paths number
	std::set<int> active_paths;

	std::vector<tools::Binary_tree_metric<Contents_SCL<B,R>,R>*> polar_trees;
	std::vector<std::vector<tools::Binary_node<Contents_SCL<B,R>>*>> leaves_array;

public:
	Decoder_polar_SCL_naive(const int& K, const int& N, const int& L, const mipp::vector<B>& frozen_bits, 
	                        const int n_frames = 1, const std::string name = "Decoder_polar_SCL_naive");
	virtual ~Decoder_polar_SCL_naive();

protected:
	        void _load       (const R *Y_N                            );
	        void _hard_decode(const R *Y_N, B *V_K, const int frame_id);
	virtual void _store      (              B *V_K                    ) const;

private:
	void recursive_compute_llr        (      tools::Binary_node<Contents_SCL<B,R>>* node_cur, int depth);
	void propagate_sums               (const tools::Binary_node<Contents_SCL<B,R>>* node_cur           );
	void recursive_duplicate_tree_llr (      tools::Binary_node<Contents_SCL<B,R>>* node_a,
	                                         tools::Binary_node<Contents_SCL<B,R>>* node_b             );
	void recursive_duplicate_tree_sums(      tools::Binary_node<Contents_SCL<B,R>>* node_a,
	                                         tools::Binary_node<Contents_SCL<B,R>>* node_b,
	                                         tools::Binary_node<Contents_SCL<B,R>>* node_caller        );

	void duplicate_path(int path, int leaf_index, 
	                    std::vector<std::vector<tools::Binary_node<Contents_SCL<B,R>>*>> leaves_array);

protected:
	virtual void select_best_path();

	void recursive_allocate_nodes_contents  (      tools::Binary_node<Contents_SCL<B,R>>* node_curr, const int vector_size             );
	void recursive_initialize_frozen_bits   (const tools::Binary_node<Contents_SCL<B,R>>* node_curr, const mipp::vector<B>& frozen_bits);
	void recursive_store                    (const tools::Binary_node<Contents_SCL<B,R>>* node_curr, B *V_K, int &k                    ) const;
	void recursive_deallocate_nodes_contents(      tools::Binary_node<Contents_SCL<B,R>>* node_curr                                    );

	void apply_f     (const tools::Binary_node<Contents_SCL<B,R>>* node_curr);
	void apply_g     (const tools::Binary_node<Contents_SCL<B,R>>* node_curr);
	void compute_sums(const tools::Binary_node<Contents_SCL<B,R>>* node_curr);
};
}
}

#include "Decoder_polar_SCL_naive.hxx"

#endif /* DECODER_POLAR_SCL_NAIVE_ */
