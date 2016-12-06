#pragma once

#include <windows.h>

#include <QWidget>

#include <QtGui/QFont>
//#include <QtGui/QShowEvent>

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
    explicit Headline(const QUrl& story, const QString& headline, QWidget* parent = nullptr);
    explicit Headline(const Headline& source)
    {
        story = source.story;
        headline = source.headline;
    }

    void    set_font(const QFont& font)                         { this->font = font; }
    void    set_normal_stylesheet(const QString& stylesheet)    { normal_stylesheet = stylesheet; }
    void    set_alert_stylesheet(const QString& stylesheet)     { alert_stylesheet = stylesheet; }
    void    set_alert_keywords(const QStringList& keywords)     { alert_keywords = keywords; }

protected:  // methods
//    void    showEvent(QShowEvent *event);
//    bool winEvent(MSG* message, long* result);

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
    int             margin;
    QUrl            story;
    QString         headline;
    QFont           font;
    QString         normal_stylesheet;
    QString         alert_stylesheet;
    QStringList     alert_keywords;

    bool                ignore;     // Chyron
    uint                viewed;     // Chyron
    QPropertyAnimation* animation;  // Chyron

    QLabel*         label;

    friend class Chyron;        // the Chyron manages the headlines on the screen
};

SPECIALIZE_SHAREDPTR(Headline, Headline)    // "HeadlinePointer"
