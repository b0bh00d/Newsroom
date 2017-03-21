#include "chyron.h"
#include "dashboard.h"

Dashboard::Dashboard(StoryInfoPointer story_info, const QFont& headline_font, const QString& headline_stylesheet, QObject *parent)
    : QObject(parent)
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

    id = story_info->dashboard_group_id;
    HeadlineGenerator generator(w, h,
                                story_info,
                                QString("<h2><center>%1</center></h2>").arg(story_info->dashboard_group_id),
                                Qt::AlignHCenter|Qt::AlignVCenter);
    lane_header = generator.get_headline();
    lane_header->set_font(headline_font);
    lane_header->set_stylesheet(headline_stylesheet);
    lane_header->set_margin(0);

    if(story_info->dashboard_compact_mode)
    {
        lane_header->set_compact_mode(true, w, h);
        w *= (story_info->dashboard_compression / 100.0);
        h *= (story_info->dashboard_compression / 100.0);
    }

    lane_header->setGeometry(QRect(0, 0, w, h));
    lane_header->initialize(story_info->headlines_always_visible, story_info->headlines_fixed_type, w, h);
}

Dashboard::~Dashboard()
{
    // make sure we release any suspended Chyrons before we leave...
    resume_chyrons();
}

void Dashboard::resume_chyrons()
{
    foreach(LaneDataPointer lane, chyrons)
        lane->owner->resume();
}

void Dashboard::add_lane(LaneDataPointer lane)
{
    if(unsubscribe_queue.contains(lane))
        unsubscribe_queue.removeAll(lane);

    chyrons.push_back(lane);
}

bool Dashboard::is_id(const QString& id)
{
    return !this->id.compare(id);
}

bool Dashboard::is_managing(Chyron* chyron)
{
    foreach(LaneDataPointer lane, chyrons)
    {
        if(lane->owner == chyron)
            return true;
    }

    return false;
}

bool Dashboard::is_empty()
{
    return chyrons.isEmpty();
}

QRect Dashboard::get_header_geometry()
{
    return lane_header->geometry();
}

void Dashboard::get_header_geometry(QRect& r)
{
    r = lane_header->geometry();
}

void Dashboard::get_header_geometry(int& x, int& y, int& w, int& h)
{
    QRect r = lane_header->geometry();
    x = r.x();
    y = r.y();
    w = r.width();
    h = r.height();
}

QPoint Dashboard::get_header_position()
{
    QPoint p;
    QRect r = lane_header->geometry();
    p.setX(r.x());
    p.setY(r.y());

    return p;
}

QSize Dashboard::get_header_size()
{
    QSize s;
    QRect r = lane_header->geometry();
    s.setWidth(r.width());
    s.setHeight(r.height());

    return s;

}

void Dashboard::remove_lane(LaneDataPointer lane, UnsubscribeAction action)
{
    if(lane.isNull())
        return;     // shouldn't happen, but just in case...

    if(action == UnsubscribeAction::Immediate)
    {
        chyrons.removeAll(lane);
        lane->owner->unsubscribed();

        if(!chyrons.length())
            emit signal_empty(lane);
    }
    else
    {
        bool start_queue = (unsubscribe_queue.count() == 0);
        unsubscribe_queue.enqueue(lane);

        if(start_queue)
            process_unsubscribe_queue();
    }
}

