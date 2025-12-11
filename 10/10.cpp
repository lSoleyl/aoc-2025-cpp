#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>

#include <deque>


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
    std::deque<std::pair<uint16_t, int>> queue;
    queue.push_back({ 0, 0 });

    while (true) {
      auto [configuration, presses] = queue.front();
      queue.pop_front();

      for (auto& button : buttons) {
        uint16_t newConfig = configuration ^ button.pattern;
        if (newConfig == indicators) {
          return presses + 1; // found the shortest number of button presses
        }
        queue.push_back({ newConfig, presses + 1 });
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
    int totalPresses = 0;
    for (auto& machine : machines) {
      totalPresses += machine.minButtonPresses();
    }
    return totalPresses;
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