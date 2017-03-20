#include "chyron.h"

#include "lanemanager.h"

LaneManager::LaneManager(const QFont& font, const QString& stylesheet, QObject *parent)
    : headline_font(font),
      headline_stylesheet(stylesheet),
      animation_count(0),
      QObject(parent)
{
}

LaneManager::~LaneManager()
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
            if(dashboard->is_id(story_info->dashboard_group_id))
            {
                dashboard_group = dashboard;
                break;
            }
        }

        if(dashboard_group.isNull())
        {
            dashboard_group = DashboardPointer(new Dashboard(story_info, headline_font, headline_stylesheet));
            connect(dashboard_group.data(), &Dashboard::signal_chyron_unsubscribed, this, &LaneManager::slot_dashboard_chyron_unsubscribed);
            connect(dashboard_group.data(), &Dashboard::signal_empty, this, &LaneManager::slot_dashboard_empty);
            connect(dashboard_group.data(), &Dashboard::signal_animation_started, this, &LaneManager::slot_animation_started);
            connect(dashboard_group.data(), &Dashboard::signal_animation_completed, this, &LaneManager::slot_animation_completed);

            dashboard_map[story_info->entry_type].push_back(dashboard_group);
        }

        dashboard_group->add_lane(data);
    }
    else
    {
        if(!lane_map.contains(story_info->entry_type))
            lane_map[story_info->entry_type] = LaneList();
        lane_map[story_info->entry_type].push_back(data);
    }
}

void LaneManager::shelve(Chyron* chyron)
{
    unsubscribe(chyron, UnsubscribeAction::Graceful);
}

void LaneManager::unsubscribe(Chyron* chyron, UnsubscribeAction action)
{
    if(!data_map.contains(chyron))
    {
        chyron->unsubscribed();
        return;
    }

    StoryInfoPointer story_info = chyron->get_settings();
    LaneDataPointer data = data_map[chyron];
    int shift = 0;

    if(IS_DASHBOARD(story_info->entry_type))
    {
        DashboardList::iterator iter;
        for(iter = dashboard_map[story_info->entry_type].begin();iter != dashboard_map[story_info->entry_type].end();++iter)
        {
            if((*iter)->is_managing(chyron))
                break;
        }

        if(iter == dashboard_map[story_info->entry_type].end())
        {
            // hmm... really shouldn't happen
            data_map.remove(chyron);
            lane_map[story_info->entry_type].removeAll(data);
            chyron->unsubscribed();
            return;
        }

        // if ExitAction is 'Graceful', the Dashboard will emit
        // "signal_unsubscribe_chyron" when any necessary animations
        // are complete
        (*iter)->remove_lane(data, action);

        return;
    }

    LaneList* lane_list = &lane_map[story_info->entry_type];

    // calculate the shift amount
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
                lane_data->lane.moveBottom(lane_data->lane.y() + lane_data->lane.height() - shift);
                lane_data->lane_boundaries.moveBottom(lane_data->lane_boundaries.y() + lane_data->lane_boundaries.height() - shift);
                break;
            case AnimEntryType::SlideInLeftBottom:
            case AnimEntryType::TrainInLeftBottom:
            case AnimEntryType::SlideInRightBottom:
            case AnimEntryType::TrainInRightBottom:
            case AnimEntryType::DashboardUpLeftBottom:
            case AnimEntryType::DashboardUpRightBottom:
                lane_data->owner->shift_down(shift);
                lane_data->lane.moveTop(lane_data->lane.y() + shift);
                lane_data->lane_boundaries.moveTop(lane_data->lane_boundaries.y() + shift);
        }
    }

    data_map.remove(chyron);
    lane_map[story_info->entry_type].removeAll(data);

    chyron->unsubscribed();
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

    if(IS_DASHBOARD(story_info->entry_type))
    {
        int r_offset_w = 0, r_offset_h = 0; // keeps track of where the next Dashboard should be placed
        foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
        {
            if(dashboard->is_id(story_info->dashboard_group_id) && dashboard->is_managing(data->owner))
            {
                dashboard->calculate_base_lane_position(data, r_desktop, r_offset_w, r_offset_h);
                break;
            }

            // track the next Dashboard position
            QSize s = dashboard->get_header_size();
            r_offset_w += s.width() + story_info->margin;
            r_offset_h += s.height() + story_info->margin;
        }

        return;
    }

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

        default:
            break;
    }

    // shift the lane based on the Chyron with the next highest
    // priority in the list

    LaneListConstIter iter;
    QRect r_higher;

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

        default:
            break;
    }
}

void LaneManager::anim_queue(Chyron* chyron, QAbstractAnimation* anim)
{
    StoryInfoPointer story_info = chyron->get_settings();

    if(IS_DASHBOARD(story_info->entry_type))
    {
        DashboardList::iterator iter;
        for(iter = dashboard_map[story_info->entry_type].begin();iter != dashboard_map[story_info->entry_type].end();++iter)
        {
            if((*iter)->is_managing(chyron))
                break;
        }

        if(iter == dashboard_map[story_info->entry_type].end())
            return;         // shouldn't happen...

        (*iter)->anim_queue(data_map[chyron], anim);
    }
    else
        anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void LaneManager::anim_clear(Chyron *chyron)
{
    StoryInfoPointer story_info = chyron->get_settings();
    if(!IS_DASHBOARD(story_info->entry_type))
        return;

    DashboardList::iterator iter;
    for(iter = dashboard_map[story_info->entry_type].begin();iter != dashboard_map[story_info->entry_type].end();++iter)
    {
        if((*iter)->is_managing(chyron))
            break;
    }

    if(iter == dashboard_map[story_info->entry_type].end())
        return;         // shouldn't happen...

    (*iter)->anim_clear(data_map[chyron]);
}

bool LaneManager::anim_in_progress()
{
    return animation_count != 0;
}

void LaneManager::slot_dashboard_chyron_unsubscribed(LaneDataPointer lane)
{
    StoryInfoPointer story_info = lane->owner->get_settings();
    data_map.remove(lane->owner);
    lane_map[story_info->entry_type].removeAll(lane);
}

void LaneManager::slot_dashboard_empty(LaneDataPointer lane)
{
    StoryInfoPointer story_info = lane->owner->get_settings();
    if(!IS_DASHBOARD(story_info->entry_type))
        return;

    Dashboard *dashboard_ptr = qobject_cast<Dashboard *>(sender());

    DashboardList::iterator iter;
    for(iter = dashboard_map[story_info->entry_type].begin();iter != dashboard_map[story_info->entry_type].end();++iter)
    {
        if((*iter).data() == dashboard_ptr)
            break;
    }

    if(iter == dashboard_map[story_info->entry_type].end())
        return;         // shouldn't happen...

    DashboardPointer dashboard_group = (*iter);

    // collapse all remaining dashboards of lower priority,
    // just like we do for individual Chyrons

    bool shifting = false;

    foreach(DashboardPointer dashboard, dashboard_map[story_info->entry_type])
    {
        if(dashboard_group == dashboard)
            shifting = true;
        if(!shifting || dashboard_group == dashboard)
            continue;

        dashboard->shift(lane);
    }

    dashboard_map[story_info->entry_type].removeAll(dashboard_group);
    dashboard_group.clear();
}

void LaneManager::slot_animation_started()
{
    ++animation_count;
}

void LaneManager::slot_animation_completed()
{
    --animation_count;
}
