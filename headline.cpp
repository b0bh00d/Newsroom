#ifdef QT_WIN
#include <Windows.h>
#endif

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGraphicsOpacityEffect>

#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <QtCore/QUrl>
#include <QtCore/QRegExp>
#include <QtCore/QPropertyAnimation>

#include "headline.h"

Headline::Headline(StoryInfoPointer story_info,
                   const QString& headline,
                   Qt::Alignment alignment,
                   QWidget* parent)
    : QLabel(parent),
      story_info(story_info),
      headline(headline)
{
    setAlignment(alignment);

    hover_timer = new QTimer(this);
    if(hover_timer)
        connect(hover_timer, &QTimer::timeout, this, &Headline::slot_zoom_in);

    set_font(story_info->font);
}

Headline::~Headline()
{
    if(hover_timer)
    {
        hover_timer->stop();
        hover_timer->deleteLater();
        hover_timer = nullptr;
    }
}

bool Headline::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    *result = 0L;

    if(!stay_visible)
    {
#ifdef QT_WIN
        if(!QString(eventType).compare("windows_generic_MSG"))
        {
            auto msg = static_cast<MSG*>(message);

            if(msg->message == WM_WINDOWPOSCHANGING)
            {
                RECT    r;
                HWND    bottom{nullptr};

                // assume the first window in the Z order that is consuming
                // the entire virtual desktop size IS the desktop (e.g.,
                // "Program Manager"), and glue ourselves right on top of it
                auto desktop_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                auto desktop_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                auto w = GetTopWindow(nullptr);
                while(w)
                {
                    if(!bottom_window)
                    {
                        GetWindowRect(w, &r);
                        if((r.right - r.left) == desktop_width && (r.bottom - r.top) == desktop_height)
                            break;
                    }
                    else if(reinterpret_cast<HWND>(bottom_window->winId()) == w)
                        break;

                    if(IsWindowVisible(w))
                        bottom = w;

                    w = GetNextWindow(w, GW_HWNDNEXT);
                }

                auto pwpos = reinterpret_cast<WINDOWPOS*>(msg->lParam);
                pwpos->hwndInsertAfter = bottom;
                pwpos->flags &= static_cast<unsigned int>(~SWP_NOZORDER);
                // fall through to the return
            }
        }
#endif
    }

    return false;
}

void Headline::enterEvent(QEvent *event)
{
    mouse_in_widget = true;
    if(!compact_mode)
        emit signal_mouse_enter();
    else
    {
        hover_timer->setInterval(1000);
        hover_timer->start();
    }
    event->accept();
}

void Headline::leaveEvent(QEvent *event)
{
    mouse_in_widget = false;
    if(!compact_mode && !is_zoomed)
        emit signal_mouse_exit();
    else
    {
        if(hover_timer)
            hover_timer->stop();
        zoom_out();
    }
    event->accept();
}

void Headline::zoom_out()
{
    if(mouse_in_widget || !is_zoomed)
        return;

    compact_mode = true;

    prepare_to_zoom_out();

    auto animation = new QPropertyAnimation(this, "geometry", this);
    animation->setDuration(200);
    animation->setStartValue(target_geometry);
    animation->setEndValue(starting_geometry);
    animation->setEasingCurve(story_info->motion_curve);
    connect(animation, &QPropertyAnimation::finished, this, &Headline::slot_turn_on_compact_mode);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    is_zoomed = false;
}

void Headline::slot_turn_on_compact_mode()
{
    stay_visible = was_stay_visible;
    if(!stay_visible)
        lower();

    zoomed_out();

    setWindowOpacity(old_opacity);

    repaint();
}

