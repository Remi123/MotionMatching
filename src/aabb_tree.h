#ifndef AABB_TREE_HPP
#define AABB_TREE_HPP

#include "../thirdparty/AABB.h"

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/string/node_path.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"

struct AABBTree : public Resource {
	GDCLASS(AABBTree, Resource)
	aabb::Tree bvh{};

	void setup_tree(int dim, float fattening, int64_t nb_particles, bool touching_is_overlap) {
		bvh.removeAll();
		delete &bvh;
		bvh = aabb::Tree(dim, fattening, nb_particles, touching_is_overlap);
	}

	void insert_particle_at_position(int64_t index, PackedFloat32Array position, float radius) {
		auto begin = position.ptrw(), end = position.ptrw(); // We use the ptr as iterator.
		end = std::next(end, position.size());
		std::vector<float> pos(begin, end);
		bvh.insertParticle(index, pos, radius);
	}

	void insert_particle(int64_t index, PackedFloat32Array lowerbound, PackedFloat32Array upperbound) {
		auto begin = lowerbound.ptrw(), end = lowerbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, lowerbound.size());
		std::vector<float> lb(begin, end);

		begin = upperbound.ptrw();
		end = upperbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, upperbound.size());
		std::vector<float> ub(begin, end);

		bvh.insertParticle(index, lb, ub);
	}

	void remove_particle(int64_t index) {
		bvh.removeParticle(index);
	}
	void remove_all() {
		bvh.removeAll();
	}

	void update_particle_at_position(int64_t index, PackedFloat32Array position, float radius, bool always_reinsert) {
		auto begin = position.ptrw(), end = position.ptrw(); // We use the ptr as iterator.
		end = std::next(end, position.size());
		std::vector<float> pos(begin, end);

		bvh.updateParticle(index, pos, radius, always_reinsert);
	}
	void update_particle(int64_t index, PackedFloat32Array lowerbound, PackedFloat32Array upperbound, bool always_reinsert) {
		auto begin = lowerbound.ptrw(), end = lowerbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, lowerbound.size());
		std::vector<float> lb(begin, end);

		begin = upperbound.ptrw();
		end = upperbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, upperbound.size());
		std::vector<float> ub(begin, end);

		bvh.updateParticle(index, lb, ub, always_reinsert);
	}

	PackedInt64Array query_index(unsigned int i) {
		auto result = bvh.query(i);
		PackedInt64Array r;
		for (auto f : result)
			r.push_back(f);
		return r;
	}

	PackedInt64Array query_bounds(PackedFloat32Array lowerbound, PackedFloat32Array upperbound) {
		auto begin = lowerbound.ptrw(), end = lowerbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, lowerbound.size());
		std::vector<float> lb(begin, end);

		begin = upperbound.ptrw();
		end = upperbound.ptrw(); // We use the ptr as iterator.
		end = std::next(end, upperbound.size());
		std::vector<float> ub(begin, end);

		aabb::AABB bounds(lb, ub);
		auto result = bvh.query(bounds);
		PackedInt64Array r;
		for (auto f : result)
			r.push_back(f);
		return r;
	}

protected:
	static void _bind_methods() {
		using namespace godot;
		ClassDB::bind_method(D_METHOD("setup_tree", "dimensionality", "fattening", "nb_particles", "touching_is_overlap"), &AABBTree::setup_tree);
		ClassDB::bind_method(D_METHOD("insert_particle_at_position", "index", "position", "radius"), &AABBTree::insert_particle_at_position);
		ClassDB::bind_method(D_METHOD("insert_particle", "index", "lowerbound", "uppderbound"), &AABBTree::insert_particle);
		ClassDB::bind_method(D_METHOD("remove_particle", "index"), &AABBTree::remove_particle);
		ClassDB::bind_method(D_METHOD("remove_all"), &AABBTree::remove_all);

		ClassDB::bind_method(D_METHOD("update_particle_at_position", "index", "position", "radius", "always_reinsert"), &AABBTree::update_particle_at_position);
		ClassDB::bind_method(D_METHOD("update_particle", "index", "lowerbound", "upperbound", "always_reinsert"), &AABBTree::update_particle);

		ClassDB::bind_method(D_METHOD("query_index", "index"), &AABBTree::query_index);
		ClassDB::bind_method(D_METHOD("query_bounds", "lowerbound", "upperbound"), &AABBTree::query_bounds);
	}
};
#endif