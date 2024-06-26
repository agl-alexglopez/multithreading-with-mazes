module;
#include <cstdint>
#include <iterator>
#include <numeric>
#include <vector>
module labyrinth:disjoint_set;

class Disjoint_set {

  public:
    explicit Disjoint_set(const std::vector<uint64_t> &maze_square_ids)
        : parent_set_(maze_square_ids.size()),
          set_rank_(maze_square_ids.size(), 0)
    {
        for (uint64_t elem = 0; elem < maze_square_ids.size(); elem++)
        {
            parent_set_[elem] = elem;
        }
    }

    explicit Disjoint_set(uint64_t num_sets)
        : parent_set_(num_sets), set_rank_(num_sets, 0)
    {
        std::iota(std::begin(parent_set_), std::end(parent_set_), 0);
    }

    uint64_t
    find(uint64_t p)
    {
        std::vector<uint64_t> compress_path;
        while (parent_set_[p] != p)
        {
            compress_path.push_back(p);
            p = parent_set_[p];
        }
        while (!compress_path.empty())
        {
            parent_set_[compress_path.back()] = p;
            compress_path.pop_back();
        }
        return p;
    }

    bool
    made_union(uint64_t a, uint64_t b)
    {

        const uint64_t x = find(a);
        const uint64_t y = find(b);
        if (x == y)
        {
            return false;
        }
        if (set_rank_[x] > set_rank_[y])
        {
            parent_set_[y] = x;
        }
        else if (set_rank_[x] < set_rank_[y])
        {
            parent_set_[x] = y;
        }
        else
        {
            parent_set_[x] = y;
            set_rank_[y]++;
        }
        return true;
    }

    bool
    is_union_no_merge(uint64_t a, uint64_t b)
    {
        return find(a) == find(b);
    }

  private:
    std::vector<uint64_t> parent_set_;
    std::vector<uint64_t> set_rank_;
};
