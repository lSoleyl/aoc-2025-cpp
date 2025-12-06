#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/regex.hpp>


int64_t add(int64_t a, int64_t b) {
  return a + b;
}

int64_t multiply(int64_t a, int64_t b) {
  return a * b;
}


struct Tasks {
  Tasks(std::istream&& input) : columns(0), rows(0) {
    // If we assume that we have the same amount of numbers in each row, we can just store all
    // numbers in one flat array and store the number of columns
    std::regex numberRegex("[0-9]+");
    std::regex operatorRegex("[+*]");

    for (auto line : stream::lines(input)) {
      if (line[0] != '*' && line[0] != '+') {
        // Hande numbers
        for (auto match : regex::iter(line, numberRegex)) {
          numbers.push_back(std::stoi(match[0].str()));
        }
        if (!columns) {
          // Set column size
          columns = numbers.size();
        }
        ++rows;
      } else {
        // Handle operators
        for (auto match : regex::iter(line, operatorRegex)) {
          operators.push_back(*match[0].first);
        }
      }
    }
  }

  /** Access to a number in the field by row,column
   */
  int get(int row, int column) const {
    return numbers[row * columns + column];
  }


  int64_t calculateResult() const {
    int64_t totalSum = 0;
    int column = 0;
    for (auto op : operators) {
      int64_t columnResult = (op == '*') ? 1 : 0; // initialize result with neutral element for the operation
      auto opFn = (op == '*') ? multiply : add;

      for (int row = 0; row < rows; ++row) {
        columnResult = opFn(columnResult, get(row, column));
      }

      totalSum += columnResult;
      ++column;
    }

    return totalSum;
  }


  int columns;
  int rows;
  std::vector<int> numbers;
  std::vector<char> operators;
};




int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  Tasks tasks(task::input());
  part1 = tasks.calculateResult();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