void Dashboard::_remove_lane(LaneDataPointer lane)
{
    // calculate the shift amount

    int shift = 0;
    StoryInfoPointer story_info = lane->owner->get_settings();
    AnimEntryType entry_type = story_info->entry_type;

    switch(entry_type)
    {
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
            shift = lane->lane_boundaries.width();
            break;
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardUpRightBottom:
            shift = lane->lane_boundaries.height();
            break;
    }

    // move only those Chyrons that are lower in priority
    // than the one that is unsubscribing

    QParallelAnimationGroup* animation_group = nullptr;

    bool shifting = false;

    foreach(LaneDataPointer lane_data, chyrons)
    {
        lane_data->owner->suspend();

        if(lane_data->owner == lane->owner)
            shifting = true;
        if(!shifting || lane_data->owner == lane->owner)
            continue;

        StoryInfoPointer data_story_info = lane_data->owner->get_settings();

        QAbstractAnimation* anim = nullptr;
        switch(data_story_info->entry_type)
        {
            case AnimEntryType::DashboardInLeftTop:
            case AnimEntryType::DashboardInLeftBottom:
                anim = lane_data->owner->shift_left(shift, false);
                if(anim)
                {
                    if(!animation_group)
                        animation_group = new QParallelAnimationGroup();
                    animation_group->addAnimation(anim);
                }
                lane_data->lane.moveLeft(lane_data->lane.x() - shift);
                lane_data->lane_boundaries.moveLeft(lane_data->lane_boundaries.x() - shift);
                break;
            case AnimEntryType::DashboardInRightTop:
            case AnimEntryType::DashboardInRightBottom:
                anim = lane_data->owner->shift_right(shift, false);
                if(anim)
                {
                    if(!animation_group)
                        animation_group = new QParallelAnimationGroup();
                    animation_group->addAnimation(anim);
                }
                lane_data->lane.moveRight(lane_data->lane.x() + lane_data->lane.width() + shift);
                lane_data->lane_boundaries.moveRight(lane_data->lane_boundaries.x() + lane_data->lane_boundaries.width() + shift);
                break;
            case AnimEntryType::DashboardDownLeftTop:
            case AnimEntryType::DashboardDownRightTop:
                anim = lane_data->owner->shift_up(shift, false);
                if(anim)
                {
                    if(!animation_group)
                        animation_group = new QParallelAnimationGroup();
                    animation_group->addAnimation(anim);
                }
                lane_data->lane.moveBottom(lane_data->lane.y() + lane_data->lane.height() - shift);
                lane_data->lane_boundaries.moveBottom(lane_data->lane_boundaries.y() + lane_data->lane_boundaries.height() - shift);
                break;
            case AnimEntryType::DashboardUpLeftBottom:
            case AnimEntryType::DashboardUpRightBottom:
                anim = lane_data->owner->shift_down(shift, false);
                if(anim)
                {
                    if(!animation_group)
                        animation_group = new QParallelAnimationGroup();
                    animation_group->addAnimation(anim);
                }
                lane_data->lane.moveTop(lane_data->lane.y() + shift);
                lane_data->lane_boundaries.moveTop(lane_data->lane_boundaries.y() + shift);
        }
    }

    anim_clear(lane);
    anim_queue(lane, animation_group);

    chyrons.removeAll(lane);
    lane->owner->unsubscribed();
    emit signal_chyron_unsubscribed(lane);

    if(!chyrons.length())
        emit signal_empty(lane);
}

