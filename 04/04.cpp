#include <common/task.hpp>
#include <common/time.hpp>
#include <common/field.hpp>


struct Warehouse : Field {
  Warehouse(std::istream&& input) : Field(std::move(input)) {}

  int countAccessiblePaperRolls() const {
    int accessiblePaperRolls = 0;
    for (size_t offset = 0; offset < data.size(); ++offset) {
      if (data[offset] == '@') { // A paper roll
        if (isLocationAccessible(fromOffset(offset))) {
          ++accessiblePaperRolls;
        }
      }
    }
    return accessiblePaperRolls;
  }



  /** Check whether a position is accessible by a forklift
   */
  bool isLocationAccessible(const Vector& pos) const {
    int paperRolls = 0;
    for (auto direction : Vector::AllDirections()) {
      if (at(pos + direction) == '@') {
        ++paperRolls;
      }
    }
    return paperRolls < 4;
  }

};



int main() {
  common::Time t;

  Warehouse warehouse(task::input());

  int64_t part1 = warehouse.countAccessiblePaperRolls();
  int64_t part2 = 0;


  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}