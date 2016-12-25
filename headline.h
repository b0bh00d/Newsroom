#pragma once

//#include <windows.h>

#include <QWidget>

#include <QtGui/QFont>

#include <QtCore/QUrl>

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

    void    set_font(const QFont& font)                         { this->font = font; }
    void    set_stylesheet(const QString& stylesheet)           { this->stylesheet = stylesheet; }

signals:
    void    signal_mouse_enter();
    void    signal_mouse_exit();

protected:  // methods
//    void    showEvent(QShowEvent *event);
//    bool winEvent(MSG* message, long* result);
    bool    nativeEvent(const QByteArray &eventType, void *message, long *result);
    void    enterEvent(QEvent *event);
    void    leaveEvent(QEvent *event);

    /*!
      This method is employed by the Chyron class to perform configuration of
      the Headline for display.  Dependending upon the value of 'fixed_text',
      the Headline will initialize differently.

      \param stay_visible Indicates whether or not the widget should be top-most in the Z order
      \param fixed_text Selects how to handle text that exceeds the size of the fixed sizes.  If this is FixedText::None, then default initialization is performed.
      \param width The fixed width the Headline will use.
      \param height The fixed height the Headline will use.
     */
    void    initialize(bool stay_visible, FixedText fixed_text = FixedText::None, int width = 0, int height = 0);      // Chyron

protected:  // data members
    bool            stay_visible;
    int             margin;
    QUrl            story;
    QString         headline;
    QFont           font;
    QString         stylesheet;

    bool                ignore;     // Chyron
    uint                viewed;     // Chyron
    QPropertyAnimation* animation;  // Chyron

    QLabel*         label;

    double          old_opacity;

    AnimEntryType   entry_type;

    Qt::Alignment   alignment;

    friend class Chyron;        // the Chyron manages the headlines on the screen
    friend class LaneManager;   // needs to initialize() it's Dashboard headline banner
};

SPECIALIZE_SHAREDPTR(Headline, Headline)    // "HeadlinePointer"
