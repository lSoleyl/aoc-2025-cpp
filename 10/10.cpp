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

// Comment in the following line and disable parallel processing or restrict input to only one machine to see 
// the equation system being solved
//#define LOG(x) std::cout << x
#define LOG(x)


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

  bool hasCommonBits(const Configuration& other) const {
    return ((a & other.a) | (b & other.b) | (c & other.c)) != 0;
  }

  bool operator==(const Configuration& other) const { return a == other.a && b == other.b && c == other.c; }
  bool operator!=(const Configuration& other) const { return a != other.a || b != other.b || c != other.c; }

  // Some ordering necessary for set
  bool operator<(const Configuration& other) const { return (a != other.a) ? a < other.a : (b != other.b) ? b < other.b : c < other.c; }


  union {
    struct { uint64_t a, b, c; };
    int16_t bit[BITS]; // max should be 10
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
  Button(int index) : index(index) {}
  Configuration pattern;
  int index;
};


struct Equation {
  Equation(int result, int nVariables) : result(result) {
    factors.resize(nVariables, 0);
  }

  /** Number of leading 0 factors in this equation
   */
  int leadingZeroes() const {
    return std::distance(factors.begin(), std::find_if(factors.begin(), factors.end(), [](int factor) { return factor != 0; }));
  }

  /** Checks whether this is a zero equation (i.e. all factors and result are zero)
   */
  bool isZero() const {
    return result == 0 && std::all_of(factors.begin(), factors.end(), [](int factor) { return factor == 0; });
  }

  /** True if the result and all coefficients are either 0 or positive
   */
  bool isPositive() const {
    return result >= 0 && std::all_of(factors.begin(), factors.end(), [](int factor) { return factor >= 0; });
  }


  bool operator<(const Equation& other) const { return leadingZeroes() < other.leadingZeroes(); }

  

  Equation& operator-=(const Equation& other) {
    result -= other.result;
    for (int i = 0; i < factors.size(); ++i) {
      factors[i] -= other.factors[i];
    }
    return *this;
  }

  Equation operator*(int factor) const {
    Equation result(*this);
    result *= factor;
    return result;
  }

  Equation& operator*=(int factor) {
    result *= factor;
    for (auto& value : factors) {
      value *= factor;
    }
    return *this;
  }


  bool divisibleBy(int divisor) {
    return result % divisor == 0 && std::all_of(factors.begin(), factors.end(), [divisor](int factor) { return factor % divisor == 0; });
  }

  /** (truncating) integer division for all components
   */
  Equation& operator/=(int divisor) {
    assert(result % divisor == 0);
    result /= divisor;
    for (auto& value : factors) {
      assert(value % divisor == 0);
      value /= divisor;
    }
    return *this;
  }


  /** This method will attempt to set this equation's first non-zero factor variable into variables from the
   *  ones already present there (the ones to the right of it)
   */
  bool setVariable(std::vector<int>& variables, const std::vector<int>& maxVarValues) {
    auto varIdx = leadingZeroes(); // the variable we are setting

    auto value = result;
    
    // Now subtract all factors right of the variable to set from the result
    for (int i = varIdx + 1; i < factors.size(); ++i) {
      value -= factors[i] * variables[i];
    }

    // Finally divide the result by the leading factor (which may not be 1)
    auto div = std::div(value, factors[varIdx]);
    if (div.rem != 0 || div.quot < 0 || div.quot > maxVarValues[varIdx]) {
      return false; // cannot set variable (outside bounds or not an integer value!)
    }

    // Otherwise set the variable
    variables[varIdx] = div.quot;
    return true;
  }



  std::vector<int> factors; // of the variables
  int result;
};


std::ostream& operator<<(std::ostream& out, const Equation& equ) {
  for (auto factor : equ.factors) {
    out << std::setw(3) << factor << " ";
  }
  out << " |" << std::setw(4) << equ.result;

  return out;
}

struct EquationSystem;
std::ostream& operator<<(std::ostream& out, const EquationSystem& system);


