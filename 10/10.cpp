#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <algorithm>
#include <numeric>
#include <execution>

struct Button {
  uint16_t pattern = 0;
};

std::ostream& operator<<(std::ostream& out, const Button& button) {
  out << "Button(pattern=";
  for (int i = 16; i --> 0;) {
    out << (((1 << i) & button.pattern) != 0 ? '1' : '0');
  }
  return out << ")";
}

struct Machine {
  Machine(std::string_view line) : indicators(0) {
    auto pos = line.begin();
    auto end = line.end();
    ++pos;

    int indicatorBits = 0;
    for (; *pos != ']'; ++pos, ++indicatorBits) {
      indicators <<= 1;
      indicators |= (*pos == '#' ? 1 : 0);
    }

    pos += 2; // consume "] "
    for (; *pos == '('; pos += 2) { // +=2 to consume ") "
      buttons.push_back({});
      auto& button = buttons.back();

      auto closingParen = std::find(pos, end, ')');
      for (auto number : common::split(std::string_view(pos + 1, closingParen), ',')) {
        button.pattern |= (1 << (indicatorBits - string_view::into<int>(number) - 1));
      }
      pos = closingParen;
    }

    assert(*pos == '{');
    auto endBrace = std::find(pos, end, '}');
    for (auto joltage : common::split(std::string_view(pos + 1, endBrace), ',')) {
      joltages.push_back(string_view::into<int>(joltage));
    }
  }

  // Part 1 - find the minumum number of button presses by performing a simple BFS
  int minButtonPresses() const {
    // Keep track of the configuration + pressed buttons
    std::vector<uint16_t> current;
    std::vector<uint16_t> next;
    next.push_back(0);

    auto targetState = indicators;

    for (int presses = 1; true; ++presses) {
      std::swap(current, next);
      next.clear();

      for (auto configuration : current) {
        // Get all the bits, which still need to be toggled
        uint16_t configDelta = configuration ^ targetState;
        for (auto& button : buttons) {
          // Only consider buttons, which contribute at least one indicator light towards the target state
          if ((button.pattern & configDelta) != 0) {
            uint16_t newConfig = configuration ^ button.pattern;
            if (newConfig == targetState) {
              return presses; // found the shortest number of button presses
            }
            next.push_back(newConfig);
          }
        }

      }
    }

    return 0;
  }




  uint16_t indicators; // as bit pattern
  std::vector<Button> buttons;
  std::vector<int> joltages;
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