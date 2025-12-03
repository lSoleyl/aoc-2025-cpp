
#include <common/task.hpp>
#include <common/time.hpp>
#include <common/stream.hpp>

#include <ranges>
#include <algorithm>

struct Battery {
  Battery(std::string ratings) : ratings(std::move(ratings)) {}

  int64_t outputJoltage(int nBatteries) {
    auto begin = ratings.begin();
    auto end = ratings.end() - nBatteries; // Ignore the last positions when searching for the first battery

    int64_t joltage = 0;
    for (int i = 0; i < nBatteries; ++i) {
      auto maxPos = std::max_element(begin, ++end); // increment end before the search to avoid incrementing past-end on last iteration (triggers debug assertion)
      joltage *= 10;
      joltage += digitValue(*maxPos);

      begin = maxPos + 1; // continue searching after this digit
    }

    return joltage;
  }


  static int digitValue(char digit) {
    return static_cast<int>(digit - '0');
  }



  std::string ratings;
};


int main() {
  common::Time t;
  
  auto batteries = stream::lines(task::input()) | std::views::transform([](std::string line) { return Battery(line); }) | std::ranges::to<std::vector>();

  int64_t part1 = 0;
  int64_t part2 = 0;

  for (auto& battery : batteries) {
    part1 += battery.outputJoltage(2);
    part2 += battery.outputJoltage(12);
  }

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
