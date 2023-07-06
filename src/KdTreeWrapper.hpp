#ifndef KDTREE_WRAPPER_HPP
#define KDTREE_WRAPPER_HPP

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/vector.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>

#include <chrono>

#include "kdtree.hpp"

struct KDTree : public Resource
{
  GDCLASS(KDTree,Resource)

  Kdtree::KdTree * kd = nullptr;
  KDTree() : kd{nullptr}
  {

  }
  ~KDTree()
  {
  }

  void set_dimension(int dim){
    if(dim > 0)
      kd->dimension = (size_t)dim;
  }

  void bake_nodes(const godot::PackedFloat32Array& points, size_t dimensions)
  {
    using namespace Kdtree;
    using namespace godot;
    using u = godot::UtilityFunctions;

    UtilityFunctions::prints("KDTree baking data",dimensions,points.size());

    if ((points.size()% dimensions) != 0)
    {
      UtilityFunctions::print("points size not divisible by dimensions");
      return;
    }

    KdNodeVector nodes{};
    for(size_t i = 0; i < points.size()/dimensions; ++i)
    {
        auto begin = points.ptr(), end = points.ptr(); // We use the ptr as iterator.
        begin = std::next(begin,dimensions * i);
        end = std::next(begin,dimensions);
        std::vector<float> point(begin,end);
        nodes.push_back(Kdtree::KdNode(point,nullptr,i));
        // u::prints(std::distance(points.ptrw(),begin),point.size());
    }
    u::prints(points);


    // if (kd != nullptr)
    //   delete kd;
    u::prints("Nb poses", nodes.size());
    // auto clock_start = std::chrono::system_clock::now();
    // try{
      // throw new std::exception("Hello");
        kd = new Kdtree::KdTree(&nodes,2);
        u::prints("Searching");
        KdNodeVector re{};
        kd->k_nearest_neighbors(std::vector<float>(points.ptr(),std::next(points.ptr(),dimensions)),1,&re);
        u::prints("Found result ",re.size());
    // }catch(std::exception e)
    // {
      // u::prints(e.what());
    // }
    // auto clock_end = std::chrono::system_clock::now();
    // float duration = float(std::chrono::duration_cast <std::chrono::microseconds> (clock_end - clock_start).count());
    // u::prints("Baking done in ", duration);
  }

  void set_weight(int difference_type , godot::PackedFloat32Array weight)
  {
    std::vector<float> w (&weight[0],&weight[weight.size()]);

    kd->set_distance(difference_type,&w);
  }

  godot::PackedInt32Array k_nearest_neighbors(godot::PackedFloat32Array point, size_t k)
  {
    if (kd == nullptr)
    {
        godot::UtilityFunctions::print("tree is null");
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

    godot::UtilityFunctions::print("Searching Data tooks ", currentTime, " ms");

    godot::PackedInt32Array indexes{};
    for (auto node : result)
    {
        indexes.append(node.index);
    }
    return indexes;
  }

  godot::PackedInt32Array range_nearest_neighbors(godot::PackedFloat32Array point, size_t k)
  {
    using namespace Kdtree;
      auto begin = point.ptrw(), end = point.ptrw(); // We use the ptr as iterator.
    end = std::next(end,point.size());
    std::vector<float> p (begin,end);
    KdNodeVector result{};
    kd->range_nearest_neighbors(p,k,&result);
    godot::PackedInt32Array indexes{};
    for(auto node : result){
      indexes.append(node.index);
    }
    return indexes;
  }

  protected:
  static void _bind_methods(){
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_dimension","dimensionality"), &KDTree::set_dimension);

    ClassDB::bind_method(D_METHOD("bake_nodes","points"), &KDTree::bake_nodes);

    ClassDB::bind_method(D_METHOD("set_weight","weights"), &KDTree::set_weight);

    ClassDB::bind_method(D_METHOD("k_nearest_neighbors","points","k"), &KDTree::k_nearest_neighbors);
    ClassDB::bind_method(D_METHOD("range_nearest_neighbors","points","k"), &KDTree::range_nearest_neighbors);

  }
};

#endif KDTREE_WRAPPER_HPP