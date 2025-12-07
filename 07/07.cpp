#include <common/time.hpp>
#include <common/task.hpp>
#include <common/field.hpp>

#include <unordered_set>


struct Tile {
  Tile(char type) : type(type), paths(0) {}
  char type;
  int64_t paths; // the number of paths to take to the bottom from this field (Part 2)

  bool operator==(const Tile& other) const { return type == other.type; }
};


struct TachyonField : public FieldT<Tile> {
  TachyonField(std::istream&& input) : FieldT(std::move(input)) {}

  // Part 1
  int64_t countBeamSplits() {
    int64_t totalSplits = 0;
    auto startPos = fromOffset(findOffset('S'));
    simulateBeam(startPos, totalSplits);

    // Now count the resulting breams by 
    return totalSplits;
  }

  // Part 2
  int64_t countTimelines() {
    // Simply running the regular beam simulation for all beam split possibilities would take too long, we need a more direct approach for this.
    // Since we know all visited fields from our beam simulation, we will now check them bottom to top and count how many paths lead from that position
    // to the top and store this number in the tile itself. Then we will be able to simply read out the number form the 'S' node.
    std::vector<Vector> sortedPositions(visitedPositions.begin(), visitedPositions.end());
    // Sort lower rows first, but also sort by ascending x values to read the row left to right (not really that important)
    std::sort(sortedPositions.begin(), sortedPositions.end(), [](const Vector& a, const Vector& b) { return a.y > b.y || (a.y == b.y && a.x < b.x); });


    for (auto position : sortedPositions) {
      auto& tile = operator[](position);
      if (tile.type == '^') {
        // Splitter case: The number of paths equals the number of paths diagonally right and left below
        // We don't take the right/left positions, because the right position hasn't been processed yet.
        // From the input I can see that these positions will always be valid, so we can just access them
        tile.paths = operator[](position + Vector::DownLeft).paths + operator[](position + Vector::DownRight).paths;
      } else {
        // Simple case: The number of paths is the same as the field right below
        // In case there is no tile below, we have exactly one path
        tile.paths = validPosition(position + Vector::Down) ? operator[](position + Vector::Down).paths : 1;
      }
    }

    // Now simply return the number of paths from the start position
    return data[findOffset('S')].paths;;
  }


private:
  void simulateBeam(Vector pos, int64_t& totalSplits) {
    // Simulate the beam until we either we reach the end or encounter a position already touched by another beam
    for (; validPosition(pos) && visitedPositions.insert(pos).second ; pos += Vector::Down) {
      if (operator[](pos).type == '^') {
        // Encountered a splitter -> split into two and terminate this function
        ++totalSplits;
        simulateBeam(pos + Vector::Left, totalSplits);
        simulateBeam(pos + Vector::Right, totalSplits);
        return;
      }
    }
  }

  std::unordered_set<Vector> visitedPositions;
};





int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;


  TachyonField field(task::input());
  part1 = field.countBeamSplits();
  part2 = field.countTimelines();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
