
#include <common/time.hpp>
#include <common/task.hpp>

std::vector<int> parseInstructions(std::istream&& input) {
  std::vector<int> instructions;

  while (input) {
    char direction;
    int distance;
    input >> direction >> distance;

    if (input) { // read successful
      instructions.push_back(direction == 'R' ? distance : -distance);
    }
  }

  return instructions;
}

const int DIAL_SIZE = 100;

int main() {
  common::Time t;

  auto instructions = parseInstructions(task::input());

  int part1 = 0;
  int part2 = 0;

  int dial = 50;
  for (auto delta : instructions) {
    int distToNextZero = delta > 0 ? DIAL_SIZE - dial :
                          dial > 0 ? -dial : -DIAL_SIZE;

    // Part 2
    if (std::abs(delta) >= std::abs(distToNextZero)) {
      // We will pass 0 at least once during this rotation
      delta -= distToNextZero;
      ++part2;

      // Now we might wrap around more than once (count each time)
      part2 += std::abs(delta) / DIAL_SIZE;
      delta %= DIAL_SIZE;

      dial = (delta + DIAL_SIZE) % DIAL_SIZE;
    } else {
      // We still need to apply modulo, because at 0 - 18 we don't "pass" 0, but we still need to wrap around
      dial = (dial + delta + DIAL_SIZE) % DIAL_SIZE;
    }

    // Part 1
    if (dial == 0) {
      ++part1;
    }
  }
  

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}

