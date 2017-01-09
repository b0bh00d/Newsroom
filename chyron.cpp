#include <limits>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGraphicsOpacityEffect>

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
    age_timer->start();

    lane_manager->subscribe(this);

#ifdef HIGHLIGHT_LANES
    highlight = new HighlightWidget();
#endif
}

void Chyron::hide()
{
#ifdef HIGHLIGHT_LANES
    if(highlight)
        highlight->deleteLater();
#endif

    lane_manager->unsubscribe(this);

    foreach(HeadlinePointer headline, headline_list)
    {
        headline->hide();
        headline.clear();
    }
}

void Chyron::initialize_headline(HeadlinePointer headline)
{
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

#ifdef HIGHLIGHT_LANES
    highlight->setGeometry(lane_boundaries);
    highlight->show();
#endif
}

void Chyron::start_headline_entry(HeadlinePointer headline)
{
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
            headline->animation = new QPropertyAnimation(headline.data(), "geometry");
            headline->animation->setDuration(speed);
            headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
            prop_anim_map[headline->animation] = headline;

            if(headline_list.length())
            {
                animation_group = new QParallelAnimationGroup();
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    posted_headline->animation = new QPropertyAnimation(posted_headline.data(), "geometry");
                    connect(posted_headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);
                }

                if(IS_TRAIN(story_info->entry_type))
                    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_train_expire_headlines);
            }
            break;

        case AnimEntryType::PopCenter:
        case AnimEntryType::PopLeftTop:
        case AnimEntryType::PopRightTop:
        case AnimEntryType::PopLeftBottom:
        case AnimEntryType::PopRightBottom:
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInLeftTop:
        case AnimEntryType::DashboardInRightTop:
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardInRightBottom:
        case AnimEntryType::DashboardUpLeftBottom:
        case AnimEntryType::DashboardUpRightBottom:
            // create a dummy effect for these types so slot_headline_posted()
            // is triggered
            if(headline->animation)
                headline->animation->deleteLater();
            headline->animation = new QPropertyAnimation(headline.data(), "geometry");
            headline->animation->setDuration(speed);
            headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEndValue(QRect(r.x(), r.y(), r.width(), r.height()));
            headline->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
            prop_anim_map[headline->animation] = headline;
            entering_map[headline] = true;
            headline->viewed = QDateTime::currentDateTime().toTime_t();
            headline->show();
            break;

        case AnimEntryType::FadeCenter:
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::FadeRightBottom:
            {
                headline->setAttribute(Qt::WA_TranslucentBackground, true);

                QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                eff->setOpacity(0.0);
                headline->setGraphicsEffect(eff);
                headline->animation = new QPropertyAnimation(eff, "opacity");
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(0.0);
                headline->animation->setEndValue(1.0);
                headline->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_headline_posted);
                prop_anim_map[headline->animation] = headline;
            }
            break;
    }

    auto configure_group_item = [](QPropertyAnimation* anim, int speed, const QRect& start, const QRect& end) {
        anim->setDuration(speed);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(QEasingCurve::InCubic);
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() + r.height() + story_info->margin, posted_r.width(), posted_r.height()));
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()));
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()));
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()));
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - story_info->margin, posted_r.y(), posted_r.width(), posted_r.height()));
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
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() - r.height() - story_info->margin, posted_r.width(), posted_r.height()));
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

    entering_map[headline] = true;
    headline->viewed = QDateTime::currentDateTime().toTime_t();
    headline->show();

    if(animation_group)
    {
        animation_group->addAnimation(headline->animation);
        foreach(HeadlinePointer posted_headline, headline_list)
            animation_group->addAnimation(posted_headline->animation);
       animation_group->start();
    }
    else if(headline->animation)
        headline->animation->start();
}

//
void Chyron::start_headline_exit(HeadlinePointer headline)
{
    // the time-to-display has expired

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
                headline->setAttribute(Qt::WA_TranslucentBackground, true);

                QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                eff->setOpacity(1.0);
                headline->setGraphicsEffect(eff);
                headline->animation = new QPropertyAnimation(eff, "opacity");
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(1.0);
                headline->animation->setEndValue(story_info->train_age_percent / 100.0);
                headline->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

                headline->animation->start();
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
                headline->setAttribute(Qt::WA_TranslucentBackground, true);

                QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                eff->setOpacity(1.0);
                headline->setGraphicsEffect(eff);
                headline->animation = new QPropertyAnimation(eff, "opacity");
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(1.0);
                headline->animation->setEndValue(story_info->dashboard_age_percent / 100.0);
                headline->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

                headline->animation->start();
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
                headline->animation = new QPropertyAnimation(headline.data(), "geometry");
                headline->animation->setDuration(speed);
                headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
                headline->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_headline_expired);
                prop_anim_map[headline->animation] = headline;
                break;

            case AnimExitType::Fade:
                {
                    headline->setAttribute(Qt::WA_TranslucentBackground, true);

                    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                    eff->setOpacity(1.0);
                    headline->setGraphicsEffect(eff);
                    headline->animation = new QPropertyAnimation(eff, "opacity");
                    headline->animation->setDuration(speed);
                    headline->animation->setStartValue(1.0);
                    headline->animation->setEndValue(0.0);
                    headline->animation->setEasingCurve(QEasingCurve::InCubic);
                    connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_headline_expired);
                    prop_anim_map[headline->animation] = headline;
                }
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
                headline->animation->setEndValue(QRect(-r.width(), r.y(), r.width(), r.height()));
                break;
            case AnimExitType::SlideRight:
                headline->animation->setEndValue(QRect(r_desktop.width() + r.width(), r.y(), r.width(), r.height()));
                break;
            case AnimExitType::SlideUp:
                headline->animation->setEndValue(QRect(r.x(), -r.height(), r.width(), r.height()));
                break;
            case AnimExitType::SlideDown:
                headline->animation->setEndValue(QRect(r.x(), r_desktop.height() + r.height(), r.width(), r.height()));
                break;
            case AnimExitType::SlideFadeLeft:
            case AnimExitType::SlideFadeRight:
            case AnimExitType::SlideFadeUp:
            case AnimExitType::SlideFadeDown:
                break;

            case AnimExitType::Fade:
            case AnimExitType::Pop:
                break;
        }

        exiting_map[headline] = true;
        headline_list.removeAll(headline);

        headline->animation->start();
    }
}

