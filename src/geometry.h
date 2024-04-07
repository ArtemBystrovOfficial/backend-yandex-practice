#pragma once

#include <vector>

using Real = double;

struct PointF {
    Real x, y;
    bool has_valid = true;
    auto operator<=>(const PointF&) const = default;
};

namespace geometry_local {

using ListPoints = std::vector<PointF>;

struct Line {
    PointF GetIntersect(const Line& line) const;

    PointF start, end;

   private:
    bool isBetween(double a, double b, double c) const;

    bool isPointOnSegment(const PointF p, const PointF start, const PointF end) const;
};

struct Box {
    bool CheckContains(const PointF& point) const;

    void FillIntersects(ListPoints& list, const Line& line) const;

    PointF left_down, right_up;
};

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

void SortLinePoints(ListPoints& points, Direction direction);

}  // namespace geometry_local