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

Chyron::Chyron(const QUrl& story, const Chyron::Settings& chyron_settings, LaneManagerPointer lane_manager, QObject* parent)
    : story(story),
      settings(chyron_settings),
//      lane_position(QRect(0,0,0,0)),
      lane_manager(lane_manager),
#ifdef HIGHLIGHT_LANES
      highlight(nullptr),
#endif
      QObject(parent)
{
    age_timer = new QTimer(this);
    age_timer->setInterval(100);
    connect(age_timer, &QTimer::timeout, this, &Chyron::slot_age_headlines);
    age_timer->start();

    lane_manager->subscribe(this);

#ifdef HIGHLIGHT_LANES
    highlight = new HighlightWidget();
#endif
}

Chyron::~Chyron()
{
#ifdef HIGHLIGHT_LANES
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

    const QRect& lane_position = lane_manager->get_base_lane_position(this);

    if(!settings.headline_fixed_width && !settings.headline_fixed_height)
        headline->initialize(settings.always_visible);

    int x = 0;
    int y = 0;
    QRect r = headline->geometry();
    int width = settings.headline_fixed_width ? settings.headline_fixed_width : r.width();
    int height = settings.headline_fixed_height ? settings.headline_fixed_height : r.height();

    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
            y = lane_position.top() - height;
            x = lane_position.left() + settings.margin;
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
            x = lane_position.right() - width - settings.margin;
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
            y = lane_position.top() + settings.margin;
            x = lane_position.left() - width;
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            y = lane_position.top() + settings.margin;
            x = lane_position.right() + width;
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
            y = lane_position.bottom() - height - settings.margin;
            x = lane_position.left() - width;
            break;
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            y = lane_position.bottom() - height - settings.margin;
            x = lane_position.right() + width;
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
            y = lane_position.bottom() + height;
            x = lane_position.left() + settings.margin;
            break;
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            y = lane_position.bottom() + height;
            x = lane_position.right() - width - settings.margin;
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
            y = lane_position.top() + settings.margin;
            x = lane_position.left() + settings.margin;
            break;
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::PopRightTop:
            y = lane_position.top() + settings.margin;
            x = lane_position.right() - width - settings.margin;
            break;
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::PopLeftBottom:
            y = lane_position.bottom() - height - settings.margin;
            x = lane_position.left() + settings.margin;
            break;
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::PopRightBottom:
            y = lane_position.bottom() - height - settings.margin;
            x = lane_position.right() - width - settings.margin;
            break;
    }

    headline->setGeometry(x, y, width, height);

    if(settings.headline_fixed_width || settings.headline_fixed_height)
        headline->initialize(settings.always_visible, settings.headline_fixed_text, width, height);

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
    int speed = 500;

    QParallelAnimationGroup* animation_group = nullptr;
    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(settings.display);
    QRect r = headline->geometry();

    switch(settings.entry_type)
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

                if(settings.entry_type >= AnimEntryType::TrainDownLeftTop && settings.entry_type <= AnimEntryType::TrainUpCenterBottom)
                    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_train_expire_headlines);
            }
            break;

        case AnimEntryType::PopCenter:
        case AnimEntryType::PopLeftTop:
        case AnimEntryType::PopRightTop:
        case AnimEntryType::PopLeftBottom:
        case AnimEntryType::PopRightBottom:
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

    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownLeftTop:
        case AnimEntryType::TrainDownCenterTop:
        case AnimEntryType::TrainDownRightTop:
            headline->animation->setEndValue(QRect(r.x(), r_desktop.top() + settings.margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() + r.height() + settings.margin, posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
            headline->animation->setEndValue(QRect(r_desktop.left() + settings.margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + settings.margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            headline->animation->setEndValue(QRect(r_desktop.width() - r.width() - settings.margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - settings.margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
            headline->animation->setEndValue(QRect(r_desktop.left() + settings.margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + settings.margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            headline->animation->setEndValue(QRect(r_desktop.width() - r.width() - settings.margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - settings.margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpLeftBottom:
        case AnimEntryType::TrainUpRightBottom:
        case AnimEntryType::TrainUpCenterBottom:
            headline->animation->setEndValue(QRect(r.x(), r_desktop.bottom() - r.height() - settings.margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(HeadlinePointer posted_headline, headline_list)
                {
                    QRect posted_r = posted_headline->geometry();
                    configure_group_item(posted_headline->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() - r.height() - settings.margin, posted_r.width(), posted_r.height()));
                }
            }
            break;

        case AnimEntryType::FadeCenter:
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::FadeRightBottom:
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
    else
        headline->animation->start();
}

//
void Chyron::start_headline_exit(HeadlinePointer headline)
{
    // the time-to-display has expired

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(settings.display);
    QRect r = headline->geometry();

    int speed = 500;

    if(settings.entry_type >= AnimEntryType::TrainDownLeftTop && settings.entry_type <= AnimEntryType::TrainUpCenterBottom)
    {
        if(settings.effect == AgeEffects::ReduceOpacityFixed)
        {
            headline->setAttribute(Qt::WA_TranslucentBackground, true);

            QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
            eff->setOpacity(1.0);
            headline->setGraphicsEffect(eff);
            headline->animation = new QPropertyAnimation(eff, "opacity");
            headline->animation->setDuration(speed);
            headline->animation->setStartValue(1.0);
            headline->animation->setEndValue(settings.train_reduce_opacity / 100.0);
            headline->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

            headline->animation->start();
        }

        headline->ignore = true;     // ignore subsequent 'aging' activities on this one
    }
    else
    {
        switch(settings.exit_type)
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

        switch(settings.exit_type)
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
    int speed = 500;

    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(speed);
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
    int speed = 500;

    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(speed);
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
    int speed = 500;

    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(speed);
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
    int speed = 500;

    if(!headline_list.length())
        return;     // no headlines visible

    QParallelAnimationGroup* animation_group = new QParallelAnimationGroup();
    foreach(HeadlinePointer headline, headline_list)
    {
        QRect r = headline->geometry();
        headline->animation = new QPropertyAnimation(headline.data(), "geometry");
        headline->animation->setDuration(speed);
        headline->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
        headline->animation->setEasingCurve(QEasingCurve::InCubic);
        headline->animation->setEndValue(QRect(r.x(), r.y() + amount, r.width(), r.height()));

        connect(headline->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);

        animation_group->addAnimation(headline->animation);
    }

    connect(animation_group, &QParallelAnimationGroup::finished, this, &Chyron::slot_release_animation);
    animation_group->start();
}

void Chyron::slot_file_headline(HeadlinePointer headline)
{
    Q_ASSERT(headline->story.toString().compare(story.toString()) == 0);
    incoming_headlines.enqueue(headline);
}

void Chyron::slot_headline_posted()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    HeadlinePointer headline = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    entering_map.remove(headline);

    headline->animation->deleteLater();

    headline->viewed = QDateTime::currentDateTime().toTime_t();

    connect(headline.data(), &Headline::signal_mouse_enter, this, &Chyron::slot_headline_mouse_enter);
    connect(headline.data(), &Headline::signal_mouse_exit, this, &Chyron::slot_headline_mouse_exit);

    headline_list.append(headline);
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
                if((now - headline->viewed) > settings.ttl)
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
    QRect r_desktop = desktop->screenGeometry(settings.display);

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