void Chyron::shift_left(int amount)
{
    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(story_info->anim_motion_duration);
        headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        headline->animation->setEasingCurve(QEasingCurve::InCubic);
        headline->animation->setEndValue(QRect(r.x() - amount, r.y(), r.width(), r.height()));

        connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

        animation_group->addAnimation(headline->animation);
    }

    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
    animation_group->start();
}

void Chyron::shift_right(int amount)
{
    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(story_info->anim_motion_duration);
        headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        headline->animation->setEasingCurve(QEasingCurve::InCubic);
        headline->animation->setEndValue(QRect(r.x() + amount, r.y(), r.width(), r.height()));

        connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

        animation_group->addAnimation(headline->animation);
    }

    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
    animation_group->start();
}

void Chyron::shift_up(int amount)
{
    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(story_info->anim_motion_duration);
        headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        headline->animation->setEasingCurve(QEasingCurve::InCubic);
        headline->animation->setEndValue(QRect(r.x(), r.y() - amount, r.width(), r.height()));

        connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

        animation_group->addAnimation(headline->animation);
    }

    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
    animation_group->start();
}

void Chyron::shift_down(int amount)
{
    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(story_info->anim_motion_duration);
        headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        headline->animation->setEasingCurve(QEasingCurve::InCubic);
        headline->animation->setEndValue(QRect(r.x(), r.y() + amount, r.width(), r.height()));

        connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

        animation_group->addAnimation(headline->animation);
    }

    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
    animation_group->start();
}

void Chyron::dashboard_expire_headlines()
{
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
        expired.clear();
    }

    if(headline_list.count() == 1)
        headline_list[0]->bottom_window = nullptr;
}

void Chyron::slot_file_headline(HeadlinePointer headline)
{
    Q_ASSERT(headline->story.toString().compare(story_info->story.toString()) == 0);

    // for some reason, the Producer signal 'signal_new_headline' is
    // coming in here twice on the same call to QMetaObject::activate(),
    // so we have to guard against it...

    if(!incoming_headlines.contains(headline))
        incoming_headlines.enqueue(headline);
}

void Chyron::slot_headline_posted()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    HeadlinePointer headline = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    entering_map.remove(headline);

    if(headline->animation)
        headline->animation->deleteLater();

    headline->viewed = QDateTime::currentDateTime().toTime_t();

    connect(headline.data(), &Headline::signal_mouse_enter, this, &Chyron::slot_headline_mouse_enter);
    connect(headline.data(), &Headline::signal_mouse_exit, this, &Chyron::slot_headline_mouse_exit);

    headline_list.append(headline);

    if(IS_DASHBOARD(story_info->entry_type))
        dashboard_expire_headlines();
}

void Chyron::slot_headline_expired()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    HeadlinePointer headline = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    exiting_map.remove(headline);

    headline->animation->deleteLater();

    headline->hide();
    headline.clear();
}

void Chyron::slot_release_animation()
{
    QPropertyAnimation* prop_anim = qobject_cast<QPropertyAnimation*>(sender());
    if(prop_anim)
        prop_anim->deleteLater();
    else
    {
        QParallelAnimationGroup* group_anim = qobject_cast<QParallelAnimationGroup*>(sender());
        if(group_anim)
            group_anim->deleteLater();
    }
}

void Chyron::slot_age_headlines()
{
    if(entering_map.count() || exiting_map.count())
        return;     // let any running animation finish

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

    QGraphicsEffect* eff = headline->graphicsEffect();
    if(eff)
    {
        QGraphicsOpacityEffect* oeff = qobject_cast<QGraphicsOpacityEffect*>(eff);
        if(oeff)
        {
            opacity_map[headline] = oeff->opacity();

            headline->animation = new QPropertyAnimation(oeff, "opacity");
            headline->animation->setDuration(150);
            headline->animation->setStartValue(oeff->opacity());
            headline->animation->setEndValue(1.0);
            headline->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

            headline->animation->start();
        }
    }
}

void Chyron::slot_headline_mouse_exit()
{
    Headline* headline = qobject_cast<Headline*>(sender());

    if(opacity_map.contains(headline))
    {
        QGraphicsEffect* eff = headline->graphicsEffect();
        if(eff)
        {
            QGraphicsOpacityEffect* oeff = qobject_cast<QGraphicsOpacityEffect*>(eff);
            headline->animation = new QPropertyAnimation(oeff, "opacity");
            headline->animation->setDuration(150);
            headline->animation->setStartValue(1.0);
            headline->animation->setEndValue(opacity_map[headline]);
            headline->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

            headline->animation->start();
        }

        opacity_map.remove(headline);
    }
}
