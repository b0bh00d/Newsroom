#include <limits>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>

#include "chyron.h"
#ifdef HIGHLIGHT_LANES
#include "highlightwidget.h"
#endif

Chyron::Chyron(StoryInfoPointer story_info, LaneManagerPointer lane_manager, QObject* parent)
    : story_info(story_info),
      lane_manager(lane_manager),
#ifdef HIGHLIGHT_LANES
      highlight(nullptr),
#endif
      visible(true),
      suspended(false),
      QObject(parent)
{
    age_timer = new QTimer(this);
    age_timer->setInterval(100);
    connect(age_timer, &QTimer::timeout, this, &Chyron::slot_age_headlines);
}

Chyron::~Chyron()
{
    hide();
    age_timer->deleteLater();
}

void Chyron::display()
{
    lane_manager->subscribe(this);

#ifdef HIGHLIGHT_LANES
    highlight = new HighlightWidget();
#endif

    if(age_timer && !age_timer->isActive())
        age_timer->start();

    visible = true;
}

void Chyron::hide()
{
    if(age_timer)
        age_timer->stop();

    lane_manager->unsubscribe(this);
}

void Chyron::shelve()
{
    if(age_timer)
        age_timer->stop();

    visible = false;
    lane_manager->shelve(this);
}

void Chyron::suspend()
{
    suspended = true;
}

void Chyron::resume()
{
    suspended = false;
}

void Chyron::unsubscribed()
{
#ifdef HIGHLIGHT_LANES
    if(highlight)
        highlight->deleteLater();
#endif

    foreach(HeadlinePointer headline, headline_list)
    {
        headline->hide();
        headline.clear();
    }

    visible = false;
    suspended = false;
}

void Chyron::highlight_headline(HeadlinePointer hl, qreal opacity, int timeout)
{
    foreach(HeadlinePointer headline, headline_list)
    {
        if(headline.data() == hl.data())
        {
            if(timeout == 0)
                // no animation required
                headline->setWindowOpacity(opacity < 0.0 ? 0.0 : ((opacity > 1.0) ? 1.0 : opacity));
            else
            {
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "windowOpacity"),
                        [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(timeout);
                headline->animation->setStartValue(headline->windowOpacity());
                headline->animation->setEndValue(opacity < 0.0 ? 0.0 : ((opacity > 1.0) ? 1.0 : opacity));
                headline->animation->setEasingCurve(story_info->fading_curve);

                headline->animation->start();
            }
        }
    }
}