void Headline::slot_zoom_in()
{
    hover_timer->stop();

    if(!mouse_in_widget || is_zoomed)
        return;

    prepare_to_zoom_in();

    starting_geometry = geometry();

    // we need to figure out a direction to expand.

    switch(story_info->entry_type)
    {
        case AnimEntryType::DashboardDownLeftTop:
        case AnimEntryType::DashboardInLeftTop:
            target_geometry = QRect(starting_geometry.x(), starting_geometry.x(), original_w, original_h);
            break;
        case AnimEntryType::DashboardDownRightTop:
        case AnimEntryType::DashboardInRightTop:
            target_geometry = QRect(starting_geometry.x() - (original_w - starting_geometry.width()),
                                    starting_geometry.y(), original_w, original_h);
            break;
        case AnimEntryType::DashboardInLeftBottom:
        case AnimEntryType::DashboardUpLeftBottom:
            target_geometry = QRect(starting_geometry.x(),
                                    starting_geometry.y() - (original_h - starting_geometry.height()),
                                    original_w, original_h);
            break;
        case AnimEntryType::DashboardInRightBottom:
        case AnimEntryType::DashboardUpRightBottom:
            target_geometry = QRect(starting_geometry.x() - (original_w - starting_geometry.width()),
                                    starting_geometry.y() - (original_h - starting_geometry.height()),
                                    original_w, original_h);
            break;

        default:
            break;
    }

    // if any of our visible geometry exceeds the boundaries of
    // the screen, it cannot be used.

    // expand down-and-right from top-left point
//    target_geometry = QRect(starting_geometry.x(), starting_geometry.x(), original_w, original_h);
//    if(target_geometry.right() > r_desktop.right() || target_geometry.bottom() > r_desktop.bottom())
//    {
//        // expand up-and-right from bottom-left point
//        target_geometry = QRect(starting_geometry.x(),
//                                starting_geometry.y() - (original_h - starting_geometry.height()),
//                                original_w, original_h);
//        if(target_geometry.right() > r_desktop.right() || target_geometry.top() < r_desktop.top())
//        {
//            // expand up-and-left from top-right point
//            target_geometry = QRect(starting_geometry.x() - (original_w - starting_geometry.width()),
//                                    starting_geometry.y() - (original_h - starting_geometry.height()),
//                                    original_w, original_h);
//            if(target_geometry.left() < r_desktop.left() || target_geometry.top() < r_desktop.top())
//            {
//                // expand down-and-left from bottom-right point
//                target_geometry = QRect(starting_geometry.x() - (original_w - starting_geometry.width()),
//                                        starting_geometry.y(), original_w, original_h);
//            }
//        }
//    }

    auto animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(200);
    animation->setStartValue(starting_geometry);
    animation->setEndValue(target_geometry);
    animation->setEasingCurve(story_info->motion_curve);
    connect(animation, &QPropertyAnimation::finished, this, &Headline::slot_turn_off_compact_mode);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    is_zoomed = true;
}

void Headline::slot_turn_off_compact_mode()
{
    was_stay_visible = stay_visible;
    stay_visible = true;
    raise();

    compact_mode = false;

    zoomed_in();

    old_opacity = windowOpacity();
    setWindowOpacity(1.0);

    repaint();
}

void Headline::set_compact_mode(bool compact, int original_width, int original_height)
{
    compact_mode = compact;
    original_w = original_width;
    original_h = original_height;
}

void Headline::initialize(bool stay_visible, FixedText fixed_text, int width, int height)
{
    this->stay_visible = stay_visible;

    // https://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    if(stay_visible)
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);

//    setParent(0); // Create TopLevel-Widget
    setAttribute(Qt::WA_ShowWithoutActivating);
//    setAttribute(Qt::WA_TranslucentBackground, true);

    shrink_text_to_fit = (fixed_text == FixedText::ScaleToFit);

    setContentsMargins(0, 0, 0, 0);
    setTextFormat(Qt::AutoText);
    setMargin(margin);
    setFont(font);
    setStyleSheet(stylesheet);
    setText(headline);

    auto r = geometry();
    setGeometry(r.x(), r.y(), width, height);
}

//-----------------------------------------------------------------------

PortraitHeadline::PortraitHeadline(StoryInfoPointer story_info_,
                                   const QString& headline_,
                                   Qt::Alignment alignment,
                                   bool configure_for_left,
                                   QWidget *parent)
    : Headline(story_info_, headline_, alignment, parent),
      configure_for_left(configure_for_left)
{
}

PortraitHeadline::~PortraitHeadline()
{
}

