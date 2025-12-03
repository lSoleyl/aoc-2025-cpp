
#include <common/time.hpp>
#include <common/task.hpp>
#include <common/regex.hpp>
#include <common/math.hpp>

#include <set>

/** Repeat a prefix multiple times be repeated multiplication and addition 12 -> 12*10+12 = 1212 -> ...
 */
int64_t repeatNumber(int64_t prefix, int64_t factor, int times) {
  int64_t result = 0;
  for (; times > 0; --times) {
    result *= factor;
    result += prefix;
  }
  return result;
}


struct Range {
  int64_t begin;
  int64_t end;

  /** Counts the total number of numbers in this range
   */
  int64_t size() const {
    return end - begin;
  }

  int64_t contains(int64_t number) const {
    return number >= begin && number < end;
  }

  /** Calculates the invalid id sum for this range (Part 1 & Part 2)
   *  The main idea is to determine the a prefix, which we will increment until we are out of range.
   *  Then we can check the numbers directly by simply duplicating the prefix multiple times.
   * 
   *  This is a much more efficient an direct solution than searching for repetitions in all numbers within the
   *  range, because we only check the possible candiates and don't check every number in the range.
   */
  void invalidIdSums(int64_t& part1, int64_t& part2) const {
    auto beginDigits = math::digits(begin);
    auto lastDigits = math::digits(end - 1);

    // We must collect the part 2 numbers in a set, before adding them up, because we may find the same
    // number multiple times eg 222222 -> 222 222 & 22 22 22 & 2 2 2 2 2 2
    std::set<int64_t> part2Numbers;
    
    for (int digits = beginDigits; digits <= lastDigits; ++digits) {
      // Check all possible splits (of the digits)
      for (int parts = digits; parts > 1; --parts) {  // Try out all part sizes (for Part2)
        if (digits % parts == 0) {
          // The divisor is used to extract the prefix from the number and continue incrementing it
          auto divisor = static_cast<int>(math::power10(digits/parts));
          // We can increment the prefix up to the divisor before we would increase the number of digits
          auto prefixEnd = divisor;


          auto beginNumber = std::max(begin, math::power10(digits-1));
          for (auto prefix = math::divPower(beginNumber, divisor, parts-1); repeatNumber(prefix, divisor, parts) < end && prefix < prefixEnd; ++prefix) {
            auto number = repeatNumber(prefix, divisor, parts);
            // We the range may not contain the number if we have 288352 as begin, then we start checking for 288288, which is not in the range
            if (contains(number)) {
              if (part2Numbers.insert(number).second) {
                // Only add the first occurrence of the same number (see above)
                part2 += number;
              }
              if (parts == 2) {
                // Part 1 only sums up 2 part numbers
                part1 += number;
              }
            }
          }
        }
      }
    }
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
  int64_t part2 = 0;

  for (auto& range : ranges) {
    range.invalidIdSums(part1, part2);
  }

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}

