#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>
#include <common/vector.hpp>



struct MovieTheater {
  MovieTheater(std::istream&& input) {
    for (auto& line : stream::lines(input)) {
      auto [x, y] = common::split2(line, ',');
      redTiles.emplace_back(string_view::into<int>(x), string_view::into<int>(y));
    }
  }

  // Part 1
  int64_t largestRectangleArea() const {
    // For now lets try the naive approach
    int64_t maxArea = 0;
    for (auto aPos = redTiles.begin(), end = redTiles.end(); aPos != end; ++aPos) {
      for (auto bPos = aPos+1; bPos != end; ++bPos) {
        maxArea = std::max(maxArea, area(*aPos, *bPos));
      }
    }

    return maxArea;
  }


  static int64_t area(const Vector& a, const Vector& b) {
    auto delta = (a - b).apply([](int value) { return std::abs(value) + 1;});
    return static_cast<int64_t>(delta.x) * delta.y;
  }


  std::vector<Vector> redTiles;
};



int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  MovieTheater theater(task::input());
  part1 = theater.largestRectangleArea();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}