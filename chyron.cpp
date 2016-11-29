#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtCore/QUrl>
#include <QtCore/QDateTime>

#include "chyron.h"

Chyron::Chyron(const QUrl&      story,
               uint             ttl,
               int              display,
               AnimEntryType    entry_type,
               AnimExitType     exit_type,
               ReportStacking   stacking_type,
               int              margin,
               QObject*         parent)
    : story(story),
      ttl(ttl),
      display(display),
      entry_type(entry_type),
      exit_type(exit_type),
      stacking_type(stacking_type),
      margin(margin),
      QObject(parent)
{
    age_timer = new QTimer(this);
    age_timer->setInterval(100);
    connect(age_timer, &QTimer::timeout, this, &Chyron::slot_age_articles);
    age_timer->start();
}

void Chyron::initialize_article_position(ArticlePointer article)
{
    int x = 0;
    int y = 0;

    QRect r = article->geometry();
    int width = r.width();
    int height = r.height();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(display);
    switch(entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
            y = r_desktop.top() - height;
            x = r_desktop.left() + margin;
            break;
        case AnimEntryType::SlideDownCenterTop:
            y = r_desktop.top() - height;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::SlideDownRightTop:
            y = r_desktop.top() - height;
            x = r_desktop.right() - width - margin;
            break;
        case AnimEntryType::SlideInLeftTop:
            y = r_desktop.top() + margin;
            x = r_desktop.left() - width;
            break;
        case AnimEntryType::SlideInRightTop:
            y = r_desktop.top() + margin;
            x = r_desktop.right() + width;
            break;
        case AnimEntryType::SlideInLeftBottom:
            y = r_desktop.bottom() - height - margin;
            x = r_desktop.left() - width;
            break;
        case AnimEntryType::SlideInRightBottom:
            y = r_desktop.bottom() - height - margin;
            x = r_desktop.right() + width;
            break;
        case AnimEntryType::SlideUpLeftBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.left() + margin;
            break;
        case AnimEntryType::SlideUpRightBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.right() - width - margin;
            break;
        case AnimEntryType::SlideUpCenterBottom:
            y = r_desktop.bottom() + height;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::FadeInCenter:
        case AnimEntryType::PopCenter:
            y = r_desktop.top() + (r_desktop.height() - height) / 2;
            x = r_desktop.left() + (r_desktop.width() - width) / 2;
            break;
        case AnimEntryType::FadeInLeftTop:
        case AnimEntryType::PopLeftTop:
            y = r_desktop.top() + margin;
            x = r_desktop.left() + margin;
            break;
        case AnimEntryType::FadeInRightTop:
        case AnimEntryType::PopRightTop:
            y = r_desktop.top() + margin;
            x = r_desktop.right() - width - margin;
            break;
        case AnimEntryType::FadeInLeftBottom:
        case AnimEntryType::PopLeftBottom:
            y = r_desktop.bottom() - height - margin;
            x = r_desktop.left() + margin;
            break;
        case AnimEntryType::FadeInRightBottom:
        case AnimEntryType::PopRightBottom:
            y = r_desktop.bottom() - height - margin;
            x = r_desktop.right() - width - margin;
            break;
    }

    article->setGeometry(x, y, width, height);
}

