#ifndef __kdtree_HPP
#define __kdtree_HPP


//
// Kd-Tree implementation.
//
// Copyright: Christoph Dalitz, 2018-2022
//            Jens Wilberg, 2018
// Version:   1.2
// License:   BSD style license
//            (see the file LICENSE for details)
//

// MODIFICATION
// -- Removed throw to make it pass fno-exceptions. The in case of throw nothing will
// be done to the current state of the object. This include the constructor
// -- Added logic to have a custom_weight for a single query. Good for paralellism
// and let user have custom query.

#include <cstdlib>
#include <queue>
#include <vector>

namespace Kdtree {

typedef std::vector<float> CoordPoint;
typedef std::vector<float> WeightVector;

// for passing points to the constructor of kdtree
struct KdNode {
  CoordPoint point;
  void* data;
  int index;
  KdNode(const CoordPoint& p, void* d = NULL, int i = -1) {
    point = p;
    data = d;
    index = i;
  }
  KdNode() { data = NULL; }
};
typedef std::vector<KdNode> KdNodeVector;

// base function object for search predicate in knn search
// returns true when the given KdNode is an admissible neighbor
// To define an own search predicate, derive from this class
// and overwrite the call operator operator()
struct KdNodePredicate {
  virtual ~KdNodePredicate() {}
  virtual bool operator()(const KdNode&) const { return true; }
};

//--------------------------------------------------------
// private helper classes used internally by KdTree
//
// the internal node structure used by kdtree
class kdtree_node;
// base class for different distance computations
class DistanceMeasure;
// helper class for priority queue in k nearest neighbor search
class nn4heap {
 public:
  size_t dataindex;  // index of actual kdnode in *allnodes*
  float distance;   // distance of this neighbor from *point*
  nn4heap(size_t i, float d) {
    dataindex = i;
    distance = d;
  }
};
class compare_nn4heap {
 public:
  bool operator()(const nn4heap& n, const nn4heap& m) {
    return (n.distance < m.distance);
  }
};
  typedef std::priority_queue<nn4heap, std::vector<nn4heap>, compare_nn4heap> SearchQueue;
//--------------------------------------------------------

// kdtree class
class KdTree {
 private:
  // recursive build of tree
  kdtree_node* build_tree(size_t depth, size_t a, size_t b);
  // helper variable for keeping track of subtree bounding box
  CoordPoint lobound, upbound;
  // helper variable to check the distance method
  int distance_type;
  bool neighbor_search(const CoordPoint& point, kdtree_node* node, size_t k, SearchQueue* neighborheap,DistanceMeasure * distance = nullptr);
  void range_search(const CoordPoint& point, kdtree_node* node, float r, std::vector<size_t>* range_result,DistanceMeasure* distance = nullptr);
  bool bounds_overlap_ball(const CoordPoint& point, float dist,
                           kdtree_node* node, DistanceMeasure * distance = nullptr);
  bool ball_within_bounds(const CoordPoint& point, float dist,
                          kdtree_node* node, DistanceMeasure * distance = nullptr);
  // class implementing the distance computation
  DistanceMeasure* default_distance;
  // search predicate in knn searches
  KdNodePredicate* searchpredicate;

 public:
  KdNodeVector allnodes;
  size_t dimension;
  kdtree_node* root;
  // distance_type can be 0 (max), 1 (city block), or 2 (euklid [squared])
  KdTree(const KdNodeVector* nodes, int distance_type = 2);
  ~KdTree();
  void set_distance(int distance_type, const WeightVector* weights = NULL);
  void k_nearest_neighbors(const CoordPoint& point, size_t k,
                           KdNodeVector* result, KdNodePredicate* pred = NULL,const WeightVector * custom_weight = nullptr);
  void range_nearest_neighbors(const CoordPoint& point, float r,
                               KdNodeVector* result);
};


}  // end namespace Kdtree

#endif