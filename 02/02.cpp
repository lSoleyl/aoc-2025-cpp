
#include <common/time.hpp>
#include <common/task.hpp>
#include <common/regex.hpp>

struct Range {
  int64_t begin;
  int64_t end;

  /** Counts the total number of numbers in this range
   */
  int64_t size() const {
    return end - begin;
  }

  static std::vector<Range> parseRanges(std::string input) {
    std::vector<Range> ranges;
    std::regex rangeRegex("([0-9]+)-([0-9]+)");
    for (auto match : regex::iter(input, rangeRegex)) {
      ranges.push_back(Range { .begin = std::stoll(match[1].str()), .end = std::stoll(match[2].str()) + 1 });
    }
    return ranges;
  }
};


int main() {
  common::Time t;

  auto ranges = Range::parseRanges(task::inputString());


  int64_t part1 = 0;
  int part2 = 0;

  for (auto& range : ranges) {
    for (auto number = range.begin; number != range.end; ++number) {
      auto power = static_cast<int>(std::log10(number));
      if (power % 2 == 1) {
        power = ((power + 1) / 2);
        // Odd powers of 10 correspond to even number of digits
        auto divisor = static_cast<int64_t>(std::pow(10, power));
        auto div = std::div(number, divisor);
        if (div.quot == div.rem) {
          part1 += number;
        }
      }
    }
  }

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}

