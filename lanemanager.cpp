#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include "chyron.h"

#include "lanemanager.h"

LaneManager::LaneManager(QObject *parent)
    : QObject(parent)
{
}

void LaneManager::subscribe(Chyron* chyron)
{
    const Chyron::Settings& settings = chyron->get_settings();

    LaneDataPointer data(new LaneData());
    data->owner = chyron;
    data_map[chyron] = data;

    if(!lane_map.contains(settings.entry_type))
        lane_map[settings.entry_type] = LaneList();
    lane_map[settings.entry_type].push_back(data);
}

void LaneManager::unsubscribe(Chyron* chyron)
{
    const Chyron::Settings& settings = chyron->get_settings();

    // calculate the shift amount
    LaneDataPointer data = data_map[chyron];
    int shift = 0;
    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            shift = data->lane_boundaries.width();
            break;
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
            shift = data->lane_boundaries.width();
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            shift = data->lane_boundaries.height();
            break;
    }

    lane_map[settings.entry_type].removeAll(data_map[chyron]);
    data_map.remove(chyron);

    foreach(LaneDataPointer data, lane_map[settings.entry_type])
    {
        const Chyron::Settings& data_settings = data->owner->get_settings();

        switch(data_settings.entry_type)
        {
            case AnimEntryType::SlideDownLeftTop:
            case AnimEntryType::TrainDownLeftTop:
            case AnimEntryType::SlideUpLeftBottom:
            case AnimEntryType::TrainUpLeftBottom:
                data->owner->shift_left(shift);
                break;
            case AnimEntryType::SlideDownCenterTop:
            case AnimEntryType::TrainDownCenterTop:
            case AnimEntryType::SlideUpCenterBottom:
            case AnimEntryType::TrainUpCenterBottom:
                data->owner->shift_left(shift);
                break;
            case AnimEntryType::SlideDownRightTop:
            case AnimEntryType::TrainDownRightTop:
            case AnimEntryType::SlideUpRightBottom:
            case AnimEntryType::TrainUpRightBottom:
                data->owner->shift_right(shift);
                break;
            case AnimEntryType::SlideInLeftTop:
            case AnimEntryType::TrainInLeftTop:
            case AnimEntryType::SlideInRightTop:
            case AnimEntryType::TrainInRightTop:
                data->owner->shift_up(shift);
                break;
            case AnimEntryType::SlideInLeftBottom:
            case AnimEntryType::TrainInLeftBottom:
            case AnimEntryType::SlideInRightBottom:
            case AnimEntryType::TrainInRightBottom:
                data->owner->shift_down(shift);
                break;
        }
    }
}

const QRect& LaneManager::get_base_lane_position(Chyron* chyron)
{
    LaneDataPointer data = data_map[chyron];
    calculate_base_lane_position(chyron);
    data->lane_boundaries = data->lane;
    return data->lane;
}

QRect& LaneManager::get_lane_boundaries(Chyron* chyron)
{
    LaneDataPointer data = data_map[chyron];
    return data->lane_boundaries;
}

