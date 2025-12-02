
#include <common/time.hpp>
#include <common/task.hpp>
#include <common/regex.hpp>

/** Caluclate power of 10 for huge numbers (without risk of double conversion errors)
 */
int64_t power10(int exponent) {
  int64_t result = 1;
  for (; exponent > 0; --exponent) {
    result *= 10;
  }
  return result;
}

/** Divide by the same number multiple times
 */
int64_t divPower(int64_t number, int64_t divisor, int divisorExponent) {
  for (; divisorExponent > 0; --divisorExponent) {
    number /= divisor;
  }
  return number;
}

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

  /** Calculates the invalid id sum for this range (Part 1)
   *  The main idea is to determine the a prefix, which we will increment until we are out of range.
   *  Then we can check the numbers directly by simply duplicating the prefix multiple times.
   * 
   *  This is a much more efficient an direct solution than searching for repetitions in all numbers within the
   *  range, because we only check the possible candiates and don't check every number in the range.
   */
  int64_t invalidIdSum() const {
    int64_t sum = 0;

    auto beginDigits = static_cast<int>(std::log10(begin)) + 1;
    auto lastDigits = static_cast<int>(std::log10(end-1)) + 1;
    for (int digits = beginDigits; digits <= lastDigits; ++digits) {
      // Ignore all odd digit numbers when checking for numbers with 2 parts
      //for (int parts = digits; parts > 1; --parts) {
      int parts = 2;
        if (digits % parts == 0) {
          // The divisor is used to extract the prefix from the number and continue incrementing it
          auto divisor = static_cast<int>(power10(digits/parts));
          // We can increment the prefix up to the divisor before we would increase the number of digits
          auto prefixEnd = divisor;


          auto beginNumber = std::max(begin, power10(digits-1));
          for (auto prefix = divPower(beginNumber, divisor, parts-1); repeatNumber(prefix, divisor, parts) < end && prefix < prefixEnd; ++prefix) {
            auto number = repeatNumber(prefix, divisor, parts);
            // We the range may not contain the number if we have 288352 as begin, then we start checking for 288288, which is not in the range
            if (contains(number)) {
              sum += number;
            }
          }
        }
      //}
    }

    return sum;

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
    part1 += range.invalidIdSum();
  }

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}

