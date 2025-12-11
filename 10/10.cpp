#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <algorithm>
#include <numeric>
#include <execution>
#include <set>


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


  Configuration applyToggle(const Configuration& other) const {
    // We can toggle all bits by xoring the 64 bit values
    return Configuration(a ^ other.a, b ^ other.b, c ^ other.c);
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
    uint16_t bit[12]; // max should be 10
  };
};

std::ostream& operator<<(std::ostream& out, const Configuration& config) {
  out << '(';
  for (int i = 0; i < 10; ++i) {
    out << static_cast<int>(config.bit[i]);
    if (i < 9) {
      out << ',';
    }
  }
  return out << ')';
}



struct Button {
  Configuration pattern;
};


struct Machine {
  Machine(std::string_view line) {
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
        auto configDelta = configuration.applyToggle(targetState);
        for (auto& button : buttons) {
          // Only consider buttons, which contribute at least one indicator light towards the target state
          if (button.pattern.hasCommonBits(configDelta)) {
            auto newConfig = configuration.applyToggle(button.pattern);
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
  int configureJoltages() const {




  }


  Configuration indicators;
  std::vector<Button> buttons;
  Configuration joltages;
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

    std::transform(/*std::execution::par_unseq,*/ machines.begin(), machines.end(), presses.begin(), [](const Machine& machine) { return machine.minButtonPresses(); });
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

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}