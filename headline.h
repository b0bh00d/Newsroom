#pragma once

#include <QtWidgets/QLabel>

#include <QtGui/QPaintEvent>
#include <QtGui/QFont>

#include <QtCore/QUrl>
#include <QtCore/QTimer>

#include "types.h"
#include "specialize.h"
#include "storyinfo.h"

class QPropertyAnimation;

/// @class Headline
/// @brief Contains data submitted by a Reporter
///
/// The Headline class contains the headline submitted by a Reporter watching
/// a specific story.  It inherits from QLabel, so it is the visible element
/// on the screen, but its visibility and life cycle are managed by the Chyron.
///
/// This is a base class that provides common behaviors for all subclasses.

class Headline : public QLabel
{
    Q_OBJECT
public:
    explicit Headline(StoryInfoPointer story_info,
                      const QString& headline,
                      Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                      QWidget* parent = nullptr);
    explicit Headline(const Headline& source)
    {
        story_info = source.story_info;
        headline = source.headline;
    }
    virtual ~Headline();

    void    set_shrink_to_fit(bool shrink = true)       { shrink_text_to_fit = shrink; }
    void    set_compact_mode(bool compact, int original_width, int original_height);
    void    set_margin(int margin = 5)                  { this->margin = margin; }

    void    set_font(const QFont& font)                 { this->font = font; }
    void    set_stylesheet(const QString& stylesheet)   { this->stylesheet = stylesheet; }

    void    set_reporter_draw(bool reporterdraw = true) { reporter_draw = reporterdraw; }

signals:
    void    signal_mouse_enter();
    void    signal_mouse_exit();
    void    signal_reporter_draw(const QRect& bounds, QPainter& painter);

protected slots:
    void    slot_zoom_in();
    void    slot_turn_off_compact_mode();
    void    slot_turn_on_compact_mode();

protected:  // methods
    /*!
      The Qt nativeEvent() is used to "glue" headlines onto the desktop when the
      'stay_visible' flag is false.

      This is currently only implemented under Windows.  It has no effect under
      other platforms.
     */
    bool    nativeEvent(const QByteArray &eventType, void *message, long *result);

    /*!
      The mouse is tracked in Headlines for various effects.  If "compact mode"
      is enabled (Dashboard), then the Headline will be "zoomed in" to full size
      so it may be read.  If not, a Headline may be brought back to full opacity
      if it has been "dimmed".
      @{
     */
    void    enterEvent(QEvent *event);
    void    leaveEvent(QEvent *event);
    /*!
      @}
     */

    /*!
      This method is employed by the Chyron and LaneManager classes to perform
      configuration of the Headline for display.  Dependending upon the value
      of 'fixed_text', the Headline will initialize differently.

      \param stay_visible Indicates whether or not the widget should be top-most in the Z order
      \param fixed_text Selects how to handle text that exceeds the size of the fixed sizes.  If this is FixedText::None, then default initialization is performed.
      \param width The fixed width the Headline will use.
      \param height The fixed height the Headline will use.
     */
    virtual void initialize(bool stay_visible, FixedText fixed_text = FixedText::None, int width = 0, int height = 0);      // Chyron, LaneManager

    /*!
      In "compact mode", this method is called to restore the Headline to its
      compacted size after animation is complete.
     */
    void zoom_out();

    /*!
      These virtual functions are for subclasses to override in case they need to
      make adjustments before and after the 'zooming' effect in Dashboard 'compact
      mode'.  The base class does nothing.
     @{
     */
    virtual void prepare_to_zoom_in()   {}
    virtual void zoomed_in()            {}
    virtual void prepare_to_zoom_out()  {}
    virtual void zoomed_out()           {}
    /*!
      @}
     */

protected:  // data members
    bool            stay_visible;
    bool            was_stay_visible;
    bool            mouse_in_widget;
    bool            is_zoomed;
    int             margin;
    StoryInfoPointer story_info;
    QString         headline;
    QFont           font;
    QString         stylesheet;

    bool            shrink_text_to_fit;
    bool            compact_mode;
    bool            reporter_draw;
    int             original_w;
    int             original_h;

    bool            ignore;         // Chyron
    uint            viewed;         // Chyron
    QPropertyAnimation* animation;  // Chyron

    QLabel*         label;

    bool            include_progress_bar;
    QString         progress_text_re;
    bool            progress_on_top;

    QRect           starting_geometry, target_geometry;
    QTimer*         hover_timer;

    qreal           old_opacity;