void PortraitHeadline::paintEvent(QPaintEvent* /*event*/)
{
    if(compact_mode)
        return;

    QPainter painter(this);
    auto s = geometry();

    if(reporter_draw)
    {
        QRect bounds(margin, margin, s.width() - (margin * 2), s.height() - (margin * 2));
        emit signal_reporter_draw(bounds, painter);
        return;
    }

    QTextDocument td;
    td.setDocumentMargin(margin);

    QRegExp html_tags("<[^>]*>");
    if(html_tags.indexIn(text()) != -1)
        td.setHtml(text());
    else
        td.setPlainText(text());
    auto doc_size = td.documentLayout()->documentSize();

    if(shrink_text_to_fit)
    {
        auto f = td.defaultFont();
        for(;;)
        {
            doc_size = td.documentLayout()->documentSize();
            if(doc_size.width() < s.height() && doc_size.height() < s.width())
                break;

            if((f.pointSizeF() - .1) < 4.0)
            {
                // let it just clip the remaining text
                //f.setPointSizeF(original_point_size);
                //td.setDefaultFont(f);
                break;
            }

            f.setPointSizeF(f.pointSizeF() - .1);
            td.setDefaultFont(f);
        }
    }

    auto y{0};
    auto x{0};

    if(alignment() & Qt::AlignVCenter)
    {
        y = static_cast<int>((s.width() - doc_size.height()) / 2);
        y = (y < 0) ? 0 : y;
    }
    else if(alignment() & Qt::AlignBottom)
    {
        y = static_cast<int>(s.width() - doc_size.height());
        y = (y < 0) ? 0 : y;
    }

    if(alignment() & Qt::AlignHCenter)
    {
        x = static_cast<int>((s.height() - doc_size.width()) / 2);
        x = (x < 0) ? 0 : x;
    }
    else if(alignment() & Qt::AlignRight)
    {
        x = static_cast<int>(s.height() - doc_size.width());
        x = (x < 0) ? 0 : x;
    }

    painter.save();

    if(configure_for_left)
    {
        painter.translate(y, s.height() - x);
        painter.rotate(270);
    }
    else
    {
        painter.translate(s.width() - y, x);
        painter.rotate(90);
    }

    if(!shrink_text_to_fit)
        painter.setClipRect(margin, margin, s.width() - margin * 2, s.height() - margin * 2);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, painter.pen().color());
    td.documentLayout()->draw(&painter, ctx);

    painter.restore();
}

QSize PortraitHeadline::minimumSizeHint() const
{
    auto s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize PortraitHeadline::sizeHint() const
{
    auto s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}

void PortraitHeadline::initialize(bool stay_visible_, FixedText fixed_text, int width, int height)
{
    Headline::initialize(stay_visible_, fixed_text, width, height);
    set_for_left(story_info->entry_type != AnimEntryType::DashboardInRightTop &&
                 story_info->entry_type != AnimEntryType::DashboardInRightBottom);
}

//-----------------------------------------------------------------------

LandscapeHeadline::LandscapeHeadline(StoryInfoPointer story_info_,
                                     const QString& headline_,
                                     Qt::Alignment alignment,
                                     QWidget* parent)
    : Headline(story_info_, headline_, alignment, parent)
{
    auto p = palette();
    progress_color = p.color(QPalette::Active, QPalette::Mid).lighter(75);
    progress_highlight = progress_color.lighter(135);

    if(story_info->include_progress_bar)
        enable_progress_detection(story_info->progress_text_re, story_info->progress_on_top);
}

LandscapeHeadline::~LandscapeHeadline()
{
}

void LandscapeHeadline::enable_progress_detection(const QString& re, bool on_top)
{
    detect_progress = true;
    progress_re = re;
    progress_on_top = on_top;

    progress_x = margin;
    progress_y = margin;     // preset for top
    progress_w = 0;
    progress_h = 5;
}

void LandscapeHeadline::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::StyleChange)
    {
        auto stylesheet = styleSheet();
        QRegExp bc("background\\-color\\s*:.*rgb[a]?\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)");
        if(bc.indexIn(stylesheet) != -1)
        {
            auto r = bc.cap(1).toInt();
            auto g = bc.cap(2).toInt();
            auto b = bc.cap(3).toInt();
            progress_color = QColor(r, g, b).lighter(125);
            progress_highlight = progress_color.lighter(150);
        }
        else
        {
            auto p = palette();
            progress_color = p.color(QPalette::Active, QPalette::Mid).lighter(75);
            progress_highlight = progress_color.lighter(135);
        }
    }
}

