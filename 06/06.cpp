#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/math.hpp>
#include <common/string_view.hpp>

#include <ranges>

int64_t add(int64_t a, int64_t b) {
  return a + b;
}

int64_t multiply(int64_t a, int64_t b) {
  return a * b;
}

using OperatorFn = int64_t (*)(int64_t a, int64_t b);

struct Column {
  Column(char op) : width(1), operatorFn(op == '*' ? multiply : add) {}

  // Part 1
  int64_t simpleResult() const {
    int64_t result = (operatorFn == add) ? 0 : 1; // initialize result to neutral element for the operation

    for (auto& number : numbers) {
      result = operatorFn(result, string_view::into<int64_t>(string_view::trim(number)));
    }

    return result;
  }

  // Part 2
  int64_t correctResult() const {
    int64_t result = (operatorFn == add) ? 0 : 1; // initialize result to neutral element for the operation
    
    // First construct the correct numbers to combine
    std::vector<int64_t> correctNumbers(width); // we have as many numbers per column as we have subcolumns
    for (auto& number : numbers) {
      for (int i = 0; i < width; ++i) {
        auto digit = number[i];
        if (digit != ' ') { // <- I guess we should ignore spaces... 
          correctNumbers[i] = math::appendDigit(correctNumbers[i], digit);
        }
      }
    }

    // Now we can finally combine them into the actual result
    for (auto number : correctNumbers) {
      result = operatorFn(result, number);
    }

    return result;
  }






  std::vector<std::string> numbers; // The full with numbers of the column (optionally with leading and/or trailing spaces)
  OperatorFn operatorFn;
  int width; // the width of this column in digits/chars
};


struct Tasks {
  Tasks(std::istream&& input) {
    // If we assume that we have the same amount of numbers in each row, we can just store all
    // numbers in one flat array and store the number of columns

    // First collect all the lines in a vector
    auto numberLines = stream::lines(input) | std::ranges::to<std::vector>();

    // The last line will be the operator line, which will define the column width for each column,
    // so we will process it first
    columns = processOperatorLine(numberLines.back());
    numberLines.pop_back();

    // Read all the numbers as strings (including whitespaces) into the columns
    for (auto& line : numberLines) {
      size_t offset = 0;
      for (auto& column : columns) {
        column.numbers.push_back(line.substr(offset, column.width));
        offset += column.width + 1;
      }
    }
  }

  std::pair<int64_t, int64_t> calculateResults() const {
    int64_t totalSum = 0;   // Part 1
    int64_t correctSum = 0; // Part 2
    
    for (auto& column : columns) {
      totalSum += column.simpleResult();
      correctSum += column.correctResult();
    }

    return { totalSum, correctSum };
  }


private:

  /** This method will determine the number of column, the width of each column and
   *  will store the operators to apply to each column
   */
  static std::vector<Column> processOperatorLine(const std::string& line) {
    std::vector<Column> columns;
    for (auto ch : line) {
      if (ch == '*' || ch == '+') {
        // Next operator
        if (!columns.empty()) {
          // Subtract 1 from previous column width for column separator
          --columns.back().width; 
        }

        columns.push_back(ch);
      } else {
        // Otherwise this space is part of the previous column
        ++columns.back().width;
      }
    }
    return columns;
  }

  std::vector<Column> columns;
};




int main() {
  common::Time t;

  Tasks tasks(task::input());
  auto [part1,part2] = tasks.calculateResults();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
