#include <QtWidgets/QDesktopWidget>

#include "chyron.h"

#include "lanemanager.h"

LaneManager::LaneManager(QObject *parent)
    : QObject(parent)
{
}

void LaneManager::subscribe(Chyron* chyron)
{
    AnimEntryType entry_type = chyron->get_entry_type();
    AnimExitType exit_type = chyron->get_exit_type();

    LaneDataPointer data(new LaneData());
    data->owner = chyron;

    if(!lane_map.contains(entry_type))
        lane_map[entry_type] = LaneList();
    lane_map[entry_type].push_back(data);

    data_map[chyron] = data;
}

void LaneManager::unsubscribe(Chyron* chyron)
{
    AnimEntryType entry_type = chyron->get_entry_type();
    lane_map[entry_type].removeAll(chyron);
}

QRect& LaneManager::get_lane_position(Chyron* chyron)
{
    LaneDataPointer data = data_map[chyron];
    calculate_lane_position(data);
    data->lane_boundaries = data->lane;
    return data->lane_boundaries;
}

void LaneManager::calculate_lane_position(LaneDataPointer data)
{
    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(data->owner->get_display());

    AnimEntryType entry_type = data->owner->get_entry_type();
    AnimExitType exit_type = data->owner->get_exit_type();

    int width = r_desktop.width();
    int height = r_desktop.height();
    int left = r_desktop.left();
    int top = r_desktop.top();
    int right = r_desktop.left() + r_desktop.width();
    int bottom = r_desktop.top() + r_desktop.height();

    QRect& lane_position = data->lane;

    switch(entry_type)
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
            switch(exit_type)
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
            switch(exit_type)
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
            switch(exit_type)
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
            switch(exit_type)
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
            switch(exit_type)
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
}
