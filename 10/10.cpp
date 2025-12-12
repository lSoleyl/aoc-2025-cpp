#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <algorithm>
#include <numeric>
#include <execution>
#include <set>

constexpr int BITS = 12;

struct Configuration {
  Configuration() {
    a = 0;
    b = 0;
    c = 0;
  }

  Configuration(uint64_t a, uint64_t b, uint64_t c) {
    this->a = a;
    this->b = b;
    this->c = c;
  }

  Configuration(const Configuration& other) {
    a = other.a;
    b = other.b;
    c = other.c;
  }

  Configuration& operator=(const Configuration& other) {
    a = other.a;
    b = other.b;
    c = other.c;
    return *this;
  }


  Configuration operator^(const Configuration& other) const {
    // We can toggle all bits by xoring the 64 bit values
    return Configuration(a ^ other.a, b ^ other.b, c ^ other.c);
  }

  Configuration operator+(const Configuration& other) const {
    // As long as we don't overflow any single bit, we can simply add up the three components
    return Configuration(a + other.a, b + other.b, c + other.c);
  }

  Configuration& operator+=(const Configuration& other) {
    a += other.a;
    b += other.b;
    c += other.c;
    return *this;
  }

  /** Returns a new configuration where all bits are multiplied by n
   */
  Configuration operator*(int n) {
    Configuration result(*this);
    result *= n;
    return result;
  }

  Configuration& operator*=(int n) {
    for (auto& value : bit) {
      value *= n;
    }
    return *this;
  }

  // True if this configuration is larger than target in any component
  bool overshoots(const Configuration& target) const {
    for (int i = 0; i < BITS; ++i) {
      if (bit[i] > target.bit[i]) {
        return true;
      }
    }
    return false;
  }

  /** Calculates the distance to the target by component wise subtraction
   *  Since we don't overshoot, we don't have to handle negative distances, we can simply sum them up
   */
  uint32_t targetDistance(const Configuration& target) const {
    uint32_t distance = 0;
    for (int i = 0; i < BITS; ++i) {
      distance += target.bit[i] - bit[i];
    }
    return distance;
  }

  /** Searches for a bit with the specified value and if multiple exist returns the one with the
   *  highest value in targetConfig
   */
  std::optional<int> findHighestBitValue(uint16_t value, const Configuration& targetConfig) const {
    int bestIndex = -1;
    uint16_t bestValue = 0;
    for (int i = 0; i < BITS; ++i) {
      if (bit[i] == value && targetConfig.bit[i] > bestValue) {
        bestIndex = i;
        bestValue = targetConfig.bit[i];
      }
    }
    return bestIndex >= 0 ? std::make_optional(bestIndex) : std::nullopt;
  }


  /** Searches for a bit with the specified value and if multiple exist returns the one with the 
   *  smallest value in targetConfig
   */
  std::optional<int> findLowestBitValue(uint16_t value, const Configuration& targetConfig) const {
    int bestIndex = -1;
    uint16_t bestValue = std::numeric_limits<uint16_t>::max();
    for (int i = 0; i < BITS; ++i) {
      if (bit[i] == value && targetConfig.bit[i] < bestValue) {
        bestIndex = i;
        bestValue = targetConfig.bit[i];
      }
    }

    return bestIndex >= 0 ? std::make_optional(bestIndex) : std::nullopt;
  }



  bool hasCommonBits(const Configuration& other) const {
    return ((a & other.a) | (b & other.b) | (c & other.c)) != 0;
  }

  bool operator==(const Configuration& other) const { return a == other.a && b == other.b && c == other.c; }
  bool operator!=(const Configuration& other) const { return a != other.a || b != other.b || c != other.c; }

  // Some ordering necessary for set
  bool operator<(const Configuration& other) const { return (a != other.a) ? a < other.a : (b != other.b) ? b < other.b : c < other.c; }


  union {
    struct { uint64_t a, b, c; };
    uint16_t bit[BITS]; // max should be 10
  };
};

std::ostream& operator<<(std::ostream& out, const Configuration& config) {
  out << '(';
  for (int i = 0; i < 10; ++i) {
    out << std::setw(2) << static_cast<int>(config.bit[i]);
    if (i < 9) {
      out << ',';
    }
  }
  return out << ')';
}

