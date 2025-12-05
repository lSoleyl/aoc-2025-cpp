
#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <algorithm>

using Id = int64_t;

struct Range {
  Id begin, end;

  bool contains(Id id) const {
    return id >= begin && id < end;
  }

  bool operator<(const Range& other) const {
    return begin < other.begin;
  }

  void merge(const Range& other) {
    begin = std::min(begin, other.begin);
    end = std::max(end, other.end);
  }

  int64_t size() const {
    return end - begin;
  }
};


struct Ingredients {
  Ingredients(std::istream&& input) {
    std::string line;
    while(!(line = stream::line(input)).empty()) {
      auto [start, end] = common::split2(line, '-');
      fresh.push_back({ string_view::into<Id>(start), string_view::into<Id>(end)+1 });
    }

    while (!(line = stream::line(input)).empty()) {
      available.push_back(string_view::into<Id>(line));
    }

    reorganize();
  }

  void reorganize() {
    // Sort ranges by begin position
    std::sort(fresh.begin(), fresh.end());

    // Now merge consecutive overlapping ranges
    auto writePos = fresh.begin();
    for (auto it = fresh.begin()+1, end = fresh.end(); it != end; ++it) {
      if (writePos->contains(it->begin)) {
        // We have overlap
        writePos->merge(*it);
      } else {
        // No overlap, move write position by one and copy it there
        *(++writePos) = *it;
      }
    }

    // Now we can remove all elements between writePos and end as they have been removed by merging them
    fresh.erase(writePos + 1, fresh.end());
  }


  bool isFresh(Id id) {
    // Use binary search to faster find the matching range
    // we search for the first end ordered AFTER the id to find the range which contains the id
    auto pos = std::upper_bound(fresh.begin(), fresh.end(), id, [](Id id, const Range& range) { return id < range.end; });
    return pos != fresh.end() && pos->contains(id);
  }

  std::vector<Range> fresh;
  std::vector<Id> available;
};




int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  Ingredients ingredients(task::input());

  part1 = std::ranges::count_if(ingredients.available, [&](Id id) { return ingredients.isFresh(id); });
  
  for (auto& range : ingredients.fresh) {
    part2 += range.size();
  }


  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}