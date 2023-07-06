#ifndef KDTREE_WRAPPER_HPP
#define KDTREE_WRAPPER_HPP

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/string/node_path.h"
#include "core/string/print_string.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

#include <chrono>

#include "../thirdparty/kdtree.hpp"
#include <sys/_types/_int64_t.h>

struct KDTree : public Resource {
	GDCLASS(KDTree, Resource)

	Kdtree::KdTree *kd = nullptr;
	KDTree() :
			kd{ nullptr } {
	}
	~KDTree() {
	}

	void set_dimension(int dim) {
		if (dim > 0) {
			kd->dimension = (int64_t)dim;
		}
	}

	void bake_nodes(const PackedFloat32Array &points, int64_t dimensions) {
		using namespace Kdtree;

		print_line(vformat("KDTree baking data %d", dimensions, points.size()));

		if ((points.size() % dimensions) != 0) {
			print_line("points size not divisible by dimensions");
			return;
		}

		KdNodeVector nodes{};
		for (int64_t i = 0; i < points.size() / dimensions; ++i) {
			auto begin = points.ptr(), end = points.ptr(); // We use the ptr as iterator.
			begin = std::next(begin, dimensions * i);
			end = std::next(begin, dimensions);
			std::vector<float> point(begin, end);
			nodes.push_back(Kdtree::KdNode(point, nullptr, i));
			// u::prints(std::distance(points.ptrw(),begin),point.size());
		}
		print_line(points);

		// if (kd != nullptr)
		//   delete kd;
		print_line("Nb poses", (int64_t) nodes.size());
		// auto clock_start = std::chrono::system_clock::now();
		// try{
		// throw new std::exception("Hello");
		kd = new Kdtree::KdTree(&nodes, 2);
		print_line("Searching");
		KdNodeVector re{};
		kd->k_nearest_neighbors(std::vector<float>(points.ptr(), std::next(points.ptr(), dimensions)), 1, &re);
		print_line(vformat("Found result %d", (int64_t)re.size()));
		// }catch(std::exception e)
		// {
		// u::prints(e.what());
		// }
		// auto clock_end = std::chrono::system_clock::now();
		// float duration = float(std::chrono::duration_cast <std::chrono::microseconds> (clock_end - clock_start).count());
		// u::prints("Baking done in ", duration);
	}

	void set_weight(int difference_type, PackedFloat32Array weight) {
		std::vector<float> w(&weight[0], &weight[weight.size()]);

		kd->set_distance(difference_type, &w);
	}

	PackedInt32Array k_nearest_neighbors(PackedFloat32Array point, int64_t k) {
		if (kd == nullptr) {
			print_line("tree is null");
			return {};
		}
		using namespace Kdtree;
		auto begin = point.ptrw(), end = point.ptrw(); // We use the ptr as iterator.
		end = std::next(end, point.size());
		std::vector<float> p(begin, end);
		KdNodeVector result{};
		auto clock_start = std::chrono::system_clock::now();
		kd->k_nearest_neighbors(p, k, &result);
		auto clock_now = std::chrono::system_clock::now();
		float currentTime = float(std::chrono::duration_cast<std::chrono::microseconds>(clock_now - clock_start).count());

		print_line("Searching Data tooks ", currentTime, " ms");

		PackedInt32Array indexes{};
		for (auto node : result) {
			indexes.append(node.index);
		}
		return indexes;
	}

	PackedInt32Array range_nearest_neighbors(PackedFloat32Array point, int64_t k) {
		using namespace Kdtree;
		auto begin = point.ptrw(), end = point.ptrw(); // We use the ptr as iterator.
		end = std::next(end, point.size());
		std::vector<float> p(begin, end);
		KdNodeVector result{};
		kd->range_nearest_neighbors(p, k, &result);
		PackedInt32Array indexes{};
		for (auto node : result) {
			indexes.append(node.index);
		}
		return indexes;
	}

protected:
	static void _bind_methods() {
		using namespace godot;
		ClassDB::bind_method(D_METHOD("set_dimension", "dimensionality"), &KDTree::set_dimension);

		ClassDB::bind_method(D_METHOD("bake_nodes", "points"), &KDTree::bake_nodes);

		ClassDB::bind_method(D_METHOD("set_weight", "weights"), &KDTree::set_weight);

		ClassDB::bind_method(D_METHOD("k_nearest_neighbors", "points", "k"), &KDTree::k_nearest_neighbors);
		ClassDB::bind_method(D_METHOD("range_nearest_neighbors", "points", "k"), &KDTree::range_nearest_neighbors);
	}
};

#endif KDTREE_WRAPPER_HPP