std::mutex outputMtx;

struct Button {
  Configuration pattern;
};

namespace {
  int nextIndex = 0;
}

struct Machine {
  Machine(std::string_view line) : index(nextIndex++) {
    auto pos = line.begin();
    auto end = line.end();
    ++pos;

    int indicatorIdx = 0;
    for (; *pos != ']'; ++pos) {
      indicators.bit[indicatorIdx++] = *pos == '#' ? 1 : 0;
    }

    pos += 2; // consume "] "
    for (; *pos == '('; pos += 2) { // +=2 to consume ") "
      buttons.push_back({});
      auto& button = buttons.back();

      auto closingParen = std::find(pos, end, ')');
      int bit = 0;
      for (auto number : common::split(std::string_view(pos + 1, closingParen), ',')) {
        button.pattern.bit[string_view::into<uint8_t>(number)] = 1;
      }
      pos = closingParen;
    }

    assert(*pos == '{');
    auto endBrace = std::find(pos, end, '}');
    indicatorIdx = 0;
    for (auto joltage : common::split(std::string_view(pos + 1, endBrace), ',')) {
      joltages.bit[indicatorIdx++] = string_view::into<uint16_t>(joltage);
    }
  }

  // Part 1 - find the minumum number of button presses by performing a simple BFS
  int minButtonPresses() const {
    std::set<Configuration> expanded = { Configuration() };
    std::vector<Configuration> current;
    std::vector<Configuration> next = { Configuration() };

    auto targetState = indicators;
    for (int presses = 1; true; ++presses) {
      std::swap(current, next);
      next.clear();

      assert(!current.empty());
      for (auto& configuration : current) {
        // Get all the bits, which still need to be toggled
        auto configDelta = configuration ^ targetState;
        for (auto& button : buttons) {
          // Only consider buttons, which contribute at least one indicator light towards the target state
          if (button.pattern.hasCommonBits(configDelta)) {
            auto newConfig = configuration ^ button.pattern;
            if (newConfig == targetState) {
              return presses; // found the shortest number of button presses
            }
            if (expanded.insert(newConfig).second) {
              // First time reaching that state
              next.push_back(newConfig);
            }
          }
        }
      }
    }

    return 0;
  }


