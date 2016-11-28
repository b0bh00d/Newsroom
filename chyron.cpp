#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

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
    int width = article->w;
    int height = article->h;

    QDesktopWidget* desktop = QApplication::desktop();
    QRect r = desktop->screenGeometry(display);
    switch(entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
            y = -height;
            x = margin;
            break;
        case AnimEntryType::SlideDownCenterTop:
            y = -height;
            x = (r.width() - width) / 2;
            break;
        case AnimEntryType::SlideDownRightTop:
            y = -height;
            x = r.right() - width - margin;
            break;
        case AnimEntryType::SlideInLeftTop:
            y = margin;
            x = r.left() - width;
            break;
        case AnimEntryType::SlideInRightTop:
            y = margin;
            x = r.right() + width;
            break;
        case AnimEntryType::SlideInLeftBottom:
            y = r.bottom() - height - margin;
            x = -width;
            break;
        case AnimEntryType::SlideInRightBottom:
            y = r.bottom() - height - margin;
            x = r.right() + width;
            break;
        case AnimEntryType::SlideUpLeftBottom:
            y = r.bottom() + height;
            x = margin;
            break;
        case AnimEntryType::SlideUpRightBottom:
            y = r.bottom() + height;
            x = r.right() - width - margin;
            break;
        case AnimEntryType::SlideUpCenterBottom:
            y = r.bottom() + height;
            x = (r.width() - width) / 2;
            break;
        case AnimEntryType::FadeInCenter:
        case AnimEntryType::PopCenter:
            y = (r.height() - height) / 2;
            x = (r.width() - width) / 2;
            break;
        case AnimEntryType::PopLeftTop:
            y = margin;
            x = margin;
            break;
        case AnimEntryType::PopRightTop:
            y = margin;
            x = r.right() - width - margin;
            break;
        case AnimEntryType::PopLeftBottom:
            y = r.height() - height - margin;
            x = margin;
            break;
        case AnimEntryType::PopRightBottom:
            y = r.height() - height - margin;
            x = r.width() - width - margin;
            break;
    }

    article->setGeometry(x, y, width, height);
}

void Chyron::start_article_entry(ArticlePointer article)
{
//    QPropertyAnimation* animation = nullptr;
    int speed = 500;

    QParallelAnimationGroup* animation_group = nullptr;
//    QDesktopWidget* desktop = QApplication::desktop();
//    QRect r_desktop = desktop->screenGeometry(display);
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
    }

    switch(entry_type)
    {
        case AnimEntryType::SlideDownLeftTop:
        case AnimEntryType::SlideDownCenterTop:
        case AnimEntryType::SlideDownRightTop:
            article->animation->setDuration(speed);
            article->animation->setStartValue(QRect(r.x(), r.y(), r.width(), r.height()));
            article->animation->setEndValue(QRect(r.x(), margin, r.width(), r.height()));
            article->animation->setEasingCurve(QEasingCurve::InCubic);

            if(animation_group)
            {
                foreach(ArticlePointer posted_article, article_list)
                {
                    QRect posted_r = posted_article->geometry();
                    posted_article->animation->setDuration(speed);
                    posted_article->animation->setStartValue(QRect(posted_r.x(), posted_r.y(), posted_r.width(), posted_r.height()));
                    posted_article->animation->setEndValue(QRect(posted_r.x(), posted_r.y() + r.height() + margin, posted_r.width(), posted_r.height()));
                    posted_article->animation->setEasingCurve(QEasingCurve::InCubic);
                }
            }
            break;
        case AnimEntryType::SlideInLeftTop:
            break;
        case AnimEntryType::SlideInRightTop:
            break;
        case AnimEntryType::SlideInLeftBottom:
            break;
        case AnimEntryType::SlideInRightBottom:
            break;
        case AnimEntryType::SlideUpLeftBottom:
            break;
        case AnimEntryType::SlideUpRightBottom:
            break;
        case AnimEntryType::SlideUpCenterBottom:
            break;

        case AnimEntryType::FadeInCenter:
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
