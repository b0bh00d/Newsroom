#include <limits>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtCore/QUrl>
#include <QtCore/QDateTime>

#include "chyron.h"

Chyron::Chyron(const QUrl& story, const Chyron::Settings& chyron_settings, QObject* parent)
    : story(story),
      settings(chyron_settings),
      QObject(parent)
{
    age_timer = new QTimer(this);
    age_timer->setInterval(100);
    connect(age_timer, &QTimer::timeout, this, &Chyron::slot_age_headlines);
    age_timer->start();
}

Chyron::~Chyron()
{
    foreach(HeadlinePointer headline, headline_list)
    {
        headline->hide();
        headline.clear();
    }
}

void Chyron::initialize_headline(HeadlinePointer headline)
{
    headline->configure(settings.always_visible);

    int x = 0;
    int y = 0;
    QRect r = headline->geometry();
    int width = settings.headline_fixed_width ? settings.headline_fixed_width : r.width();
    int height = r.height();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(settings.display);
    switch(settings.entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::TrainDownLeftTop:
            y = r_desktop.top() - height;
            x = r_desktop.left() + settings.margin;
            break;
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::TrainDownCenterTop:
            y = r_desktop.top() - height;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::SlideDownRightTop:
        case AnimEntryType::TrainDownRightTop:
            y = r_desktop.top() - height;
            x = r_desktop.right() - width - settings.margin;
            break;
        case AnimEntryType::SlideInLeftTop:
        case AnimEntryType::TrainInLeftTop:
            y = r_desktop.top() + settings.margin;
            x = r_desktop.left() - width;
            break;
        case AnimEntryType::SlideInRightTop:
        case AnimEntryType::TrainInRightTop:
            y = r_desktop.top() + settings.margin;
            x = r_desktop.right() + width;
            break;
        case AnimEntryType::SlideInLeftBottom:
        case AnimEntryType::TrainInLeftBottom:
            y = r_desktop.bottom() - height - settings.margin;
            x = r_desktop.left() - width;
            break;
        case AnimEntryType::SlideInRightBottom:
        case AnimEntryType::TrainInRightBottom:
            y = r_desktop.bottom() - height - settings.margin;
            x = r_desktop.right() + width;
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::TrainUpLeftBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.left() + settings.margin;
            break;
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::TrainUpRightBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.right() - width - settings.margin;
            break;
        case AnimEntryType::SlideUpCenterBottom:
        case AnimEntryType::TrainUpCenterBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::FadeCenter:
        case AnimEntryType::PopCenter:
            y = r_desktop.top() + (r_desktop.height() - height) / 2;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::FadeLeftTop:
        case AnimEntryType::PopLeftTop:
            y = r_desktop.top() + settings.margin;
            x = r_desktop.left() + settings.margin;
            break;
        case AnimEntryType::FadeRightTop:
        case AnimEntryType::PopRightTop:
            y = r_desktop.top() + settings.margin;
            x = r_desktop.right() - width - settings.margin;
            break;
        case AnimEntryType::FadeLeftBottom:
        case AnimEntryType::PopLeftBottom:
            y = r_desktop.bottom() - height - settings.margin;
            x = r_desktop.left() + settings.margin;
            break;
        case AnimEntryType::FadeRightBottom:
        case AnimEntryType::PopRightBottom:
            y = r_desktop.bottom() - height - settings.margin;
            x = r_desktop.right() - width - settings.margin;
            break;
    }

    headline->setGeometry(x, y, width, height);
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
            headline->animation->setEndValue(QRect(r_desktop.left() + settings.margin, r_desktop.top() + settings.margin, r.width(), r.height()));

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
            headline->animation->setEndValue(QRect(r_desktop.width() - r.width() - settings.margin, r_desktop.top() + settings.margin, r.width(), r.height()));

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
                break;
        }

        exiting_map[headline] = true;
        headline_list.removeAll(headline);

        headline->animation->start();
    }
}

void Chyron::slot_file_headline(HeadlinePointer headline)
{
    if(headline->story.toString().compare(story.toString()))
        return;     // this headline isn't for this story
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
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    anim->deleteLater();
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