  // Part 2
  int64_t configureJoltages() const {
    common::Time t;

    struct ExpandEntry {
      ExpandEntry(const Configuration& config, uint32_t distance, uint32_t steps) : config(config), distance(distance), steps(steps) {}
      Configuration config;
      uint32_t distance;
      uint32_t steps;

      bool operator==(const ExpandEntry& other) const { return distance == other.distance && steps == other.steps && config == other.config; }

      // Sort first by distance, then by steps, then by config
      bool operator<(const ExpandEntry& other) const { return distance != other.distance ? distance < other.distance : steps != other.steps ? steps < other.steps : config < other.config; }
    };

    auto targetState = joltages;


    // First perform targeted expansions by considering bits that are only modified by 1-3 buttons and generating configurations for them
    std::vector<ExpandEntry> current = { ExpandEntry(Configuration(), Configuration().targetDistance(targetState), 0) };
    std::vector<ExpandEntry> next;

    

    


    // First perform some pre processing to more quickly solve some easier tasks.
    // For that we will add up all buttons to check for positions that are controlled by a single button
    // These positions determine the remaining number of button presses
    auto localButtons = buttons;
    while (true) {
      auto sum = buttonSum(localButtons);
      if (auto idx = sum.findHighestBitValue(1, targetState)) { // <- Looking for the highest value in a single bit will bring us closest to the target state with fewest total states
        // A bit only manipulated by one button
        auto buttonPos = std::find_if(localButtons.begin(), localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        
        for (auto& entry : current) {
          auto deltaSteps = targetState.bit[*idx] - entry.config.bit[*idx];
          if (deltaSteps) {
            // Jump directly by deltaSteps
            auto config = entry.config + (buttonPos->pattern * deltaSteps);
            if (!config.overshoots(targetState)) { // If we didn't increment another field too high -> add it
              next.push_back(ExpandEntry(config, config.targetDistance(targetState), entry.steps + deltaSteps));
            }
          } else {
            // Bit is already correctly set -> do nothing, but keep the entry for the next iteration
            next.push_back(entry);
          }
        }
        // and remove the button from the local buttons as we don't need have to press it anymore for this machine
        localButtons.erase(buttonPos);
      } else if (auto idx = sum.findLowestBitValue(2, targetState)) { // <- find the lowest bit value to avoid too large of a state explosion
        // A bit manipulated by two buttons
        auto buttonAPos = std::find_if(localButtons.begin(), localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonBPos = std::find_if(buttonAPos+1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        
        for (auto& entry : current) {
          auto deltaSteps = targetState.bit[*idx] - entry.config.bit[*idx];
          // Check all combinations of these two buttons
          if (deltaSteps) {
            for (int a = 0; a <= deltaSteps; ++a) {
              int b = deltaSteps - a;
              auto config = entry.config + (buttonAPos->pattern * a) + (buttonBPos->pattern * b);
              if (!config.overshoots(targetState)) { // If we didn't increment another field too high -> add it
                next.push_back(ExpandEntry(config, config.targetDistance(targetState), entry.steps + deltaSteps));
              }
            }
          } else {
            // Bit is already correctly set -> do nothing, but keep the entry for the next iteration
            next.push_back(entry);
          }
        }

        // Remove both buttons
        localButtons.erase(buttonBPos);
        localButtons.erase(buttonAPos);
      } else if (auto idx = sum.findLowestBitValue(3, targetState)) { // <- find the lowest bit value to avoid too large of a state explosion
        // A bit manipulated by three buttons
        auto buttonAPos = std::find_if(localButtons.begin(), localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonBPos = std::find_if(buttonAPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonCPos = std::find_if(buttonBPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });

        for (auto& entry : current) {
          auto deltaSteps = targetState.bit[*idx] - entry.config.bit[*idx];
          // Check all combinations of these three buttons (which may be a lot!)
          if (deltaSteps) {
            for (int a = 0; a <= deltaSteps; ++a) {
              for (int b = 0; b <= deltaSteps - a; ++b) {
                int c = deltaSteps - a - b;
                auto config = entry.config;
                config += buttonAPos->pattern * a;
                config += buttonBPos->pattern * b;
                config += buttonCPos->pattern * c;
                if (!config.overshoots(targetState)) { // If we didn't increment another field too high -> add it
                  next.push_back(ExpandEntry(config, config.targetDistance(targetState), entry.steps + deltaSteps));
                }
              }
            }
          } else {
            // Bit is already correctly set -> do nothing, but keep the entry for the next iteration
            next.push_back(entry);
          }
        }

        // Remove all used buttons
        localButtons.erase(buttonCPos);
        localButtons.erase(buttonBPos);
        localButtons.erase(buttonAPos);
      } else if (auto idx = sum.findLowestBitValue(4, targetState)) { // <- find the lowest bit value to avoid too large of a state explosion
        // A bit manipulated by three buttons
        auto buttonAPos = std::find_if(localButtons.begin(), localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonBPos = std::find_if(buttonAPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonCPos = std::find_if(buttonBPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonDPos = std::find_if(buttonCPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });

        for (auto& entry : current) {
          auto deltaSteps = targetState.bit[*idx] - entry.config.bit[*idx];
          // Check all combinations of these three buttons (which may be a lot!)
          if (deltaSteps) {
            for (int a = 0; a <= deltaSteps; ++a) {
              for (int b = 0; b <= deltaSteps - a; ++b) {
                for (int c = 0; c <= deltaSteps - a - b; ++c) {
                  int d = deltaSteps - a - b - c;
                  auto config = entry.config;
                  config += buttonAPos->pattern * a;
                  config += buttonBPos->pattern * b;
                  config += buttonCPos->pattern * c;
                  config += buttonDPos->pattern * d;
                  if (!config.overshoots(targetState)) { // If we didn't increment another field too high -> add it
                    next.push_back(ExpandEntry(config, config.targetDistance(targetState), entry.steps + deltaSteps));
                  }
                }
              }
            }
          } else {
            // Bit is already correctly set -> do nothing, but keep the entry for the next iteration
            next.push_back(entry);
          }
        }

        // Remove all used buttons
        localButtons.erase(buttonDPos);
        localButtons.erase(buttonCPos);
        localButtons.erase(buttonBPos);
        localButtons.erase(buttonAPos);
      } else if (auto idx = sum.findLowestBitValue(5, targetState)) { // <- find the lowest bit value to avoid too large of a state explosion
        // A bit manipulated by three buttons
        auto buttonAPos = std::find_if(localButtons.begin(), localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonBPos = std::find_if(buttonAPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonCPos = std::find_if(buttonBPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonDPos = std::find_if(buttonCPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });
        auto buttonEPos = std::find_if(buttonDPos + 1, localButtons.end(), [&](const Button& btn) { return btn.pattern.bit[*idx] == 1; });

        for (auto& entry : current) {
          auto deltaSteps = targetState.bit[*idx] - entry.config.bit[*idx];
          // Check all combinations of these three buttons (which may be a lot!)
          if (deltaSteps) {
            for (int a = 0; a <= deltaSteps; ++a) {
              for (int b = 0; b <= deltaSteps - a; ++b) {
                for (int c = 0; c <= deltaSteps - a - b; ++c) {
                  for (int d = 0; d <= deltaSteps - a - b - c; ++d) {
                    int e = deltaSteps - a - b - c - d;
                    auto config = entry.config;
                    config += buttonAPos->pattern * a;
                    config += buttonBPos->pattern * b;
                    config += buttonCPos->pattern * c;
                    config += buttonDPos->pattern * d;
                    config += buttonEPos->pattern * e;
                    if (!config.overshoots(targetState)) { // If we didn't increment another field too high -> add it
                      next.push_back(ExpandEntry(config, config.targetDistance(targetState), entry.steps + deltaSteps));
                    }
                  }
                }
              }
            }
          } else {
            // Bit is already correctly set -> do nothing, but keep the entry for the next iteration
            next.push_back(entry);
          }
        }

        // Remove all used buttons
        localButtons.erase(buttonEPos);
        localButtons.erase(buttonDPos);
        localButtons.erase(buttonCPos);
        localButtons.erase(buttonBPos);
        localButtons.erase(buttonAPos);
      } else {
        break;
      }

      std::swap(current, next);
      next.clear();
    }


    assert(localButtons.empty());

    std::sort(current.begin(), current.end());
    auto targetPos = std::find_if(current.begin(), current.end(), [&](const ExpandEntry& entry) { return entry.config == targetState; });

    // {
    //   std::unique_lock<std::mutex> outputLock(outputMtx);
    //   std::cout << "Machine[" << index << "] = " << targetPos->steps << " after " << t;
    // }

    return targetPos->steps;
  }

  /** Calculates the sum of all buttons
   */
  static Configuration buttonSum(const std::vector<Button>& buttons) {
    Configuration sum;
    for (auto& button : buttons) {
      sum += button.pattern;
    }
    return sum;
  }


  Configuration indicators;
  std::vector<Button> buttons;
  Configuration joltages;
  int index;
};



struct Factory {
  Factory(std::istream&& input) {
    for (auto line : stream::lines(input)) {
      machines.emplace_back(line);
    }
  }

  // Part 1
  int minButtonPresses() const {
    std::vector<int> presses(machines.size());

    std::transform(std::execution::par_unseq, machines.begin(), machines.end(), presses.begin(), [](const Machine& machine) { return machine.minButtonPresses(); });
    return std::reduce(presses.begin(), presses.end());
  }

  // Part 2
  int64_t configureJoltages() const {
    std::vector<int64_t> presses(machines.size());


    std::transform(std::execution::par_unseq, machines.begin(), machines.end(), presses.begin(), [](const Machine& machine) { return machine.configureJoltages(); });
    return std::reduce(presses.begin(), presses.end());
  }

  std::vector<Machine> machines;
};


int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  Factory factory(task::input());

  part1 = factory.minButtonPresses();
  part2 = factory.configureJoltages();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
  system("PAUSE");
}