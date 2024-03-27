#include "geometry.h"

namespace geometry_local {
PointF Line::GetIntersect(const Line& line) const {
    PointF intersection;
    Real denominator = (start.x - end.x) * (line.start.y - line.end.y) - (start.y - end.y) * (line.start.x - line.end.x);

    if (denominator == 0) {
        intersection.has_valid = false;
        intersection.x = intersection.y = 0;
    } else {
        intersection.x = ((start.x * end.y - start.y * end.x) * (line.start.x - line.end.x) -
                          (start.x - end.x) * (line.start.x * line.end.y - line.start.y * line.end.x)) /
                         denominator;
        intersection.y = ((start.x * end.y - start.y * end.x) * (line.start.y - line.end.y) -
                          (start.y - end.y) * (line.start.x * line.end.y - line.start.y * line.end.x)) /
                         denominator;

        if (!isPointOnSegment(intersection, start, end) || !isPointOnSegment(intersection, line.start, line.end)) {
            intersection.has_valid = false;
            intersection.x = intersection.y = 0;
        }
    }

    return intersection;
}
bool Line::isBetween(double a, double b, double c) const { 
    if(a>c)
        std::swap(a, c);
    Real kof_inac = 0.001;
    return (a - kof_inac <= b && b <= c + kof_inac); 
}
bool Line::isPointOnSegment(const PointF p, const PointF start, const PointF end) const {
    return isBetween(start.x, p.x, end.x) && isBetween(start.y, p.y, end.y);
}
bool Box::CheckContains(const PointF& point) const {
    Real kof_inac = 0.001;
    return point.x >= left_down.x - kof_inac && point.x <= right_up.x + kof_inac && point.y >= left_down.y - kof_inac  && point.y <= right_up.y + kof_inac;
}
void Box::FillIntersects(ListPoints& list, const Line& line) const {
    PointF right_down = {right_up.x, left_down.y}, left_up = {left_down.x, right_up.y};

    std::vector<Line> sides = {{right_down, right_up}, {right_down, left_down}, {left_down, left_up}, {left_up, right_up}};

    for (auto& side : sides) {
        auto point = line.GetIntersect(side);
        if (point.has_valid) list.push_back(point);
    }
}
void SortLinePoints(ListPoints& points, Direction direction) {
    static auto СomparePoints = [](const PointF& p1, const PointF& p2, Direction direction) {
        switch (direction) {
            case Direction::Down:
                return p1.y > p2.y;
            case Direction::Up:
                return p1.y < p2.y;
            case Direction::Right:
                return p1.x < p2.x;
            case Direction::Left:
                return p1.x > p2.x;
            default:
                return false;  // Для неверного направления возвращаем false
        }
    };

    std::sort(points.begin(), points.end(), [direction](const PointF& p1, const PointF& p2) { return СomparePoints(p1, p2, direction); });
}
}  // namespace geometry_local