void Dashboard::shift(LaneDataPointer exiting)
{
    StoryInfoPointer data_story_info = chyrons[0]->owner->get_settings();
    AnimEntryType entry_type = data_story_info->entry_type;

    int shift = 0;
    foreach(LaneDataPointer lane_data, chyrons)
    {
        switch(entry_type)
        {
            case AnimEntryType::DashboardInLeftTop:
            case AnimEntryType::DashboardInRightTop:
                shift = exiting->lane_boundaries.height();
                lane_data->owner->shift_up(shift);
                break;
            case AnimEntryType::DashboardInLeftBottom:
            case AnimEntryType::DashboardInRightBottom:
                shift = exiting->lane_boundaries.height();
                lane_data->owner->shift_down(shift);
                break;
            case AnimEntryType::DashboardDownLeftTop:
            case AnimEntryType::DashboardUpLeftBottom:
                shift = exiting->lane_boundaries.width();
                lane_data->owner->shift_left(shift);
                break;
            case AnimEntryType::DashboardDownRightTop:
            case AnimEntryType::DashboardUpRightBottom:
                shift = exiting->lane_boundaries.width();
                lane_data->owner->shift_right(shift);
                break;
        }
    }

    // don't forget to move the dashboard header as well

    QRect r = get_header_geometry();
    QPropertyAnimation* animation = new QPropertyAnimation(lane_header.data(), "geometry");
    animation->setDuration(data_story_info->anim_motion_duration);
    animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
    animation->setEasingCurve(data_story_info->motion_curve);

    switch(entry_type)
    {
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
            animation->setEndValue(QRect(r.x(), r.y() - shift, r.width(), r.height()));
            break;
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
            animation->setEndValue(QRect(r.x(), r.y() + shift, r.width(), r.height()));
            break;
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardUpLeftBottom:
            animation->setEndValue(QRect(r.x() - shift, r.y(), r.width(), r.height()));
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardUpRightBottom:
            animation->setEndValue(QRect(r.x() + shift, r.y(), r.width(), r.height()));
            break;
    }

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Dashboard::calculate_base_lane_position(LaneDataPointer data, const QRect& r_desktop, int r_offset_w, int r_offset_h)
{
    StoryInfoPointer story_info = data->owner->get_settings();
    QRect& lane_position = data->lane;

    int left = r_desktop.left();
    int top = r_desktop.top();
    int right = r_desktop.left() + r_desktop.width();
    int bottom = r_desktop.top() + r_desktop.height();

    QSize s = get_header_size();
    int r_header_w = s.width();
    int r_header_h = s.height();
    int r_header_x = 0;
    int r_header_y = 0;

    switch(story_info->entry_type)
    {
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

        default:
            break;
    }

    switch(story_info->entry_type)
    {
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardUpLeftBottom:
            // shift header right
            r_header_x += r_offset_w;
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardUpRightBottom:
            // shift header left
            r_header_x -= r_offset_w;
            break;
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
            // shift header down
            r_header_y += r_offset_h;
            break;
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
            // shift header up
            r_header_y -= r_offset_h;
            break;

        default:
            break;
    }

    uint chyron_position = 0;   // higher == lower priority
    foreach(LaneDataPointer lane, chyrons)
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

    left = r_header_x;
    right = left + r_header_w;

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

    if(!lane_header->isVisible())
    {
        lane_header->setGeometry(QRect(r_header_x, r_header_y, r_header_w, r_header_h));
        lane_header->show();
    }
}

bool Dashboard::anim_in_progress()
{
    return anim_tracker.count() || unsubscribe_queue.count();
}

void Dashboard::anim_queue(LaneDataPointer owner, QAbstractAnimation* anim)
{
    bool start_timer = false;
    if(!anim_tracker.count())
        start_timer = true;

    anim_tracker.append(qMakePair(owner, anim));

    if(start_timer)
        QTimer::singleShot(0, this, &Dashboard::slot_process_anim_queue);
}

void Dashboard::anim_clear(LaneDataPointer owner)
{
    QVector<AnimPair> del;
    foreach(const AnimPair& p, anim_tracker)
        if(p.first.data() == owner.data())
            del.append(p);

    foreach(const AnimPair& p, del)
        anim_tracker.removeAll(p);
}

void Dashboard::slot_animation_complete()
{
    QAbstractAnimation* anim = qobject_cast<QAbstractAnimation*>(sender());
    anim->deleteLater();

    emit signal_animation_completed();

    animation_complete();
}

void Dashboard::animation_complete()
{
    resume_chyrons();

    if(anim_tracker.count())
        QTimer::singleShot(0, this, &Dashboard::slot_process_anim_queue);
    else
    {
        if(unsubscribe_queue.count())
            (void)unsubscribe_queue.dequeue();      // remove the head item; we just processed it

        if(unsubscribe_queue.count())
            process_unsubscribe_queue();
    }
}

void Dashboard::slot_process_anim_queue()
{
    if(!anim_tracker.count())
        return;

    AnimPair p = anim_tracker.front();
    anim_tracker.pop_front();

    if(p.second)
    {
        connect(p.second, &QAbstractAnimation::finished, this, &Dashboard::slot_animation_complete);
        p.second->start();
        emit signal_animation_started();
    }
    else
        animation_complete();
}

void Dashboard::process_unsubscribe_queue()
{
    if(!unsubscribe_queue.count())
        return;
    _remove_lane(unsubscribe_queue.head());
}