struct EquationSystem {
  /** Establishes reasonable variable limits, brings the equations into the diagnoal form using the gaussian method
   *  and determines the number of free variables.
   */
  void simplifyGaussian() {
    // Sort the equations by free bits to the left
    std::sort(equations.begin(), equations.end());

    // Limit all variables to the maximum joltage value for now
    maxVarValues.resize(equations[0].factors.size(), maxResult());

    // Initialize the column order to allow for undoing the swaps we perform here during simplification
    columnOrder.resize(maxVarValues.size());
    std::iota(columnOrder.begin(), columnOrder.end(), 0);

    // Limit the max values to the max positive result that a variable contributes to
    optimizeFreeVariableLimits();


    auto totalVariables = equations[0].factors.size();
    for (int equationIdx = 0; equationIdx < equations.size(); ++equationIdx) {
      auto& equation = equations[equationIdx];

      LOG(*this << "\n\n");

      if (equation.isZero()) {
        // Remove zero equations from the system as the contribute no information
        equations.erase(equations.begin() + equationIdx);
        --equationIdx;
        continue;
      }


      // First ensure that we have exactly equationIdx leading zeroes
      if (equation.leadingZeroes() > equationIdx) {
        // Find another column, which is non-zero (there must be one unless this is a zero line) - which we assume isn't
        auto swapPos = std::find_if(equation.factors.begin(), equation.factors.end(), [](int factor) { return factor != 0; });
        assert(swapPos != equation.factors.end());
        swapColumns(equationIdx, std::distance(equation.factors.begin(), swapPos));

        // Swapping columns can affect the sort order of the following equations -> re-sort
        // Use a stable sort to not change this equations position in case a second one exists with the same number of leading zeroes.
        // <-- to not invalidate our reference on the top
        std::stable_sort(equations.begin(), equations.end());
        LOG(*this << "\n\n");
      }

      // Try to simplify the equation by normalizing the leading factor to 1 (if the other values are divisible)
      bool factorNormalized = false;
      if (equation.factors[equationIdx] != 1 && equation.divisibleBy(equation.factors[equationIdx])) {
        equation /= equation.factors[equationIdx];
        factorNormalized = true;
      }

      // If we cannot normalize to 1 at least keep the sign positive
      if (equation.factors[equationIdx] < 0) {
        equation *= -1;
        factorNormalized = true;
      }

      if (factorNormalized) {
        LOG(*this << "\n\n");
      }
      
      // Now for all equations following, check for the leading factor and subtract this equation a sufficient number of times
      auto leadingFactor = equation.factors[equationIdx];
      for (int otherIdx = equationIdx + 1; otherIdx < equations.size(); ++otherIdx) {
        auto& otherEquation = equations[otherIdx];
        auto otherLeadingFactor = otherEquation.factors[equationIdx];
        if (otherLeadingFactor != 0) {
          auto lcm = std::lcm(leadingFactor, otherLeadingFactor);

          otherEquation *= (lcm / otherLeadingFactor);
          otherEquation -= equation * (lcm / leadingFactor);
        }
      }
    }

    freeVariables = equations[0].factors.size() - equations.size();
  }

  /** A simple optimization step reducing the range of variable values, which need to be checked by 
   *  looking for equations with only positive coefficients and if we find a free variable among them, we can
   *  calculate a smaller limit for it
   */
  void optimizeFreeVariableLimits() {
    for (auto& equation : equations) {
      if (equation.isPositive()) {
        for (int i = 0; i < maxVarValues.size(); ++i) {
          if (equation.factors[i] > 0) {
            auto div = std::div(equation.result, equation.factors[i]);
            auto newLimit = div.quot;
            if (div.rem) {
              // Not clearly divisible -> always round up (to not forget to check a value)
              ++newLimit;
            }
            
            maxVarValues[i] = std::min(maxVarValues[i], newLimit);
          }
        }
      }
    }
  }


  /** Solve the gaussian equation by trying out all possible values for all free variables (only possible after simplifying the system)
   */
  int solveMinSteps() {
    int minSteps = std::numeric_limits<int>::max();

    std::vector<int> minVariables;

    std::vector<int> variables(equations[0].factors.size());

    // Reduce the number of values to test by looking for equations with only positive coefficents to derive a lower limit
    // for out free variables
    optimizeFreeVariableLimits();
    const int maxVarConfigs = totalFreeVarConfigs();
    for (int config = 0; config < maxVarConfigs; ++config) {
      // Set the variables for this run
      setVariableConfig(variables, config);


      bool validConfiguration = true;
      for (int i = equations.size(); i --> 0 ;) {
        if (!equations[i].setVariable(variables, maxVarValues)) {
          // Variable value out of bounds or not an integer
          validConfiguration = false;
          break;
        }
      }

      if (validConfiguration) {
        // All variables are set -> count the number of button presses (by simply adding up all variables values)
        auto steps = std::reduce(variables.begin(), variables.end());
        if (steps < minSteps) {
          minSteps = steps;
          minVariables = variables;
        }
      }
    }

    if (!minVariables.empty()) {
      LOG("\nButtons: " << stream::join(undoColumnSwaps(minVariables), ',') << "\n");
    }

    return minSteps;
  }

  /** Reverses the column swaps performed during simplifyGaussian on the given variable result vector
   *  to be able to display them in the original order
   */
  std::vector<int> undoColumnSwaps(const std::vector<int>& variables) const {
    auto result = variables;
    for (int i = 0; i < columnOrder.size(); ++i) {
      result[columnOrder[i]] = variables[i];
    }
    return result;
  }


  // Forgetting the +1 also contributed to me wondering for a day why the solver fails for 3 equations (the ones with the most buttons)

