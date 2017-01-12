#include "chyron.h"

#include "lanemanager.h"

LaneManager::LaneManager(const QFont& font, const QString& stylesheet, QObject *parent)
    : headline_font(font),
      headline_stylesheet(stylesheet),
      QObject(parent)
{
}

void LaneManager::subscribe(Chyron* chyron)
{
    if(data_map.contains(chyron))
        return;

    StoryInfoPointer story_info = chyron->get_settings();

    LaneDataPointer data(new LaneData());
    data->owner = chyron;
    data_map[chyron] = data;

    if(IS_DASHBOARD(story_info->entry_type))
    {
        // figure out which Dashboard group this Chyron should be
        // assigned to (or add a new one if it doesn't exist)

        if(!dashboard_map.contains(story_info->entry_type))
            dashboard_map[story_info->entry_type] = DashboardList();

        // see if this entry type already sports our group id
        DashboardPointer dashboard_group;
        foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
        {
            if(!dashboard->id.compare(story_info->dashboard_group_id))
            {
                dashboard_group = dashboard;
                break;
            }
        }

        if(dashboard_group.isNull())
        {

            int w = 0;
            int h = 0;
            story_info->get_dimensions(w, h);

            switch(story_info->entry_type)
            {
                case AnimEntryType::DashboardDownLeftTop:
                case AnimEntryType::DashboardDownRightTop:
                case AnimEntryType::DashboardUpLeftBottom:
                case AnimEntryType::DashboardUpRightBottom:
                    h *= 0.35;
                    break;
                case AnimEntryType::DashboardInLeftTop:
                case AnimEntryType::DashboardInLeftBottom:
                case AnimEntryType::DashboardInRightTop:
                case AnimEntryType::DashboardInRightBottom:
                    w *= 0.15;
                    break;
            }

            dashboard_group = DashboardPointer(new DashboardData());
            dashboard_group->id = story_info->dashboard_group_id;
            HeadlineGenerator generator(w, h,
                                        story_info,
                                        QString("<h2><center>%1</center></h2>").arg(story_info->dashboard_group_id),
                                        story_info->entry_type,
                                        Qt::AlignHCenter|Qt::AlignVCenter);
            dashboard_group->lane_header = generator.get_headline();
            dashboard_group->lane_header->set_font(headline_font);
            dashboard_group->lane_header->set_stylesheet(headline_stylesheet);
            dashboard_group->lane_header->set_margin(0);

            if(story_info->dashboard_compact_mode)
            {
                dashboard_group->lane_header->set_compact_mode(true, w, h);
                w *= (story_info->dashboard_compression / 100.0);
                h *= (story_info->dashboard_compression / 100.0);
            }

            dashboard_group->lane_header->setGeometry(QRect(0, 0, w, h));
            dashboard_group->lane_header->initialize(story_info->headlines_always_visible, story_info->headlines_fixed_type, w, h);
            dashboard_map[story_info->entry_type].push_back(dashboard_group);
        }

        dashboard_group->chyrons.push_back(data);
    }
    else
    {
        if(!lane_map.contains(story_info->entry_type))
            lane_map[story_info->entry_type] = LaneList();
        lane_map[story_info->entry_type].push_back(data);
    }
}