void LaneManager::calculate_base_lane_position(Chyron* chyron)
{
    const Chyron::Settings& settings = chyron->get_settings();
    LaneDataPointer data = data_map[chyron];

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(settings.display);

    int width = r_desktop.width();
    int height = r_desktop.height();
    int left = r_desktop.left();
    int top = r_desktop.top();
    int right = r_desktop.left() + r_desktop.width();
    int bottom = r_desktop.top() + r_desktop.height();

    QRect& lane_position = data->lane;

    // calculate the default lane position for the entry/exit type,
    // and then shift it based on the positions of Chyrons in the
    // list with higher priorities

    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
            lane_position.setTopLeft(QPoint(left, top));
            lane_position.setBottomRight(QPoint(left, bottom));
            break;
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
            lane_position.setTopLeft(QPoint(width / 2, top));
            lane_position.setBottomRight(QPoint(width / 2, bottom));
            break;
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            lane_position.setTopLeft(QPoint(right, top));
            lane_position.setBottomRight(QPoint(right, bottom));
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            lane_position.setTopLeft(QPoint(left, top));
            lane_position.setBottomRight(QPoint(right, top));
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            lane_position.setTopLeft(QPoint(left, bottom));
            lane_position.setBottomRight(QPoint(right, bottom));
            break;

        // boundaries defined for these depend upon their corresponding exit type
        case AnimEntryType::FadeCenter:
        case AnimEntryType::PopCenter:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    lane_position.setTopLeft(QPoint(left, top + (height / 2)));
                    lane_position.setBottomRight(QPoint(right, top + (height / 2)));
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    lane_position.setTopLeft(QPoint(left + (width / 2), top));
                    lane_position.setBottomRight(QPoint(left + (width / 2), bottom));
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    lane_position.setTopLeft(QPoint(left + (width / 2), top + (height / 2)));
                    lane_position.setBottomRight(QPoint(left + (width / 2), top + (height / 2)));
                    break;
            }
            break;
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::PopLeftTop:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    lane_position.setTopLeft(QPoint(left, top));
                    lane_position.setBottomRight(QPoint(right, top));
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    lane_position.setTopLeft(QPoint(left, top));
                    lane_position.setBottomRight(QPoint(left, bottom));
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    lane_position.setTopLeft(QPoint(left, top));
                    lane_position.setBottomRight(QPoint(left, top));
                    break;
            }
            break;
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::PopRightTop:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    lane_position.setTopLeft(QPoint(left, top));
                    lane_position.setBottomRight(QPoint(right, top));
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    lane_position.setTopLeft(QPoint(right, top));
                    lane_position.setBottomRight(QPoint(right, bottom));
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    lane_position.setTopLeft(QPoint(right, top));
                    lane_position.setBottomRight(QPoint(right, top));
                    break;
            }
            break;
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::PopLeftBottom:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    lane_position.setTopLeft(QPoint(left, bottom));
                    lane_position.setBottomRight(QPoint(right, bottom));
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    lane_position.setTopLeft(QPoint(left, top));
                    lane_position.setBottomRight(QPoint(left, bottom));
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    lane_position.setTopLeft(QPoint(left, bottom));
                    lane_position.setBottomRight(QPoint(left, bottom));
                    break;
            }
            break;
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::PopRightBottom:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    lane_position.setTopLeft(QPoint(left, bottom));
                    lane_position.setBottomRight(QPoint(right, bottom));
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    lane_position.setTopLeft(QPoint(right, top));
                    lane_position.setBottomRight(QPoint(right, bottom));
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    lane_position.setTopLeft(QPoint(right, bottom));
                    lane_position.setBottomRight(QPoint(right, bottom));
                    break;
            }
            break;
    }

    // shift the lane based on the Chyron with the next highest
    // priority in the list

    LaneListConstIter iter = lane_map[settings.entry_type].end();
    for(iter = lane_map[settings.entry_type].begin();
        iter != lane_map[settings.entry_type].end();
        ++iter)
    {
        if((*iter)->owner == chyron)
        {
            if(iter == lane_map[settings.entry_type].begin())
                // we are top priority, no shifting required
                iter = lane_map[settings.entry_type].end();
            else
                --iter;
            break;
        }
    }

    if(iter == lane_map[settings.entry_type].end())
        return;

    // 'iter' is pointing at the next highest priority lane data; shift
    // lane position based upon it

    QRect r_higher = (*iter)->lane_boundaries;

    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
            // shift right
            lane_position.setRight(r_higher.left() + r_higher.width());
            lane_position.setLeft(lane_position.right());
            break;
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
            // for center, we should ping-pong positions
            // for now, we just shift right
            lane_position.setRight(r_higher.left() + r_higher.width());
            lane_position.setLeft(lane_position.right());
            break;
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            // shift left
            lane_position.setLeft(r_higher.right() - r_higher.width());
            lane_position.setRight(lane_position.left());
            break;

        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::SlideInLeftBottom:
            // shift right
            lane_position.setRight(r_higher.left() + r_higher.width());
            lane_position.setLeft(lane_position.right());
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::SlideInRightBottom:
            // shift left
            lane_position.setLeft(r_higher.right() - r_higher.width());
            lane_position.setRight(lane_position.left());
            break;

        case AnimEntryType::TrainInLeftTop:
        case AnimEntryType::TrainInRightTop:
            // shift down
            lane_position.setTop(r_higher.top() + r_higher.height());
            lane_position.setBottom(lane_position.top());
            break;

        case AnimEntryType::TrainInLeftBottom:
        case AnimEntryType::TrainInRightBottom:
            // shift up
            lane_position.setTop(r_higher.bottom() - r_higher.height());
            lane_position.setBottom(lane_position.top());
            break;

        // boundaries defined for these depend upon their corresponding exit type
        case AnimEntryType::FadeCenter:
        case AnimEntryType::PopCenter:
            // for center, we should ping-pong positions
            // for now, we just shift right
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    break;
            }
            break;
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::PopLeftTop:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    break;
            }
            break;
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::PopRightTop:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    break;
            }
            break;
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::PopLeftBottom:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    break;
            }
            break;
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::PopRightBottom:
            switch(settings.exit_type)
            {
                case AnimExitType::SlideLeft:
                case AnimExitType::SlideRight:
                case AnimExitType::SlideFadeLeft:
                case AnimExitType::SlideFadeRight:
                    break;
                case AnimExitType::SlideUp:
                case AnimExitType::SlideDown:
                case AnimExitType::SlideFadeUp:
                case AnimExitType::SlideFadeDown:
                    break;
                case AnimExitType::Fade:
                case AnimExitType::Pop:
                    break;
            }
            break;
    }
}
