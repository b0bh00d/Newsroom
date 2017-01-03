#pragma once

#include <QWidget>

#include <QtGui/QFont>

#include <QtCore/QUrl>
#include <QtCore/QTimer>

#include "types.h"
#include "specialize.h"

class QLabel;
class QPropertyAnimation;

/// @class Headline
/// @brief Contains data submitted by a Reporter
///
/// The Headline class contains the headline submitted by a Reporter watching
/// a specific story.  It inherits from QWidget, so it is the visible element
/// on the screen, but its visibility and life cycle are managed by the Chyron.

class Headline : public QWidget
{
    Q_OBJECT
public:
    explicit Headline(const QUrl& story,
                      const QString& headline,
                      AnimEntryType entry_type = AnimEntryType::PopCenter,
                      Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                      QWidget* parent = nullptr);
    explicit Headline(const Headline& source)
    {
        story = source.story;
        headline = source.headline;
        entry_type = source.entry_type;
    }
    ~Headline();

    void    set_font(const QFont& font)                         { this->font = font; }
    void    set_stylesheet(const QString& stylesheet)           { this->stylesheet = stylesheet; }
    void    set_progress(bool include_progress, const QString& re, bool on_top);
    void    set_compact_mode(bool compact, int original_width, int original_height);

signals:
    void    signal_mouse_enter();
    void    signal_mouse_exit();

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
      This method is employed by the Chyron class to perform configuration of
      the Headline for display.  Dependending upon the value of 'fixed_text',
      the Headline will initialize differently.

      \param stay_visible Indicates whether or not the widget should be top-most in the Z order
      \param fixed_text Selects how to handle text that exceeds the size of the fixed sizes.  If this is FixedText::None, then default initialization is performed.
      \param width The fixed width the Headline will use.
      \param height The fixed height the Headline will use.
     */
    void    initialize(bool stay_visible, FixedText fixed_text = FixedText::None, int width = 0, int height = 0);      // Chyron, LaneManager

    /*!
      In "compact mode", this method is called to restore the Headline to its
      compacted size.
     */
    void    zoom_out();

protected:  // data members
    bool            stay_visible;
    bool            was_stay_visible;
    bool            mouse_in_widget;
    bool            is_zoomed;
    int             margin;
    QUrl            story;
    QString         headline;
    QFont           font;
    QString         stylesheet;

    bool            compact_mode;
    int             original_w;
    int             original_h;

    bool                ignore;     // Chyron
    uint                viewed;     // Chyron
    QPropertyAnimation* animation;  // Chyron

    QLabel*         label;

    double          old_opacity;

    AnimEntryType   entry_type;

    Qt::Alignment   alignment;

    bool            include_progress_bar;
    QString         progress_text_re;
    bool            progress_on_top;

    QTimer*         hover_timer;
    QRect           georect;

    // in the '!stay_visible' case, because of the way the nativeEvent()
    // function works, a new window will end up displaying BENEATH the
    // topmost window.  this can cause visual artifacting in Dashboard
    // types if opacity is not 100% due to Headling stacking.  the Chyron
    // will set this 'bottom_window' value directly to be the topmost
    // Headline if it is configured for Dashboard, so the nativeEvent()
    // function will use it to place the new window precisely at the top
    // of the Dashboard Z-order.

    QWidget*        bottom_window;  // Chyron

    friend class Chyron;        // the Chyron manages the Headlines on the screen
    friend class LaneManager;   // needs to initialize() it's Dashboard Headline banner
};

SPECIALIZE_SHAREDPTR(Headline, Headline)    // "HeadlinePointer"