void Chyron::initialize_headline(HeadlinePointer headline)
{
    if(!visible)
        return;

#ifdef HIGHLIGHT_LANES
    highlight->hide();
#endif

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(story_info->primary_screen);

    const QRect& lane_position = lane_manager->get_base_lane_position(this);

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    if(story_info->headlines_pixel_width && story_info->headlines_pixel_height)
    {
        width = story_info->headlines_pixel_width;
        height = story_info->headlines_pixel_height;
    }
    else
    {
        width = (story_info->headlines_percent_width / 100.0) * r_desktop.width();
        height = (story_info->headlines_percent_height / 100.0) * r_desktop.height();
    }

    if(IS_DASHBOARD(story_info->entry_type) && story_info->dashboard_compact_mode)
    {
        headline->set_compact_mode(story_info->dashboard_compact_mode, width, height);
        width *= (story_info->dashboard_compression / 100.0);
        height *= (story_info->dashboard_compression / 100.0);
    }

    switch(story_info->entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
            y = lane_position.top() - height;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
            y = lane_position.top() - height;
            // left() is already positioned in the center
            x = lane_position.left() - (width / 2);
            break;
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
            y = lane_position.top() - height;
            x = lane_position.right() - width - story_info->margin;
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.left() - width;
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.right() + width;
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.left() - width;
            break;
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.right() + width;
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
            y = lane_position.bottom() + height;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            y = lane_position.bottom() + height;
            x = lane_position.right() - width - story_info->margin;
            break;
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
            y = lane_position.bottom() + height;
            // left() is already positioned in the center
            x = lane_position.left() - (width / 2);
            break;
        case AnimEntryType::FadeCenter:
        case AnimEntryType::PopCenter:
            // top() is already positioned in the center
            y = lane_position.top() - (height / 2);
            // left() is already positioned in the center
            x = lane_position.left() - (width / 2);
            break;
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::PopLeftTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::PopRightTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.right() - width - story_info->margin;
            break;
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::PopLeftBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::PopRightBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.right() - width - story_info->margin;
            break;

        // the base lane position returned by the Lane Manager is the
        // position of the Chyron in the case of the Dashboard
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardInLeftTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInRightTop:
            y = lane_position.top() + story_info->margin;
            x = lane_position.right() - width - story_info->margin;
            break;
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardUpLeftBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.left() + story_info->margin;
            break;
        case AnimEntryType::DashboardInRightBottom:
        case AnimEntryType::DashboardUpRightBottom:
            y = lane_position.bottom() - height - story_info->margin;
            x = lane_position.right() - width - story_info->margin;
            break;
    }

    headline->setGeometry(x, y, width, height);
    headline->initialize(story_info->headlines_always_visible, story_info->headlines_fixed_type, width, height);
    if(IS_DASHBOARD(story_info->entry_type) && headline_list.count())
        headline->bottom_window = headline_list.back().data();

    // update lane's boundaries (this updates the data in the
    // Lane Manager for lower-priority lanes to reference)

    QRect& lane_boundaries = lane_manager->get_lane_boundaries(this);

    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        if(r.x() < lane_boundaries.left())
            lane_boundaries.setLeft(r.x());
        if(r.y() < lane_boundaries.top())
            lane_boundaries.setTop(r.y());
        if((r.x() + r.width()) > lane_boundaries.right())
            lane_boundaries.setRight(r.x() + r.width());
        if((r.y() + r.height()) > lane_boundaries.bottom())
            lane_boundaries.setBottom(r.y() + r.height());
    }

    if(x < lane_boundaries.left())
        lane_boundaries.setLeft(x);
    if(y < lane_boundaries.top())
        lane_boundaries.setTop(y);
    if((x + width) > lane_boundaries.right())
        lane_boundaries.setRight(x + width);
    if((y + height) > lane_boundaries.bottom())
        lane_boundaries.setBottom(y + height);

    // there's a calculation error somewhere that is causing
    // the width/height values to exceed the correct values
    // by 1.  it manifests when Chyrons are animated by the
    // LaneManager.  until I can track that down, this puts
    // a band-aid on it.

    // TODO: Locate the 1-off calculation error.
    lane_boundaries.setWidth(qMin(width + story_info->margin, lane_boundaries.width()));
    lane_boundaries.setHeight(qMin(height + story_info->margin, lane_boundaries.height()));

#ifdef HIGHLIGHT_LANES
    highlight->setGeometry(lane_boundaries);
    highlight->show();
#endif
}

