
#include <common/task.hpp>
#include <common/time.hpp>
#include <common/stream.hpp>

#include <ranges>
#include <algorithm>

struct Battery {
  Battery(std::string ratings) : ratings(std::move(ratings)) {}

  int outputJoltage() {
    auto firstPos = std::max_element(ratings.begin(), ratings.end()-1); // Look for the largest number and ignore the last position 
    auto secondPos = std::max_element(firstPos + 1, ratings.end()); // Now find the highest number right of the first number
    return static_cast<int>(*firstPos-'0') * 10 + static_cast<int>(*secondPos-'0');
  }


  std::string ratings;
};


int main() {
  common::Time t;
  
  auto batteries = stream::lines(task::input()) | std::views::transform([](std::string line) { return Battery(line); }) | std::ranges::to<std::vector>();

  int part1 = 0;
  int64_t part2 = 0;

  for (auto& battery : batteries) {
    part1 += battery.outputJoltage();
  }

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
