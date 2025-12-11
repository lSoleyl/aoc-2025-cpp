#include <common/time.hpp>
#include <common/task.hpp>
#include <common/stream.hpp>
#include <common/split.hpp>
#include <common/string_view.hpp>
#include <common/vector.hpp>

#include <optional>

// Returns a simple direction (Up, Left, Right, Down) to get from 'from' to 'to'
// The direction will point into 'to' with a length of 1.
Vector simpleDirection(const Vector& from, const Vector& to) {
  return to.compare(from);
}


struct Line {
  Line(Vector a, Vector b) : a(std::min(a, b)), b(std::max(a, b)) {
    // OPTIMIZATION: precompute the direction as this is needed inside our hot path function crossesLine() a lot
    //               and the call is rather expensive in Debug
    direction = b.compare(a); 
  }

  /** Performs a quick check whether this line crosses another line and returns the position of the intersection point if so
   *  OPTIMIZATION: I replaced the std::optional<Vector> return type with a bool and an out reference parameter as this halved
   *                the total execution time in Debug (down from 1000ms to 500ms)
   */
  bool crossesLine(const Line& other, Vector& intersection) const {
    // OPTIMIZATION: We already made sure to not call crossesLine() for parallel lines
    // if (direction == other.direction) {
    //   // We don't consider intersections of parallel lines here
    //   return std::nullopt;
    // }

    // Now only if both lines include the intersection point then we can have an intersection of both lines
    intersection = direction.x ? Vector(other.a.x, a.y) : Vector(a.x, other.a.y);
    return isPointOnLine(intersection) && other.isPointOnLine(intersection);
  }

  /** True if the given point is on this line
   */
  bool isPointOnLine(const Vector& point) const {
    // Here we utilize the fact that a and b are sorted by coordinates
    return point.x >= a.x && point.x <= b.x && point.y >= a.y && point.y <= b.y;
  }

  /** True if this the specified point is either a or b
   */
  bool isEndPoint(const Vector& point) const {
    return point == a || point == b;
  }

  bool horizontal() const {
    return direction.x != 0;
  }


  bool operator==(const Line& other) const { return a == other.a && b == other.b; }
  bool operator!=(const Line& other) const { return a != other.a || b != other.b; }

  Vector a, b, direction; // direction = direction vector from a -> b
};

std::ostream& operator<<(std::ostream& out, const Line& line) {
  return out << "Line(a=" << line.a << ", b=" << line.b << ")";
}


//
//  a-----b
//  |     |
//  d-----c
//
struct Rectangle {
  Rectangle(Vector c1, Vector c2) : 
    // Initialize corners in clockwise order starting top left
    a(std::min(c1.x, c2.x), std::min(c1.y, c2.y)),
    b(std::max(c1.x, c2.x), std::min(c1.y, c2.y)),
    c(std::max(c1.x, c2.x), std::max(c1.y, c2.y)),
    d(std::min(c1.x, c2.x), std::max(c1.y, c2.y))
  {
    auto delta = (c1 - c2).apply([](int value) { return std::abs(value) + 1; });
    area = static_cast<int64_t>(delta.x) * delta.y;
  }

  /** Returns a list of all four sides and corresponding directions pointing inside into the rectangle
   */
  std::array<std::pair<Line, Vector>, 4> getCheckSides() const {
    return { {
      { Line(a,b), simpleDirection(b,c) },
      { Line(b,c), simpleDirection(c,d) },
      { Line(c,d), simpleDirection(d,a) },
      { Line(d,a), simpleDirection(a,b) }
    } };
  }


  Vector a, b, c, d;
  int64_t area;
};

std::ostream& operator<<(std::ostream& out, const Rectangle& r) {
  return out << "Rectangle(a=" << r.a << ", b=" << r.b << ", c=" << r.c << ", d=" << r.d << ", area=" << r.area << ")";
}



struct MovieTheater {
  MovieTheater(std::istream&& input) {
    for (auto& line : stream::lines(input)) {
      auto [x, y] = common::split2(line, ',');
      redTiles.emplace_back(string_view::into<int>(x), string_view::into<int>(y));
    }

    // Collect all lines as they are needed for Part two
    horizontalLines.reserve(redTiles.size() / 2);
    verticalLines.reserve(redTiles.size() / 2);

    for (int i = 0; i < redTiles.size(); ++i) {
      int j = (i + 1) % redTiles.size();
      Line line(redTiles[i], redTiles[j]);
      (line.horizontal() ? horizontalLines : verticalLines).push_back(line);
    }

    // Now collect all rectangles with an area > 0 and sort them descending by area
    rectangles.reserve(redTiles.size());
    for (auto aPos = redTiles.begin(), end = redTiles.end(); aPos != end; ++aPos) {
      for (auto bPos = aPos + 1; bPos != end; ++bPos) {
        auto& a = *aPos;
        auto& b = *bPos;
        if (a.x != b.x && a.y != b.y) { // non empty rectangle
          rectangles.emplace_back(a, b);
        }
      }
    }

    std::sort(rectangles.begin(), rectangles.end(), [](const Rectangle& a, const Rectangle& b) { return a.area > b.area; });
  }

  // Part 1
  int64_t largestRectangleArea() const {
    return rectangles[0].area;
  }


  // Part 2
  int64_t largestRectangleInPolygon() const {
    // Check all rectangles largest to smallest
    for (auto& rectangle : rectangles) {
      if (isRectangleInPolygon(rectangle)) {
        return rectangle.area;
      }
    }

    return 0;
  }



  bool isRectangleInPolygon(const Rectangle& rectangle) const {
    Vector intersection;

    // Check all rectangle sides for intersections with other lines inside the polygon
    for (auto [side, innerDir] : rectangle.getCheckSides()) {
      // Check for intersections of lines with the side (only check vertical lines for intersections with horizontal lines and vice versa)
      for (auto& line : (side.horizontal() ? verticalLines : horizontalLines)) {
        if (line.crossesLine(side, intersection)) {
          // Ignore intersection at the endpoints of the lines to check as these are the other sides of the rectangle
          if (!side.isEndPoint(intersection)) {
            // Now we have an intersection point. From here go in the direction into the center of the rectangle.
            // If that point is also on the same line, then this line cuts into this rectangle, which makes it an 
            // invalid rectangle
            if (line.isPointOnLine(intersection + innerDir)) {
              return false;
            }
          }
        }
      }
    }

    // No conflicting intersections found
    return true;
  }


  static int64_t area(Vector a, Vector b) {
    auto delta = (a - b).apply([](int value) { return std::abs(value) + 1;});
    return static_cast<int64_t>(delta.x) * delta.y;
  }
  


  std::vector<Vector> redTiles;
  std::vector<Line> horizontalLines;
  std::vector<Line> verticalLines;
  std::vector<Rectangle> rectangles; // sorted by area (represented by the diagonal line)
};

int main() {
  common::Time t;

  int64_t part1 = 0;
  int64_t part2 = 0;

  MovieTheater theater(task::input());
  part1 = theater.largestRectangleArea();
  part2 = theater.largestRectangleInPolygon();

  std::cout << "Part 1: " << part1 << "\n";
  std::cout << "Part 2: " << part2 << "\n";
  std::cout << t;
}