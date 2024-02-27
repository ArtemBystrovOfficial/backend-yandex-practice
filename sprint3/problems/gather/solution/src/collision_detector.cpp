#include "collision_detector.h"
#include <cassert>
#include <algorithm>
#include <cmath>

namespace collision_detector {
namespace {
bool IsHorizontal(const Gatherer & gatherer) {
    return gatherer.start_pos.y == gatherer.end_pos.y;
} 

geom::Rect MakeRectFromGatherer(Gatherer gatherer) {
    if(gatherer.start_pos.x > gatherer.end_pos.x || gatherer.start_pos.y > gatherer.end_pos.y )
        std::swap(gatherer.start_pos, gatherer.end_pos);
    if(IsHorizontal(gatherer))
        return {{gatherer.start_pos.x,gatherer.start_pos.y-gatherer.width},{gatherer.end_pos.x,gatherer.end_pos.y+gatherer.width}};
    else 
        return {{gatherer.start_pos.x-gatherer.width,gatherer.start_pos.y},{gatherer.end_pos.x + gatherer.width,gatherer.end_pos.y}};
}

bool CheckRectInherits(const geom::Point2D & point, const Gatherer & gatherer){
    auto rect = MakeRectFromGatherer(gatherer);
    return point.x >= rect.left_down.x&&
           point.x <= rect.right_up.x&&
           point.y >= rect.left_down.y &&
           point.y <= rect.right_up.y;
};

bool CheckRectInherits(const geom::Point2D & point, const geom::Rect & rect){
    return point.x >= rect.left_down.x&&
           point.x <= rect.right_up.x&&
           point.y >= rect.left_down.y &&
           point.y <= rect.right_up.y;
};

std::vector<geom::Point2D> CheckCollision(const Gatherer & gatherer, const Item & item ) {
    std::vector<geom::Point2D> points;

    if(auto point = geom::Point2D{ item.position.x + item.width, item.position.y}; CheckRectInherits(point, gatherer))
        points.push_back(point);
    if(auto point = geom::Point2D{ item.position.x - item.width, item.position.y}; CheckRectInherits(point, gatherer))
        points.push_back(point);
    if(auto point = geom::Point2D{ item.position.x, item.position.y + item.width}; CheckRectInherits(point, gatherer))
        points.push_back(point);
    if(auto point = geom::Point2D{ item.position.x, item.position.y - item.width}; CheckRectInherits(point, gatherer))
        points.push_back(point);

    return points;
}

geom::Point2D GetProjectionOnGatherer(const Gatherer & gatherer, const geom::Point2D & point){
    if(IsHorizontal(gatherer)) {
        return {point.x,gatherer.start_pos.y};
    } else {
        return {gatherer.start_pos.x,point.y};
    }
}

double GetRange(const Gatherer & gatherer, const geom::Point2D & point1, const geom::Point2D & point2) {
    return (IsHorizontal(gatherer)) ? fabs(point1.y - point2.y) : fabs(point1.x - point2.x);

}

bool IsItemInherits(const Gatherer & gatherer, const Item & item) {
    auto center_point = GetProjectionOnGatherer(gatherer, item.position);

    double min_x = std::min(gatherer.start_pos.x, gatherer.end_pos.x);
    double max_x = std::max(gatherer.start_pos.x, gatherer.end_pos.x);
    double min_y = std::min(gatherer.start_pos.y, gatherer.end_pos.y);
    double max_y = std::max(gatherer.start_pos.y, gatherer.end_pos.y);

    if(!(min_x <= center_point.x && center_point.x <= max_x && min_y <= center_point.y && center_point.y <= max_y))
        return false;

    return CheckRectInherits(center_point, geom::Rect{{item.position.x-item.width,item.position.y-item.width},
                                                      {item.position.x+item.width,item.position.y+item.width}});
}

double GetTime(const Gatherer & gatherer,const geom::Point2D & point) {
    if(IsHorizontal(gatherer)) {
        return fabs(point.x - gatherer.start_pos.x)/abs(gatherer.start_pos.x-gatherer.end_pos.x);
    } else {
        return fabs(point.y - gatherer.start_pos.y)/abs(gatherer.start_pos.y-gatherer.end_pos.y);
    }
}

std::pair<double, double> GetRangeAndTime(const Gatherer & gatherer, const Item & item) {

    if(IsItemInherits(gatherer, item)) {
        double range = GetRange(gatherer, item.position, GetProjectionOnGatherer(gatherer,item.position));
        return {range, GetTime(gatherer, item.position)};
    }

    auto points = CheckCollision(gatherer,item);

    if(points.empty())
        return {-1.0,-1.0};
    double minimal_range = std::numeric_limits<double>::max();
    double current_time = 0.0;
    for(auto point : points) {
        double range = GetRange(gatherer,point,GetProjectionOnGatherer(gatherer, point));
        if( minimal_range > range) {
            minimal_range = range;
            current_time = GetTime(gatherer, point);
        }
    }
    return {minimal_range+item.width, current_time};
}

}

std::vector<GatheringEvent> FindGatherEvents( const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;
    for(uint32_t i=0;i < provider.GatherersCount();i++) {
        for(uint32_t j=0;j < provider.ItemsCount();j++) {
            auto gatherer = provider.GetGatherer(i);
            auto item = provider.GetItem(j);
            if((gatherer.start_pos.x == gatherer.end_pos.x && gatherer.start_pos.y == gatherer.end_pos.y) || !gatherer.width || !item.width)
                continue;
            auto [range, time] = GetRangeAndTime(gatherer, item);
            if(range < 0 || time < 0)
                continue;
            events.push_back(GatheringEvent{j,i,pow(range,2),time});
        }
    }
    std::sort(events.begin(),events.end(),[](const GatheringEvent & event1,const GatheringEvent & event2){
        return event1.time < event2.time;
    });
    return events;
}

}  // namespace collision_detector