void LaneManager::unsubscribe(Chyron* chyron)
{
    if(!data_map.contains(chyron))
        return;

    StoryInfoPointer story_info = chyron->get_settings();

    LaneList* lane_list;

    DashboardPointer dashboard_group;
    if(IS_DASHBOARD(story_info->entry_type))
    {
        // find the dashboard group that contains this Chyron
        foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
        {
            foreach(LaneDataPointer lane, dashboard->chyrons)
            {
                if(lane->owner == chyron)
                    dashboard_group = dashboard;
            }
        }
        lane_list = &dashboard_group->chyrons;
    }
    else
        lane_list = &lane_map[story_info->entry_type];

    // calculate the shift amount
    LaneDataPointer data = data_map[chyron];
    int shift = 0;
    switch(story_info->entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
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
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardUpRightBottom:
            shift = data->lane_boundaries.height();
            break;
    }

    // move only those Chyrons that are lower in priority
    // than the one that is unsubscribing
    bool shifting = false;

    foreach(LaneDataPointer lane_data, (*lane_list))
    {
        if(lane_data->owner == chyron)
            shifting = true;
        if(!shifting || lane_data->owner == chyron)
            continue;

        StoryInfoPointer data_story_info = lane_data->owner->get_settings();

        switch(data_story_info->entry_type)
        {
            case AnimEntryType::SlideDownLeftTop:
            case AnimEntryType::TrainDownLeftTop:
            case AnimEntryType::SlideUpLeftBottom:
            case AnimEntryType::TrainUpLeftBottom:
            case AnimEntryType::SlideDownCenterTop:
            case AnimEntryType::TrainDownCenterTop:
            case AnimEntryType::SlideUpCenterBottom:
            case AnimEntryType::TrainUpCenterBottom:
            case AnimEntryType::DashboardInLeftTop:
            case AnimEntryType::DashboardInLeftBottom:
                lane_data->owner->shift_left(shift);
                lane_data->lane.moveLeft(lane_data->lane.x() - shift);
                lane_data->lane_boundaries.moveLeft(lane_data->lane_boundaries.x() - shift);
                break;
            case AnimEntryType::SlideDownRightTop:
            case AnimEntryType::TrainDownRightTop:
            case AnimEntryType::SlideUpRightBottom:
            case AnimEntryType::TrainUpRightBottom:
            case AnimEntryType::DashboardInRightTop:
            case AnimEntryType::DashboardInRightBottom:
                lane_data->owner->shift_right(shift);
                lane_data->lane.moveRight(lane_data->lane.x() + lane_data->lane.width() + shift);
                lane_data->lane_boundaries.moveRight(lane_data->lane_boundaries.x() + lane_data->lane_boundaries.width() + shift);
                break;
            case AnimEntryType::SlideInLeftTop:
            case AnimEntryType::TrainInLeftTop:
            case AnimEntryType::SlideInRightTop:
            case AnimEntryType::TrainInRightTop:
            case AnimEntryType::DashboardDownLeftTop:
            case AnimEntryType::DashboardDownRightTop:
                lane_data->owner->shift_up(shift);
                lane_data->lane.moveTop(lane_data->lane.y() - shift);
                lane_data->lane_boundaries.moveTop(lane_data->lane_boundaries.y() - shift);
                break;
            case AnimEntryType::SlideInLeftBottom:
            case AnimEntryType::TrainInLeftBottom:
            case AnimEntryType::SlideInRightBottom:
            case AnimEntryType::TrainInRightBottom:
            case AnimEntryType::DashboardUpLeftBottom:
            case AnimEntryType::DashboardUpRightBottom:
                lane_data->owner->shift_down(shift);
                lane_data->lane.moveBottom(lane_data->lane.y() + lane_data->lane.height() + shift);
                lane_data->lane_boundaries.moveBottom(lane_data->lane_boundaries.y() + lane_data->lane_boundaries.height() + shift);
        }
    }

    data_map.remove(chyron);
    lane_map[story_info->entry_type].removeAll(data);
    if(!dashboard_group.isNull())
    {
        dashboard_group->chyrons.removeAll(data);
        if(dashboard_group->chyrons.isEmpty())
        {
            // collapse all remaining dashboards of lower priority,
            // just like we did for individual Chyrons above

            bool shifting = false;

            foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
            {
                if(dashboard_group == dashboard)
                    shifting = true;
                if(!shifting || dashboard_group == dashboard)
                    continue;

                StoryInfoPointer data_story_info = dashboard->chyrons[0]->owner->get_settings();
                AnimEntryType entry_type = data_story_info->entry_type;

                foreach(LaneDataPointer lane_data, dashboard->chyrons)
                {
                    switch(entry_type)
                    {
                        case AnimEntryType::DashboardInLeftTop:
                        case AnimEntryType::DashboardInRightTop:
                            shift = data->lane_boundaries.height();
                            lane_data->owner->shift_up(shift);
                            break;
                        case AnimEntryType::DashboardInLeftBottom:
                        case AnimEntryType::DashboardInRightBottom:
                            shift = data->lane_boundaries.height();
                            lane_data->owner->shift_down(shift);
                            break;
                        case AnimEntryType::DashboardDownLeftTop:
                        case AnimEntryType::DashboardUpLeftBottom:
                            shift = data->lane_boundaries.width();
                            lane_data->owner->shift_left(shift);
                            break;
                        case AnimEntryType::DashboardDownRightTop:
                        case AnimEntryType::DashboardUpRightBottom:
                            shift = data->lane_boundaries.width();
                            lane_data->owner->shift_right(shift);
                            break;
                    }
                }

                // don't forget to move the dashboard header as well

                QRect r = dashboard->lane_header->geometry();
                dashboard->lane_header->animation = new QPropertyAnimation(dashboard->lane_header.data(), "geometry");
                dashboard->lane_header->animation->setDuration(data_story_info->anim_motion_duration);
                dashboard->lane_header->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
                dashboard->lane_header->animation->setEasingCurve(data_story_info->motion_curve);

                switch(entry_type)
                {
                    case AnimEntryType::DashboardInLeftTop:
                    case AnimEntryType::DashboardInRightTop:
                        dashboard->lane_header->animation->setEndValue(QRect(r.x(), r.y() - shift, r.width(), r.height()));
                        break;
                    case AnimEntryType::DashboardInLeftBottom:
                    case AnimEntryType::DashboardInRightBottom:
                        dashboard->lane_header->animation->setEndValue(QRect(r.x(), r.y() + shift, r.width(), r.height()));
                        break;
                    case AnimEntryType::DashboardDownLeftTop:
                    case AnimEntryType::DashboardUpLeftBottom:
                        dashboard->lane_header->animation->setEndValue(QRect(r.x() - shift, r.y(), r.width(), r.height()));
                        break;
                    case AnimEntryType::DashboardDownRightTop:
                    case AnimEntryType::DashboardUpRightBottom:
                        dashboard->lane_header->animation->setEndValue(QRect(r.x() + shift, r.y(), r.width(), r.height()));
                        break;
                }

                dashboard->lane_header->animation->start(QAbstractAnimation::DeleteWhenStopped);
            }

            dashboard_map[story_info->entry_type].removeAll(dashboard_group);
        }
    }
}

