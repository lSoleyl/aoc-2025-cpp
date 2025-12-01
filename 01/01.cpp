
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

  int dial = 50;
  for (auto delta : instructions) {
    dial = (dial + delta + DIAL_SIZE) % DIAL_SIZE;
    if (dial == 0) {
      ++part1;
    }
  }



  int part2 = 0;

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}

