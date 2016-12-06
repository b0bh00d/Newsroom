#pragma once

#include <windows.h>

#include <QWidget>

#include <QtGui/QFont>
//#include <QtGui/QShowEvent>

#include <QtCore/QUrl>

#include "types.h"
#include "specialize.h"

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
      This method is employed by the Chyron class to configure the Headline for display.

      \param font This is the font that the Headline should use when displaying information.
      \param stay_visible Indicates whether or not the widget should be top-most in the Z order
     */
    void    configure(bool stay_visible);    // Chyron

protected:  // data members
    QUrl            story;
    QString         headline;
    QFont           font;
    QString         normal_stylesheet;
    QString         alert_stylesheet;
    QStringList     alert_keywords;

    bool                ignore;     // Chyron
    uint                viewed;     // Chyron
    QPropertyAnimation* animation;  // Chyron

    friend class Chyron;        // the Chyron manages the headlines on the screen
};

SPECIALIZE_SHAREDPTR(Headline, Headline)    // "HeadlinePointer"