void LandscapeHeadline::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    auto s = geometry();

    if(reporter_draw)
    {
        QRect bounds(margin, margin, s.width() - (margin * 2), s.height() - (margin * 2));
        emit signal_reporter_draw(bounds, painter);
        return;
    }

    QTextDocument td;
    td.setDocumentMargin(margin);

    QRegExp html_tags("<[^>]*>");
    if(html_tags.indexIn(text()) != -1)
        td.setHtml(text());
    else
        td.setPlainText(text());

    if(!compact_mode)
    {
        auto doc_size = td.documentLayout()->documentSize();

        if(shrink_text_to_fit)
        {
            auto f = td.defaultFont();
            for(;;)
            {
                doc_size = td.documentLayout()->documentSize();
                if(doc_size.width() < s.width() && doc_size.height() < s.height())
                    break;

                if((f.pointSizeF() - .1) < 4.0)
                {
                    // let it just clip the remaining text
                    //f.setPointSizeF(original_point_size);
                    //td.setDefaultFont(f);
                    break;
                }

                f.setPointSizeF(f.pointSizeF() - .1);
                td.setDefaultFont(f);
            }
        }

        auto y{0};
        auto x{0};

        if(alignment() & Qt::AlignVCenter)
        {
            y = static_cast<int>((s.height() - doc_size.height()) / 2);
            y = (y < 0) ? 0 : y;
        }
        else if(alignment() & Qt::AlignBottom)
        {
            y = static_cast<int>(s.height() - doc_size.height());
            y = (y < 0) ? 0 : y;
        }

        if(alignment() & Qt::AlignHCenter)
        {
            x = static_cast<int>((s.width() - doc_size.width()) / 2);
            x = (x < 0) ? 0 : x;
        }
        else if(alignment() & Qt::AlignRight)
        {
            x = static_cast<int>(s.width() - doc_size.width());
            x = (x < 0) ? 0 : x;
        }

        painter.save();
        painter.translate(x, y);
        painter.setClipRect(margin, margin, s.width() - margin * 2, s.height() - margin * 2);

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.palette.setColor(QPalette::Text, painter.pen().color());
        td.documentLayout()->draw(&painter, ctx);

        painter.restore();
    }

    if(detect_progress)
    {
        painter.save();

        // see if we can detect any progress indicator in the plain
        // text, and put a progress bar on the headline if so.

        auto plain_text = td.toPlainText();
        QRegExp re(progress_re);
        if(re.indexIn(plain_text) != -1)
        {
            auto percent = re.cap(1).toFloat() / 100.0f;
            if(percent > 1.0f)
                percent = 1.0f;

            // draw a progress bar along the bottom
            if(compact_mode)
            {
                // progress bar consumes the whole window
                progress_h = s.height() - margin * 2 - 1;
                progress_x = margin;
                progress_y = margin;
                progress_w = s.width() - margin * 2 - 1;
            }
            else
            {
                progress_h = 5;
                progress_x = margin;
                progress_y = s.height() - progress_h - margin;
                progress_w = s.width() - margin * 2 - 1;

                if(progress_on_top)
                    progress_y = 5;
            }

            painter.setPen(QPen(progress_color));
            painter.setBrush(QBrush(progress_color));
            painter.drawRect(progress_x, progress_y, progress_w, progress_h);

            painter.setPen(QPen(progress_highlight));
            painter.setBrush(QBrush(progress_highlight));
            painter.drawRect(progress_x, progress_y, static_cast<int>(progress_w * percent), progress_h);
        }

        painter.restore();
    }
}

QSize LandscapeHeadline::minimumSizeHint() const
{
    auto s = QLabel::minimumSizeHint();
    return QSize(s.width(), s.height());
}

QSize LandscapeHeadline::sizeHint() const
{
    auto s = QLabel::sizeHint();
    return QSize(s.width(), s.height());
}

void LandscapeHeadline::initialize(bool stay_visible_, FixedText fixed_text, int width, int height)
{
    Headline::initialize(stay_visible_, fixed_text, width, height);
}

void LandscapeHeadline::prepare_to_zoom_in()
{
    old_detect_progress = detect_progress;
    detect_progress = false;
}

void LandscapeHeadline::zoomed_in()
{
    detect_progress = old_detect_progress;
}

void LandscapeHeadline::prepare_to_zoom_out()
{
    old_detect_progress = detect_progress;
    detect_progress = false;
}

void LandscapeHeadline::zoomed_out()
{
    detect_progress = old_detect_progress;
}

//-----------------------------------------------------------------------

HeadlineGenerator::HeadlineGenerator(int w, int h,
                                     StoryInfoPointer story_info,
                                     const QString& headline,
                                     Qt::Alignment alignment,
                                     QWidget* parent)
{
    if(w < h)
        this->headline = HeadlinePointer(new PortraitHeadline(story_info, headline, alignment, parent));
    else
        this->headline = HeadlinePointer(new LandscapeHeadline(story_info, headline, alignment, parent));
}
