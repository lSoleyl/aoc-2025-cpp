#include <common/time.hpp>
#include <common/task.hpp>
#include <common/field.hpp>

#include <unordered_set>


struct TachyonField : public Field {
  TachyonField(std::istream&& input) : Field(std::move(input)) {}

  // Part 1
  int64_t countBeamSplits() {
    std::unordered_set<Vector> visitedPositions;
    int64_t totalSplits = 0;
    auto startPos = fromOffset(findOffset('S'));
    simulateBeam(startPos, visitedPositions, totalSplits);

    // Now count the resulting breams by 
    return totalSplits;
  }


private:
  void simulateBeam(Vector pos, std::unordered_set<Vector>& visitedPositions, int64_t& totalSplits) {
    // Simulate the beam until we either we reach the end or encounter a position already touched by another beam
    for (; validPosition(pos) && visitedPositions.insert(pos).second; pos += Vector::Down) {
      if ((*this)[pos] == '^') { 
        // Encountered a splitter -> split into two and terminate this function
        ++totalSplits;
        simulateBeam(pos + Vector::Left, visitedPositions, totalSplits);
        simulateBeam(pos + Vector::Right, visitedPositions, totalSplits);
        return;
      }
    }
  }



};





int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;


  TachyonField field(task::input());
  part1 = field.countBeamSplits();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