void Chyron::start_article_entry(ArticlePointer article)
{
    int speed = 500;

    QParallelAnimationGroup* animation_group = nullptr;
    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(display);
    QRect r = article->geometry();

    switch(entry_type)
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
            article->animation = new QPropertyAnimation(article.data(), "geometry");
            article->animation->setDuration(speed);
            article->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            article->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(article->animation, &QPropertyAnimation::finished, this, &Chyron::slot_article_posted);
            prop_anim_map[article->animation] = article;

            if(article_list.length())
            {
                animation_group = new QParallelAnimationGroup();
                foreach(ArticlePointer posted_article, article_list)
                {
                    posted_article->animation = new QPropertyAnimation(posted_article.data(), "geometry");
                    connect(posted_article->animation, &QPropertyAnimation::finished, this, &Chyron::slot_release_animation);
                }
            }
            break;

        case AnimEntryType::PopCenter:
        case AnimEntryType::PopLeftTop:
        case AnimEntryType::PopRightTop:
        case AnimEntryType::PopLeftBottom:
        case AnimEntryType::PopRightBottom:
            article->viewed = QDateTime::currentDateTime().toTime_t();
            article->show();
            break;

        case AnimEntryType::FadeInCenter:
        case AnimEntryType::FadeInLeftTop:
        case AnimEntryType::FadeInRightTop:
        case AnimEntryType::FadeInLeftBottom:
        case AnimEntryType::FadeInRightBottom:
            {
                article->setAttribute(Qt::WA_TranslucentBackground, true);

                QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                eff->setOpacity(0.0);
                article->setGraphicsEffect(eff);
                article->animation = new QPropertyAnimation(eff, "opacity");
                article->animation->setDuration(speed);
                article->animation->setStartValue(0.0);
                article->animation->setEndValue(1.0);
                article->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(article->animation, &QPropertyAnimation::finished, this, &Chyron::slot_article_posted);
                prop_anim_map[article->animation] = article;
            }
            break;
    }

    auto configure_group_item = [](QPropertyAnimation* anim, int speed, const QRect& start, const QRect& end) {
        anim->setDuration(speed);
        anim->setStartValue(start);
        anim->setEndValue(end);
        anim->setEasingCurve(QEasingCurve::InCubic);
    };

    switch(entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::SlideDownRightTop:
            article->animation->setEndValue(QRect(r.x(), margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() + r.height() + margin, posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInLeftTop:
            article->animation->setEndValue(QRect(margin, margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInRightTop:
            article->animation->setEndValue(QRect(r_desktop.width() - r.width() - margin, margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInLeftBottom:
            article->animation->setEndValue(QRect(margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x() + r.width() + margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideInRightBottom:
            article->animation->setEndValue(QRect(r_desktop.width() - r.width() - margin, r.y(), r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x() - r.width() - margin, posted_r.y(), posted_r.width(), posted_r.height()));
                }
            }
            break;
        case AnimEntryType::SlideUpLeftBottom:
        case AnimEntryType::SlideUpRightBottom:
        case AnimEntryType::SlideUpCenterBottom:
            article->animation->setEndValue(QRect(r.x(), r_desktop.bottom() - r.height() - margin, r.width(), r.height()));

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    configure_group_item(posted_article->animation, speed, posted_r,
                                         QRect(posted_r.x(), posted_r.y() - r.height() - margin, posted_r.width(), posted_r.height()));
                }
            }
            break;

        case AnimEntryType::FadeInCenter:
        case AnimEntryType::FadeInLeftTop:
        case AnimEntryType::FadeInRightTop:
        case AnimEntryType::FadeInLeftBottom:
        case AnimEntryType::FadeInRightBottom:
            break;
    }

    entering_map[article] = true;
    article->viewed = QDateTime::currentDateTime().toTime_t();
    article->show();

    if(animation_group)
    {
        animation_group->addAnimation(article->animation);
        foreach(ArticlePointer posted_article, article_list)
            animation_group->addAnimation(posted_article->animation);
       animation_group->start();
    }
    else
        article->animation->start();
}

void Chyron::start_article_exit(ArticlePointer article)
{
    QDesktopWidget* desktop = QApplication::desktop();
    QRect r_desktop = desktop->screenGeometry(display);
    QRect r = article->geometry();

    int speed = 500;

    switch(exit_type)
    {
        case AnimExitType::SlideLeft:
        case AnimExitType::SlideRight:
        case AnimExitType::SlideUp:
        case AnimExitType::SlideDown:
        case AnimExitType::SlideFadeLeft:
        case AnimExitType::SlideFadeRight:
        case AnimExitType::SlideFadeUp:
        case AnimExitType::SlideFadeDown:
            article->animation = new QPropertyAnimation(article.data(), "geometry");
            article->animation->setDuration(speed);
            article->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            article->animation->setEasingCurve(QEasingCurve::InCubic);
            connect(article->animation, &QPropertyAnimation::finished, this, &Chyron::slot_article_expired);
            prop_anim_map[article->animation] = article;
            break;

        case AnimExitType::Fade:
            {
                article->setAttribute(Qt::WA_TranslucentBackground, true);

                QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
                eff->setOpacity(1.0);
                article->setGraphicsEffect(eff);
                article->animation = new QPropertyAnimation(eff, "opacity");
                article->animation->setDuration(speed);
                article->animation->setStartValue(1.0);
                article->animation->setEndValue(0.0);
                article->animation->setEasingCurve(QEasingCurve::InCubic);
                connect(article->animation, &QPropertyAnimation::finished, this, &Chyron::slot_article_expired);
                prop_anim_map[article->animation] = article;
            }
            break;

        case AnimExitType::Pop:
            article_list.removeAll(article);
            article->hide();
            article.clear();
            break;
    }

    switch(exit_type)
    {
        case AnimExitType::SlideLeft:
            article->animation->setEndValue(QRect(-r.width(), r.y(), r.width(), r.height()));
            break;
        case AnimExitType::SlideRight:
            article->animation->setEndValue(QRect(r_desktop.width() + r.width(), r.y(), r.width(), r.height()));
            break;
        case AnimExitType::SlideUp:
            article->animation->setEndValue(QRect(r.x(), -r.height(), r.width(), r.height()));
            break;
        case AnimExitType::SlideDown:
            article->animation->setEndValue(QRect(r.x(), r_desktop.height() + r.height(), r.width(), r.height()));
            break;
        case AnimExitType::SlideFadeLeft:
        case AnimExitType::SlideFadeRight:
        case AnimExitType::SlideFadeUp:
        case AnimExitType::SlideFadeDown:
            break;

        case AnimExitType::Fade:
            break;
    }

    exiting_map[article] = true;
    article_list.removeAll(article);

    article->animation->start();
}

void Chyron::slot_file_article(ArticlePointer article)
{
    if(article->story.toString().compare(story.toString()))
        return;     // this article isn't for this story
    incoming_articles.enqueue(article);
}

void Chyron::slot_article_posted()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    ArticlePointer article = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    entering_map.remove(article);

    article->animation->deleteLater();

    article->viewed = QDateTime::currentDateTime().toTime_t();
    article_list.append(article);
}

void Chyron::slot_article_expired()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    ArticlePointer article = prop_anim_map[anim];

    prop_anim_map.remove(anim);
    exiting_map.remove(article);

    article->animation->deleteLater();

    article->hide();
    article.clear();
}

void Chyron::slot_release_animation()
{
    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    anim->deleteLater();
}

void Chyron::slot_age_articles()
{
    if(entering_map.count() || exiting_map.count())
        return;     // let any running animation finish

    uint now = QDateTime::currentDateTime().toTime_t();

    if(incoming_articles.length())
    {
        ArticlePointer article = incoming_articles.dequeue();
        article->viewed = 0;    // let's us know when the article was first displayed
        initialize_article_position(article);

        // inject an article into the chyron
        start_article_entry(article);
    }
    else
    {
        foreach(ArticlePointer article, article_list)
        {
            if((now - article->viewed) > ttl)
            {
                start_article_exit(article);
                break;
            }
        }
    }
}
