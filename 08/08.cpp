#include <common/time.hpp>
#include <common/task.hpp>
#include <common/vector3d.hpp>
#include <common/stream.hpp>
#include <unordered_set>


namespace {
  int64_t squaredDistance(Vector3D a, Vector3D b) {
    auto delta = a - b;
    auto result = static_cast<int64_t>(delta.x) * delta.x;
    result += static_cast<int64_t>(delta.y) * delta.y;
    result += static_cast<int64_t>(delta.z) * delta.z;
    return result;
  }
}



struct JunctionBox {
  JunctionBox(std::istream&& input) {
    char ch;
    input >> position.x >> ch >> position.y >> ch >> position.z;
  }

  Vector3D position;
};


struct Playground {
  Playground(std::istream&& input) {
    for (auto& line : stream::lines(input)) {
      boxes.emplace_back(std::istringstream(line));
    }
  }

  // Part 1 & Part 2
  std::pair<int64_t, int64_t> countCircuits() {
    std::vector<std::unordered_set<JunctionBox*>> circuits;
    auto sortedDistances = calculateSortedDistances();

    std::pair<int64_t, int64_t> results;

    // Repeat the loop until all boxes are part of one large circuit
    int connection = 0;
    for (auto& entry : sortedDistances) {
      auto& boxA = boxes[entry.firstIndex];
      auto& boxB = boxes[entry.secondIndex];

      auto circuitAPos = std::find_if(circuits.begin(), circuits.end(), [&](const auto& circuit) { return circuit.contains(&boxA); });
      auto circuitBPos = std::find_if(circuits.begin(), circuits.end(), [&](const auto& circuit) { return circuit.contains(&boxB); });

      if (circuitAPos == circuits.end() && circuitBPos == circuits.end()) {
        // None is part of a circuit yet, create a new circuit
        circuits.push_back({ &boxA, &boxB });
      } else if (circuitAPos != circuits.end() && circuitBPos == circuits.end()) {
        // Only A is in a circuit -> add B to it
        circuitAPos->insert(&boxB);
      } else if (circuitAPos == circuits.end() && circuitBPos != circuits.end()) {
        // Only B is in a circuit -> add A to it
        circuitBPos->insert(&boxA);
      } else if (circuitAPos != circuitBPos) {
        // Both are in a circuit and not in the same circuit -> merge the circuits
        circuitAPos->insert_range(*circuitBPos);
        circuits.erase(circuitBPos);
      }

      if (circuits.size() == 1 && circuits[0].size() == boxes.size()) {
        // We connected everything into one large circuit
        results.second = static_cast<int64_t>(boxA.position.x) * boxB.position.x;
        break; // we are done
      }


      if (++connection == 1000) {
        // Now sort circuits descending by size and save the size of the 3 larges circuits
        std::sort(circuits.begin(), circuits.end(), [](const auto& circuitA, const auto& circuitB) { return circuitA.size() > circuitB.size(); });
        results.first = circuits[0].size() * circuits[1].size() * circuits[2].size();
      }
    }

    return results;
  }


  struct DistanceEntry {
    DistanceEntry(int a, int b, int64_t distance) : firstIndex(a), secondIndex(b), distance(distance) {}
    bool operator<(const DistanceEntry& other) const { return distance < other.distance; }

    int firstIndex;
    int secondIndex;
    int64_t distance;
  };

  std::vector<DistanceEntry> calculateSortedDistances() const {
    std::vector<DistanceEntry> entries;
    for (int i = 0; i < boxes.size(); ++i) {
      auto& firstBox = boxes[i];
      // start inner loop at i+1 to not check any distance twice
      for (int j = i + 1; j < boxes.size(); ++j) {
        auto& secondBox = boxes[j];
        entries.emplace_back(i, j, squaredDistance(firstBox.position, secondBox.position));
      }
    }
    std::sort(entries.begin(), entries.end());
    return entries;
  }


  std::vector<JunctionBox> boxes;
};




int main() {
  common::Time t;

  Playground playground(task::input());
  auto [part1, part2] = playground.countCircuits();

  
  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}