  /** Calculates the total number of steps to take from the number of free variables and the variable value limit
   */ 
  int totalFreeVarConfigs() const {
    int steps = 1;
    for (int varIdx = 0; varIdx < freeVariables; ++varIdx) {
      steps *= (maxVarValues[maxVarValues.size() - varIdx - 1] + 1); // +1 because the max value is included in the possible values
    }
    return steps;
  }


  // I initially implemented this function wrongly, which has cost me a day to figure out why certain equations are not being solved
  // incidentially these where the equations with the most buttons, thus the most difficult to debug

  /** Increments the variable configuration to the next valid configuration to loop through all free variable settings
   */
  void setVariableConfig(std::vector<int>& variables, int configIdx) const {
    auto freeVarIdx = variables.size() - freeVariables;
    for (auto i = 0; i < freeVarIdx; ++i) {
      variables[i] = 0; // zero all previous variables (not strictly needed due to normlization)
    }

    // Now update the free variables (if any)
    for (auto freeVar = 0; freeVar < freeVariables; ++freeVar) {
      int i = variables.size() - freeVariables + freeVar;

      // Divide by the max value + 1 and the remainder will be the variable's value (+1, because the max value is included)
      // The quotient will be carried over to the next variable
      auto div = std::div(configIdx, maxVarValues[i]+1);
      variables[i] = div.rem;
      configIdx = div.quot;
    }
  }


  /** Returns the highest result value from any equation in this system
   */
  int maxResult() const {
    int maxValue = 0;
    for (auto& equation : equations) {
      maxValue = std::max(maxValue, equation.result);
    }
    return maxValue;
  }


  /** Swaps the factors at the specified two columns for all equations
   */
  void swapColumns(int idxA, int idxB) {
    for (auto& equation : equations) {
      std::swap(equation.factors[idxA], equation.factors[idxB]);
    }

    // Also swap max variable values
    std::swap(maxVarValues[idxA], maxVarValues[idxB]);

    // And swap in column order to be able to restore the original column ordering (this is only needed to print out the resulting button presses in the original order)
    std::swap(columnOrder[idxA], columnOrder[idxB]);
  }


  std::vector<Equation> equations;
  std::vector<int> maxVarValues; // set by simplifyGaussian()
  std::vector<int> columnOrder;  // modified by swapColumns
  int freeVariables = 0; // set by simplifyGaussian()
};


std::ostream& operator<<(std::ostream& out, const EquationSystem& system) {
  for (auto& equ : system.equations) {
    out << equ << "\n";
  }
  return out;
}


namespace {
  int nextIndex = 0;
}

struct Machine {
  Machine(std::string_view line) : index(nextIndex++), nBits(0) {
    auto pos = line.begin();
    auto end = line.end();
    ++pos;

    int indicatorIdx = 0;
    for (; *pos != ']'; ++pos) {
      indicators.bit[indicatorIdx++] = *pos == '#' ? 1 : 0;
    }

    int buttonIdx = 0;
    pos += 2; // consume "] "
    for (; *pos == '('; pos += 2) { // +=2 to consume ") "
      buttons.push_back(buttonIdx++);
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
      joltages.bit[indicatorIdx++] = string_view::into<int16_t>(joltage);
      ++nBits;
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


  // Part 2 - second attempt
  int64_t solveForJoltages() const {
    // Try solving using gaussian eliminiation technique...
    EquationSystem system;

    common::Time t;

    // Convert the buttons and target value into an equation system
    for (int i = 0; i < nBits; ++i) {
      system.equations.emplace_back(joltages.bit[i], buttons.size());
      auto& equation = system.equations.back();

      for (int buttonIdx = 0; buttonIdx < buttons.size(); ++buttonIdx) {
        if (buttons[buttonIdx].pattern.bit[i]) {
          equation.factors[buttonIdx] = 1;
        }
      }
    }

    system.simplifyGaussian();
    LOG(system << "\n");

    auto result = system.solveMinSteps();
    // std::unique_lock<std::mutex> lock(outputMtx);
    // std::cout << "Machine[" << index << "] = " << result << " after " << t;
    return result;
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
  int nBits; // number of indicator bits/joltage values in this machine
  int index;
};



struct Factory {
  Factory(std::istream&& input) {
    for (auto line : stream::lines(input)) {
      if (!std::string_view(line).starts_with("//") && !line.empty()) { // to support comments in the input file
        machines.emplace_back(line);
      }
    }
  }

  // Part 1
  int minButtonPresses() const {
    std::vector<int> presses(machines.size());

    std::transform(std::execution::par_unseq, machines.begin(), machines.end(), presses.begin(), [](const Machine& machine) { return machine.minButtonPresses(); });
    return std::reduce(presses.begin(), presses.end());
  }

  // Part 2
  int64_t solveForJoltages() const {
    std::vector<int64_t> presses(machines.size());
    std::transform(std::execution::par_unseq, machines.begin(), machines.end(), presses.begin(), [](const Machine& machine) { return machine.solveForJoltages(); });
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
  part2 = factory.solveForJoltages();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}