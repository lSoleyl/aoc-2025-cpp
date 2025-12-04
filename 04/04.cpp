#include <common/task.hpp>
#include <common/time.hpp>
#include <common/field.hpp>


struct Warehouse : Field {
  Warehouse(std::istream&& input) : Field(std::move(input)) {}

  std::vector<Vector> collectAccessiblePaperRolls() const {
    std::vector<Vector> accessiblePaperRolls;
    for (size_t offset = 0; offset < data.size(); ++offset) {
      if (data[offset] == '@') { // A paper roll
        auto rollPos= fromOffset(offset);
        if (isLocationAccessible(rollPos)) {
          accessiblePaperRolls.push_back(rollPos);
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
        if (++paperRolls >= 4) {
          return false;
        }
      }
    }

    return true;
  }

};



int main() {
  common::Time t;

  Warehouse warehouse(task::input());


  auto accessibleRolls = warehouse.collectAccessiblePaperRolls();
  int64_t part1 = accessibleRolls.size();
  int64_t part2 = 0;

  while (!accessibleRolls.empty()) {
    // Remove rolls
    for (auto pos : accessibleRolls) {
      warehouse[pos] = '.';
    }

    // Update removed count
    part2 += accessibleRolls.size();

    // Check for new rolls to remove
    accessibleRolls = warehouse.collectAccessiblePaperRolls();
  }
  


  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}