const QRect& LaneManager::get_base_lane_position(Chyron* chyron)
{
    LaneDataPointer data = data_map[chyron];
    calculate_base_lane_position(data);
    data->lane_boundaries = data->lane;
    return data->lane;
}

QRect& LaneManager::get_lane_boundaries(Chyron* chyron)
{
    LaneDataPointer data = data_map[chyron];
    return data->lane_boundaries;
}

void LaneManager::calculate_base_lane_position(LaneDataPointer data)
{
    StoryInfoPointer story_info = data->owner->get_settings();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(story_info->primary_screen);

    int width = r_desktop.width();
    int height = r_desktop.height();
    int left = r_desktop.left();
    int top = r_desktop.top();
    int right = r_desktop.left() + r_desktop.width();
    int bottom = r_desktop.top() + r_desktop.height();

    DashboardPointer dashboard_group;
    int r_lane_x = left, r_lane_y = top;    // keeps track of where the next Dashboard should be placed
    int r_header_x, r_header_y, r_header_w, r_header_h;
    uint dashboard_position = 0;            // higher == lower priority
    if(IS_DASHBOARD(story_info->entry_type))
    {
        foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
        {
            if(!dashboard->id.compare(story_info->dashboard_group_id))
            {
                foreach(LaneDataPointer lane, dashboard->chyrons)
                {
                    if(lane->owner == data->owner)
                    {
                        dashboard_group = dashboard;
                        QRect r = dashboard_group->lane_header->geometry();
                        r_header_w = r.width();
                        r_header_h = r.height();
                        break;
                    }
                }
            }

            if(!dashboard_group.isNull())
                break;

            // track the next Dashboard position
            QRect r = dashboard->lane_header->geometry();
            r_lane_x += r.width() + story_info->margin;
            r_lane_y += r.height() + story_info->margin;

            ++dashboard_position;
        }
    }

    QRect& lane_position = data->lane;

    // calculate the default lane position for the entry/exit type,
    // and then shift it based on the positions of Chyrons in the
    // list with higher priorities

    switch(story_info->entry_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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

            // don't bake in the margin
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardInLeftTop:
            r_header_x = left;
            r_header_y = top;
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInRightTop:
            r_header_x = right - r_header_w;
            r_header_y = top;
            break;
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardInLeftBottom:
            r_header_x = left;
            r_header_y = bottom - r_header_h;
            break;
        case AnimEntryType::DashboardUpRightBottom:
        case AnimEntryType::DashboardInRightBottom:
            r_header_x = right - r_header_w;
            r_header_y = bottom - r_header_h;
            break;
    }

    // shift the lane based on the Chyron with the next highest
    // priority in the list

    LaneListConstIter iter;
    QRect r_higher;

    if(dashboard_group.isNull())
    {
        iter = lane_map[story_info->entry_type].end();
        for(iter = lane_map[story_info->entry_type].begin();
            iter != lane_map[story_info->entry_type].end();
            ++iter)
        {
            if((*iter)->owner == data->owner)
            {
                if(iter == lane_map[story_info->entry_type].begin())
                    // we are top priority, no shifting required
                    iter = lane_map[story_info->entry_type].end();
                else
                    --iter;
                break;
            }
        }

        if(iter == lane_map[story_info->entry_type].end())
            return;

        // 'iter' is pointing at the next highest priority lane data; shift
        // lane position based upon it

        r_higher = (*iter)->lane_boundaries;
    }

    switch(story_info->entry_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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
            switch(story_info->exit_type)
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

        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardUpLeftBottom:
            // shift header right
            r_header_x += r_lane_x;
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardUpRightBottom:
            // shift header left
            r_header_x -= r_lane_x;
            break;
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
            // shift header down
            r_header_y += r_lane_y;
            break;
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
            // shift header up
            r_header_y -= r_lane_y;
            break;
    }

    if(IS_DASHBOARD(story_info->entry_type))
    {
        uint chyron_position = 0;   // higher == lower priority
        foreach(LaneDataPointer lane, dashboard_group->chyrons)
        {
            if(lane->owner == data->owner)
                break;
            ++chyron_position;
        }

        int headline_w, headline_h;
        story_info->get_dimensions(headline_w, headline_h);

        if(story_info->dashboard_compact_mode)
        {
            headline_w *= (story_info->dashboard_compression / 100.0);
            headline_h *= (story_info->dashboard_compression / 100.0);
        }

        int left = r_header_x;
        int right = left + r_header_w;
        int top, bottom;
        int i;

        switch(story_info->entry_type)
        {
            case AnimEntryType::DashboardDownLeftTop:
                top = r_header_y;
                bottom = top + r_header_h;
                i = bottom + story_info->margin + (chyron_position * (headline_h + story_info->margin));
                lane_position.setTopLeft(QPoint(left, i));
                lane_position.setBottomRight(QPoint(left + r_header_w, i + r_header_h));
                r_header_x += story_info->margin;
                r_header_y += story_info->margin;
                break;
            case AnimEntryType::DashboardDownRightTop:
                top = r_header_y;
                bottom = top + r_header_h;
                i = bottom + story_info->margin + (chyron_position * (headline_h + story_info->margin));
                lane_position.setTopLeft(QPoint(right - r_header_w, i));
                lane_position.setBottomRight(QPoint(right, i + r_header_h));
                r_header_x -= story_info->margin;
                r_header_y += story_info->margin;
                break;
            case AnimEntryType::DashboardUpLeftBottom:
                top = r_header_y;
                i = top - story_info->margin - (chyron_position * (headline_h + story_info->margin));
                lane_position.setTopLeft(QPoint(left, i));
                lane_position.setBottomRight(QPoint(left + r_header_w, i));
                r_header_x += story_info->margin;
                r_header_y -= story_info->margin;
                break;
            case AnimEntryType::DashboardUpRightBottom:
                top = r_header_y;
                i = top - story_info->margin - (chyron_position * (headline_h + story_info->margin));
                lane_position.setTopLeft(QPoint(right - r_header_w, i));
                lane_position.setBottomRight(QPoint(right, i));
                r_header_x -= story_info->margin;
                r_header_y -= story_info->margin;
                break;

            case AnimEntryType::DashboardInLeftTop:
                left = r_header_x;
                right = left + r_header_w;
                i = right + story_info->margin + (chyron_position * (headline_w + story_info->margin));
                lane_position.setTopLeft(QPoint(i, r_header_y));
                lane_position.setBottomRight(QPoint(i + r_header_w, r_header_y + r_header_h));
                r_header_x += story_info->margin;
                r_header_y += story_info->margin;
                break;

            case AnimEntryType::DashboardInLeftBottom:
                left = r_header_x;
                right = left + r_header_w;
                i = right + story_info->margin + (chyron_position * (headline_w + story_info->margin));
                lane_position.setTopLeft(QPoint(i, r_header_y));
                lane_position.setBottomRight(QPoint(i + r_header_w, r_header_y + r_header_h));
                r_header_x += story_info->margin;
                r_header_y -= story_info->margin;
                break;

            case AnimEntryType::DashboardInRightTop:
                left = r_header_x - r_header_w;
                i = left - story_info->margin - (chyron_position * (headline_w + story_info->margin));
                lane_position.setTopLeft(QPoint(i, r_header_y));
                lane_position.setBottomRight(QPoint(i + r_header_w, r_header_y + r_header_h));
                r_header_x -= story_info->margin;
                r_header_y += story_info->margin;
                break;

            case AnimEntryType::DashboardInRightBottom:
                left = r_header_x - r_header_w;
                i = left - story_info->margin - (chyron_position * (headline_w + story_info->margin));
                lane_position.setTopLeft(QPoint(i, r_header_y));
                lane_position.setBottomRight(QPoint(i + r_header_w, r_header_y + r_header_h));
                r_header_x -= story_info->margin;
                r_header_y -= story_info->margin;
                break;
        }

        if(!dashboard_group->lane_header->isVisible())
        {
            dashboard_group->lane_header->setGeometry(QRect(r_header_x, r_header_y, r_header_w, r_header_h));
            dashboard_group->lane_header->show();
        }
    }
}
