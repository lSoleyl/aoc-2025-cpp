
#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <algorithm>

using Id = int64_t;

struct IdRange {
  Id begin, end;

  bool contains(Id id) const {
    return id >= begin && id < end;
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
  }

  std::vector<IdRange> fresh;
  std::vector<Id> available;
};




int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  Ingredients ingredients(task::input());

  for (auto id : ingredients.available) {
    if (std::ranges::any_of(ingredients.fresh, [=](const IdRange& range) { return range.contains(id); })) {
      ++part1;
    }
  }




  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}