    // in the '!stay_visible' case, because of the way the nativeEvent()
    // function works, a new window will end up displaying BENEATH the
    // topmost window.  this can cause visual artifacting in Dashboard
    // types if opacity is not 100% due to Headling stacking.  the Chyron
    // will set this 'bottom_window' value directly to be the topmost
    // Headline if it is configured for Dashboard, so the nativeEvent()
    // function will use it to place the new window precisely at the top
    // of the Dashboard Z-order.

    QWidget*        bottom_window;  // Chyron

    friend class Chyron;        // manages the Headline's life cycle and appearance
    friend class Dashboard;     // needs to access initialize() for its Dashboard Headline banner
};

class PortraitHeadline : public Headline
{
    Q_OBJECT
public:
    explicit PortraitHeadline(StoryInfoPointer story_info,
                              const QString& headline,
                              Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                              bool configure_for_left = true,
                              QWidget *parent = nullptr);
    ~PortraitHeadline();

    /*!
      By definition, the PortraitHeadline has a width that is smaller than its
      height.  Therefore, text displayed within it is rotated vertically.  This
      method sets an internal value that indicates how the text should be rotated
      for display.  If 'left' (meaning the Headline will be aligned along the left
      side of the screen), then the text is rotated such that English would flow
      from the bottom to the top.  Otherwise, the assumption is that the Headline
      will be right-side aligned, and text will be rotated such that English would
      flow from the top to the bottom.

      @param for_left A Boolean indicating a left-side screen alignment if true, otherwise a right-side alignment will be used.
     */
    void    set_for_left(bool for_left = true) { configure_for_left = for_left; }

protected:      // methods
    void    paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    QSize   sizeHint() const Q_DECL_OVERRIDE;
    QSize   minimumSizeHint() const Q_DECL_OVERRIDE;

    virtual void initialize(bool stay_visible, FixedText fixed_text = FixedText::None, int width = 0, int height = 0);      // Chyron, LaneManager

protected:      // data members
    bool    configure_for_left;
};

class LandscapeHeadline : public Headline
{
    Q_OBJECT
public:
    explicit LandscapeHeadline(StoryInfoPointer story_info,
                               const QString& headline,
                               Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                               QWidget *parent = nullptr);
    ~LandscapeHeadline();

protected:      // methods
    void    changeEvent(QEvent* event) Q_DECL_OVERRIDE;
    void    paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    QSize   sizeHint() const  Q_DECL_OVERRIDE;
    QSize   minimumSizeHint() const  Q_DECL_OVERRIDE;

    void initialize(bool stay_visible, FixedText fixed_text = FixedText::None, int width = 0, int height = 0);      // Chyron, LaneManager

    /*!
      We override the Headline 'zoom' hooks so we can turn off progress detection
      during the zooming animation, and then turn it back on after the animation
      is complete.  This provides for a cleaner looking animation.
     @{
     */
    void prepare_to_zoom_in();
    void zoomed_in();
    void prepare_to_zoom_out();
    void zoomed_out();
    /*!
      @}
     */

    /*!
      The LandscapeHeadline can be configured to detect a progress indicator within
      the Headline text.  This indicator is usually a numeric value followed by a
      percent sign (%), but the user can configure whatever regular expression they
      wish to retrieve a numeric value for this purpose.

      If detected, the LandscapeHeadline will include a progress bar somewhere on
      the Headline itself.  The user can place the progress bar on the top or the
      bottom, or if 'compact mode' is active, the entire Headline window itself will
      display the progress bar.

      @param re The regular expression to use in attempts to detect progress indicator within the Headline text.
      @param on_top A Boolean indicating the position of the progress bar: true places it on the top of the Headline, false on the bottom.
     */
    void    enable_progress_detection(const QString& re, bool on_top);

protected:      // data members
    bool    detect_progress, old_detect_progress;
    QString progress_re;
    bool    progress_on_top;

    int     progress_x;
    int     progress_y;
    int     progress_w;
    int     progress_h;

    QColor  progress_color, progress_highlight;
};

SPECIALIZE_SHAREDPTR(Headline, Headline)    // "HeadlinePointer"

/// @class HeadlineGenerator
/// @brief Helper class for generating the correct Headline subclass
///
/// HeadlineGenerator is a simple helper class that generates the
/// correct Headline subclass for the dimensions provided.  It is
/// intended to be used on the stack.

class HeadlineGenerator
{
public:
    explicit HeadlineGenerator(int w, int h,
                               StoryInfoPointer story_info,
                               const QString& headline,
                               Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                               QWidget* parent = nullptr);

    HeadlinePointer get_headline()  const   { return headline; }

protected:  // data members
    HeadlinePointer headline;
};
