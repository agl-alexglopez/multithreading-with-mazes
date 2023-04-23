#ifndef DISJOINT_SET_HH
#define DISJOINT_SET_HH

#include <cstdint>
#include <numeric>
#include <vector>

class Disjoint_set
{
public:
  explicit Disjoint_set( const std::vector<uint64_t>& maze_square_ids );
  explicit Disjoint_set( uint64_t num_sets );
  uint64_t find( uint64_t p );
  bool made_union( uint64_t a, uint64_t b );
  bool is_union_no_merge( uint64_t a, uint64_t b );

private:
  std::vector<uint64_t> parent_set_;
  std::vector<uint64_t> set_rank_;
};


#endif