void Chyron::start_headline_entry(HeadlinePointer headline)
{
    if(!visible || suspended)
        return;

    int speed = story_info->anim_motion_duration;

    QParallelAnimationGroup* animation_group = nullptr;
    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(story_info->primary_screen);
    QRect r = headline->geometry();

    switch(story_info->entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::TrainDownRightTop:
        case AnimEntryType::TrainInLeftTop:
        case AnimEntryType::TrainInRightTop:
        case AnimEntryType::TrainInLeftBottom:
        case AnimEntryType::TrainInRightBottom:
        case AnimEntryType::TrainUpLeftBottom:
        case AnimEntryType::TrainUpRightBottom:
        case AnimEntryType::TrainUpCenterBottom:
            headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "geometry"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });
            headline->animation->setDuration(speed);
            headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEasingCurve(story_info->motion_curve);
            connect(headline->animation.data(), &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
            prop_anim_map[headline->animation.data()] = headline;

            if(headline_list.length())
            {
                animation_group = new QParallelAnimationGroup();
                foreach(HeadlinePointer posted_headline, headline_list)
                    posted_headline->animation = AnimationPointer(new QPropertyAnimation(posted_headline.data(), "geometry"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });

                if(IS_TRAIN(story_info->entry_type))
                    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_train_expire_headlines);
            }
            break;

        case AnimEntryType::PopCenter:
        case AnimEntryType::PopLeftTop:
        case AnimEntryType::PopRightTop:
        case AnimEntryType::PopLeftBottom:
        case AnimEntryType::PopRightBottom:
            // create a dummy effect for these types so slot_headline_posted()
            // is triggered
            headline->animation.clear();
            headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "geometry"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });
            headline->animation->setDuration(speed);
            headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEndValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEasingCurve(story_info->motion_curve);
            connect(headline->animation.data(), &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
            prop_anim_map[headline->animation.data()] = headline;
            entering_map[headline] = true;
            break;

        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardUpRightBottom:
            headline->animation.clear();
            headline->viewed = QDateTime::currentDateTime().toTime_t();
            headline->show();
            // slot_headline_posted() will never be hit with this, so the
            // shift_*() methods will abort because no headlines are visible,
            // so we call headline_posted() directly
            headline_posted(headline);
            return;

        case AnimEntryType::FadeCenter:
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::FadeRightBottom:
            {
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "windowOpacity"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(0.0);
                headline->animation->setEndValue(1.0);
                headline->animation->setEasingCurve(story_info->motion_curve);
                connect(headline->animation.data(), &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
                prop_anim_map[headline->animation.data()] = headline;
            }
            break;
    }

    auto configure_group_item = [](QPropertyAnimation* anim, int speed, const QRect& start, const QRect& end, QEasingCurve curve) {
        anim->setDuration(speed);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(curve);
    };

    switch(story_info->entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::TrainDownRightTop:
            headline->animation->setEndValue(QRect(r.x(), r_desktop.top() + story_info->margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() + r.height() + story_info->margin, posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
            headline->animation->setEndValue(QRect(r_desktop.left() + story_info->margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x() + r.width() + story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            headline->animation->setEndValue(QRect(r_desktop.width() - r.width() - story_info->margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x() - r.width() - story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
            headline->animation->setEndValue(QRect(r_desktop.left() + story_info->margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x() + r.width() + story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            headline->animation->setEndValue(QRect(r_desktop.width() - r.width() - story_info->margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x() - r.width() - story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpLeftBottom:
        case AnimEntryType::TrainUpRightBottom:
        case AnimEntryType::TrainUpCenterBottom:
            headline->animation->setEndValue(QRect(r.x(), r_desktop.bottom() - r.height() - story_info->margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation.data(), speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() - r.height() - story_info->margin, posted_r.width(), posted_r.height()),
                                         story_info->motion_curve);
                }
            }
            break;

        case AnimEntryType::PopCenter:
        case AnimEntryType::PopLeftTop:
        case AnimEntryType::PopRightTop:
        case AnimEntryType::PopLeftBottom:
        case AnimEntryType::PopRightBottom:
        case AnimEntryType::FadeCenter:
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardUpRightBottom:
            break;
    }

    headline->viewed = QDateTime::currentDateTime().toTime_t();
    headline->show();

    if(animation_group || headline->animation)
    {
        entering_map[headline] = true;

        if(animation_group)
        {
            animation_group->addAnimation(headline->animation.data());
            foreach(HeadlinePointer posted_headline, headline_list)
                animation_group->addAnimation(posted_headline->animation.data());
            lane_manager->anim_queue(this, animation_group);
        }
        else if(headline->animation)
            lane_manager->anim_queue(this, headline->animation.data());
    }
}

void Chyron::start_headline_exit(HeadlinePointer headline)
{
    if(!visible || suspended)
        return;

    // the time-to-display has expired

    QParallelAnimationGroup* animation_group = nullptr;

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(story_info->primary_screen);
    QRect r = headline->geometry();

    int speed = story_info->fade_target_duration;

    if(IS_TRAIN(story_info->entry_type))
    {
        if(story_info->train_use_age_effect)
        {
            if(story_info->train_age_effect == AgeEffects::ReduceOpacityFixed)
            {
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "windowOpacity"),
                                    [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(1.0);
                headline->animation->setEndValue(story_info->train_age_percent / 100.0);
                headline->animation->setEasingCurve(story_info->fading_curve);
                lane_manager->anim_queue(this, headline->animation.data());
            }
        }

        headline->ignore = true;     // ignore subsequent 'aging' activities on this one
    }
    else if(IS_DASHBOARD(story_info->entry_type))
    {
        if(story_info->dashboard_use_age_effect)
        {
            if(story_info->dashboard_age_percent)
            {
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "windowOpacity"),
                                    [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(1.0);
                headline->animation->setEndValue(story_info->dashboard_age_percent / 100.0);
                headline->animation->setEasingCurve(story_info->fading_curve);
                lane_manager->anim_queue(this, headline->animation.data());
            }
        }

        headline->ignore = true;     // ignore subsequent 'aging' activities on this one
    }
    else
    {
        switch(story_info->exit_type)
        {
            case AnimExitType::SlideLeft:
            case AnimExitType::SlideRight:
            case AnimExitType::SlideUp:
            case AnimExitType::SlideDown:
            case AnimExitType::SlideFadeLeft:
            case AnimExitType::SlideFadeRight:
            case AnimExitType::SlideFadeUp:
            case AnimExitType::SlideFadeDown:
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "geometry"),
                                    [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
                headline->animation->setEasingCurve(story_info->motion_curve);
                connect(headline->animation.data(), &QPropertyAnimation::finished, this, &Chyron::slot_headline_expired);
                prop_anim_map[headline->animation.data()] = headline;
                break;

            case AnimExitType::Fade:
                headline->animation = AnimationPointer(new QPropertyAnimation(headline.data(), "windowOpacity"),
                                    [] (QAbstractAnimation* anim) { anim->deleteLater(); });
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(1.0);
                headline->animation->setEndValue(0.0);
                headline->animation->setEasingCurve(story_info->motion_curve);
                connect(headline->animation.data(), &QPropertyAnimation::finished, this, &Chyron::slot_headline_expired);
                prop_anim_map[headline->animation.data()] = headline;
                break;

            case AnimExitType::Pop:
                headline_list.removeAll(headline);
                headline->hide();
                headline.clear();
                break;
        }

        switch(story_info->exit_type)
        {
            case AnimExitType::SlideLeft:
            case AnimExitType::SlideFadeLeft:
                headline->animation->setEndValue(QRect(r_desktop.x() - r.width(), r.y(), r.width(), r.height()));
                break;
            case AnimExitType::SlideRight:
            case AnimExitType::SlideFadeRight:
                headline->animation->setEndValue(QRect(r_desktop.x() + r_desktop.width() + r.width(), r.y(), r.width(), r.height()));
                break;
            case AnimExitType::SlideUp:
            case AnimExitType::SlideFadeUp:
                headline->animation->setEndValue(QRect(r.x(), r_desktop.y() - r.height(), r.width(), r.height()));
                break;
            case AnimExitType::SlideDown:
            case AnimExitType::SlideFadeDown:
                headline->animation->setEndValue(QRect(r.x(), r_desktop.y() + r_desktop.height() + r.height(), r.width(), r.height()));
                break;

            case AnimExitType::Fade:
            case AnimExitType::Pop:
                break;
        }

        switch(story_info->exit_type)
        {
            case AnimExitType::SlideFadeLeft:
            case AnimExitType::SlideFadeRight:
            case AnimExitType::SlideFadeUp:
            case AnimExitType::SlideFadeDown:
            {
                animation_group = new QParallelAnimationGroup(this);

                animation_group->addAnimation(headline->animation.data());

                QPropertyAnimation* opacity_animation = new QPropertyAnimation(headline.data(), "windowOpacity");
                opacity_animation->setDuration(speed);
                opacity_animation->setStartValue(1.0);
                opacity_animation->setEndValue(0.0);
                opacity_animation->setEasingCurve(story_info->motion_curve);

                animation_group->addAnimation(opacity_animation);
                break;
            }
        }

        exiting_map[headline] = true;
        headline_list.removeAll(headline);

        if(animation_group)
            lane_manager->anim_queue(this, animation_group);
        else
            lane_manager->anim_queue(this, headline->animation.data());
    }
}

QAbstractAnimation* Chyron::shift_left(int amount, bool auto_start)
{
    if(!visible || !headline_list.length())
        return nullptr; // no headlines visible

    QParallelAnimationGroup* animation_group = nullptr;
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        QPropertyAnimation* animation = new QPropertyAnimation(headline.data(), "geometry");
        animation->setDuration(story_info->anim_motion_duration);
        animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        animation->setEasingCurve(story_info->motion_curve);
        animation->setEndValue(QRect(r.x() - amount, r.y(), r.width(), r.height()));

        if(!animation_group)
            animation_group = new QParallelAnimationGroup();
        animation_group->addAnimation(animation);
    }

    if(auto_start)
    {
        connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
        animation_group->start();
        return nullptr;
    }

    return animation_group;
}

QAbstractAnimation* Chyron::shift_right(int amount, bool auto_start)
{
    if(!visible || !headline_list.length())
        return nullptr; // no headlines visible

    QParallelAnimationGroup* animation_group = nullptr;
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        QPropertyAnimation* animation = new QPropertyAnimation(headline.data(), "geometry");
        animation->setDuration(story_info->anim_motion_duration);
        animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        animation->setEasingCurve(story_info->motion_curve);
        animation->setEndValue(QRect(r.x() + amount, r.y(), r.width(), r.height()));

        if(!animation_group)
            animation_group = new QParallelAnimationGroup();
        animation_group->addAnimation(animation);
    }

    if(auto_start)
    {
        connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
        animation_group->start();
        return nullptr;
    }

    return animation_group;
}

QAbstractAnimation* Chyron::shift_up(int amount, bool auto_start)
{
    if(!visible || !headline_list.length())
        return nullptr; // no headlines visible

    QParallelAnimationGroup* animation_group = nullptr;
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        QPropertyAnimation* animation = new QPropertyAnimation(headline.data(), "geometry");
        animation->setDuration(story_info->anim_motion_duration);
        animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        animation->setEasingCurve(story_info->motion_curve);
        animation->setEndValue(QRect(r.x(), r.y() - amount, r.width(), r.height()));

        if(!animation_group)
            animation_group = new QParallelAnimationGroup();
        animation_group->addAnimation(animation);
    }

    if(auto_start)
    {
        connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
        animation_group->start();
        return nullptr;
    }

    return animation_group;
}

QAbstractAnimation* Chyron::shift_down(int amount, bool auto_start)
{
    if(!visible || !headline_list.length())
        return nullptr; // no headlines visible

    QParallelAnimationGroup* animation_group = nullptr;
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        QPropertyAnimation* animation = new QPropertyAnimation(headline.data(), "geometry");
        animation->setDuration(story_info->anim_motion_duration);
        animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        animation->setEasingCurve(story_info->motion_curve);
        animation->setEndValue(QRect(r.x(), r.y() + amount, r.width(), r.height()));

        if(!animation_group)
            animation_group = new QParallelAnimationGroup();
        animation_group->addAnimation(animation);
    }

    if(auto_start)
    {
        connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
        animation_group->start();
        return nullptr;
    }

    return animation_group;
}

void Chyron::dashboard_expire_headlines()
{
    if(suspended)
        return;

    // all headlines that are not in the front (i.e., tail of the list)
    // need to be expired

    if(headline_list.count() == 1)
        return;     // nothing to expire

    HeadlineList expired_list;
    for(int i = 0;i < (headline_list.count() - 1);++i)
        expired_list.append(headline_list[i]);

    foreach(HeadlinePointer expired, expired_list)
    {
        headline_list.removeAll(expired);
        expired->hide();
        emit signal_headline_going_out_of_scope(expired);
        expired.clear();
    }

    if(headline_list.count() == 1)
        headline_list[0]->bottom_window = nullptr;
}

void Chyron::slot_file_headline(HeadlinePointer headline)
{
    if(!visible || suspended)
        return;

    Q_ASSERT(headline->story_info->story == story_info->story);

    // for some reason, the Producer signal 'signal_new_headline' is
    // coming in here twice on the same call to QMetaObject::activate(),
    // so we have to guard against it...

    if(!incoming_headlines.contains(headline))
        incoming_headlines.enqueue(headline);
}

void Chyron::headline_posted(HeadlinePointer headline)
{
    entering_map.remove(headline);

    if(headline->animation)
        headline->animation->deleteLater();

    headline->viewed = QDateTime::currentDateTime().toTime_t();

    connect(headline.data(), &Headline::signal_mouse_enter, this, &Chyron::slot_headline_mouse_enter);
    connect(headline.data(), &Headline::signal_mouse_exit, this, &Chyron::slot_headline_mouse_exit);

    headline_list.append(headline);

    if(IS_DASHBOARD(story_info->entry_type))
        // give the new headline time to appear before removing old ones...
        QTimer::singleShot(50, this, [this] () { this->dashboard_expire_headlines(); });
}

void Chyron::slot_headline_posted()
{
    HeadlinePointer headline;
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    if(anim)
    {
        headline = prop_anim_map[anim];
        prop_anim_map.remove(anim);
    }
    else
    {
        Headline* headline_ptr = qobject_cast<Headline*>(sender());
        if(!headline_ptr)
            return;
        foreach(HeadlinePointer hp, headline_list)
        {
            if(hp.data() == headline_ptr)
            {
                headline = hp;
                break;
            }
        }
    }

    if(!headline.isNull())
        headline_posted(headline);
}

void Chyron::slot_headline_expired()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    HeadlinePointer headline = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    exiting_map.remove(headline);

    headline->animation->deleteLater();

    headline->hide();
    emit signal_headline_going_out_of_scope(headline);
    headline.clear();
}

void Chyron::slot_release_animation()
{
    QPropertyAnimation* prop_anim = qobject_cast<QPropertyAnimation*>(sender());
    if(prop_anim)
        prop_anim->deleteLater();
}

void Chyron::slot_age_headlines()
{
    if(suspended || entering_map.count() || exiting_map.count())
        return;     // let any in-progress actions complete

    uint now = QDateTime::currentDateTime().toTime_t();

    if(incoming_headlines.length())
    {
        HeadlinePointer headline = incoming_headlines.dequeue();
        headline->viewed = 0;    // let's us know when the headline was first displayed
        initialize_headline(headline);

        // inject an headline into the chyron
        start_headline_entry(headline);
    }
    else
    {
        foreach(HeadlinePointer headline, headline_list)
        {
            if(!headline->ignore)
            {
                if((now - headline->viewed) > story_info->ttl)
                {
                    start_headline_exit(headline);
                    break;
                }
            }
        }
    }
}

void Chyron::slot_train_expire_headlines()
{
    if(suspended)
        return;

    // each headline that is no longer visible on the primary
    // display will be expired

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(story_info->primary_screen);

    HeadlineList expired_list;
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        if(!r_desktop.contains(QPoint(r.x(), r.y())) &&
           !r_desktop.contains(QPoint(r.x(), r.y() + r.height())) &&
           !r_desktop.contains(r.x() + r.width(), r.y() + r.height()) &&
           !r_desktop.contains(r.x() + r.width(), r.y()))
            expired_list.append(headline);
    }

    foreach(HeadlinePointer expired, expired_list)
    {
        headline_list.removeAll(expired);
        expired->hide();
        emit signal_headline_going_out_of_scope(expired);
        expired.clear();
    }

    if(story_info->train_age_effect == AgeEffects::ReduceOpacityByAge)
    {
        // those that have aged will have their opacities reduced

        foreach(HeadlinePointer headline, headline_list)
        {
            QGraphicsEffect* eff = headline->graphicsEffect();
            if(!eff)
            {
                // add one
                eff = new QGraphicsOpacityEffect(this);
                QGraphicsOpacityEffect* opacity_eff = qobject_cast<QGraphicsOpacityEffect*>(eff);
                opacity_eff->setOpacity(1.0);
                headline->setGraphicsEffect(eff);
            }

            qreal target_opacity = 1.0;

            QRect r = headline->geometry();

            switch(story_info->entry_type)
            {
                case AnimEntryType::TrainInLeftTop:
                case AnimEntryType::TrainInLeftBottom:
                    target_opacity = 1.0 - (r.left() / (r_desktop.width() * 1.0));
                    break;
                case AnimEntryType::TrainInRightTop:
                case AnimEntryType::TrainInRightBottom:
                    target_opacity = r.left() / (r_desktop.width() * 1.0);
                    break;
                case AnimEntryType::TrainUpLeftBottom:
                case AnimEntryType::TrainUpRightBottom:
                case AnimEntryType::TrainUpCenterBottom:
                    target_opacity = 1.0 - (r.top() / (r_desktop.height() * 1.0));
                    break;
                case AnimEntryType::TrainDownLeftTop:
                case AnimEntryType::TrainDownCenterTop:
                case AnimEntryType::TrainDownRightTop:
                    target_opacity = r.top() / (r_desktop.height() * 1.0);
                    break;
            }

            QGraphicsOpacityEffect* opacity_eff = qobject_cast<QGraphicsOpacityEffect*>(eff);
            opacity_eff->setOpacity(target_opacity);
        }
    }
}

void Chyron::slot_headline_mouse_enter()
{
    Headline* headline = qobject_cast<Headline*>(sender());
    if(headline->windowOpacity() < 1.0)
    {
        opacity_map[headline] = headline->windowOpacity();

        headline->animation = AnimationPointer(new QPropertyAnimation(headline, "windowOpacity"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });
        headline->animation->setDuration(150);
        headline->animation->setStartValue(headline->windowOpacity());
        headline->animation->setEndValue(1.0);
        headline->animation->setEasingCurve(story_info->motion_curve);
        headline->animation->start();
    }
}

void Chyron::slot_headline_mouse_exit()
{
    Headline* headline = qobject_cast<Headline*>(sender());
    if(opacity_map.contains(headline))
    {
        headline->animation = AnimationPointer(new QPropertyAnimation(headline, "windowOpacity"),
                                [] (QAbstractAnimation* anim) { anim->deleteLater(); });
        headline->animation->setDuration(150);
        headline->animation->setStartValue(1.0);
        headline->animation->setEndValue(opacity_map[headline]);
        headline->animation->setEasingCurve(story_info->motion_curve);
        headline->animation->start();

        opacity_map.remove(headline);
    }
}
