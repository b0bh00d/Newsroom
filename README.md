# Newsroom
A C++ Qt-based desktop application for real-time activity monitoring

## Summary
Newsroom is a Qt-based application designed for the desktop that uses a full
newsroom paradigm--Stories, Reporters, Producers, Chyrons, Headlines, etc.--to
construct a system for performing real-time monitoring of activities.
Reporters cover "beats" (areas of their particular expertise) and file reports
with Producers who then generate Headlines based on established formatting
policies for Chyrons to display on the desktop.

Reporters are defined as plug-ins.  Newsroom comes out of the box with
Reporters designed to cover two "beats": A local text file (e.g., log), and
a TeamCity v9 REST API.

## Qt versions
This projects was developed with, and tested under, Qt versions 5.4.2 and
5.6.2.

## OpenSSL
Newsroom itself does not have a dependency upon OpenSSL, however, the
TeamCity9 Reporter plug-in will likely not function unless those libraries
are available and Qt has been configured and built against them.

Future Reporters may have similar dependencies external to the main application
in order to be useable.

## Feature set
Everything is based around a Story.  All other elements--Reporters, Producers,
Chyrons, etc.--exist to "cover" a Story.  You need a Reporter who understands
the Story's "beat" in order to cover it.  Local text files and REST API
Reporters are included; others can be created.

Stories can be grouped into Series.  A Series is a collection of related
Stories (think: TeamCity builds).  Series have no affect on the display of
a Story's Headlines.  They are purely an organizing tool.

Reporters only know how to file a text report about some recent activity
on its assigned "beat".  The Producer knows how to take that filed report
and inject it into the system so the Chyron can display it.

The responsibility of displaying Headlines is relegated to Chyrons.  Chyrons
can display Headlines in one of many different "entry" and "exit" types,
almost all of which involve motion and/or fading animation.

Headlines displayed by the Chyron can be locked to the top of the desktop
Z-order so they are always visible.  Additionally, under Windows, Headlines
can also be "glued" to the desktop, so they are always at the bottom of the
visible Z-order and will not raise, even if explicitly clicked on by the user.

A particular kind of Chyron "entry" type (which is really more of a grouping
type) is available, called a "Dashboard".  This "entry" type will group one
or more Chyron displays into a common group id, where each entry monitors a
single activity.  This type is handy for employing the TeamCity9 Reporter for
monitoring multiple builders and their builds.  "Dashboard" types can also be
"compacted" to provide thousand-foot views of monitored activities.

Most GUI items have descriptive tooltips to help you understand their function.

### Current status
The system is currently functional, but should be considered a work-in-progress
while I continue to tinker with it.  There are some approaches I've taken with
which I am not completely satisfied, and there are some additions I am planning
to make (e.g., I don't like the available "entry" types for use with incremental
log monitoring).

In other words, don't be surprised if I change something later that might end up
being incompatible with your established Stories.

Some things are not yet hooked up.  For example, auto-start does not yet work.

I've not tweaked this for, or even compiled under, operating systems other than
Windows so far.  All development has taken place there to this point.

## Usage
Run the main application, and then drop either a local text file, or a Web URL
(assumed to be a REST API node), onto the main window.  This will launch the
"Add Story" dialog where you can configure the Reporter and other details about
the Headlines that are generated for this Story.

Newsroom comes with one defined "Default" stylesheet.  This is the stylesheet
that is used for all Headlines displayed on the screen.  You may modify this
stylesheet, but you cannot delete, or place it in a lower-priority order.  You
may add new styles, each of which should have "trigger" keywords associated
which will select them if any of those keywords appear in the Headline text.
Trigger keywords are separated by commas, and are case-insensitive.

For example, for TeamCity Headlines, you might have a stylesheet defined that
"triggers" if the one of the keywords "error, fail" appear in the report.  Or
perhaps one to be used if the word "hung" appears (indicating that a builder
might be hung).  Styles are in priority order in the list, with "Default"
having the highest position.  If you have one with a "finish" trigger ahead of
one with "error", and both keywords appear in the same Headline text, the
stylesheet for "finish" will be used and not "error".  Plan your styles and
their relationships to one another carefully.

Series can be created by double-clicking on the "Default" series to rename it.
There is always a "Default" Series to capture all newly added Stories, so
renaming it will cause a new "Default" entry to appear.  Stories can be dragged
and dropped between Series, but each Story must belong to a Series (with new
Stories automatically added to "Default").

Story positions within Series, and Series order relative to other Series, is
important when starting Story coverage.  Stories will begin coverage--and
therefore, screen presence and position--based on the relative ordering within
the Series tree, top to bottom.  Depending upon how Chyrons are stacked by the
internal LaneManager, you may need to re-order Story positions to achieve the
desired layout based on the assigned "entry" type.

## Examples

The following are some visual examples of the Newroom program in action.
Because the included TextFile Reporter isn't every interesting beyond its
possible Entry and Exit types, I'm only including screen shots of the TeamCity9
and YahooChartAPI Reporters included with the program.

### TeamCity9

This screen shot shows multiple TeamCity9 Reporters covering a number of
build configurations using one of the Dashboard Entry types, with each
related build configuration grouped within a single Dashboard group.  The
various Headline coloring corresponds to defined stylesheets associated with
"trigger" keywords in the Headline text ("Changes", "Success", etc.):

![snap1](https://cloud.githubusercontent.com/assets/4536448/22610013/ac5e719a-ea20-11e6-9724-899d53ffe75a.png)

This screen shot shows 32 TeamCity build configurations across three different
TeamCity servers being monitored by TeamCity9 Reporters in the "compact"
Dashboard mode.  Note that, if the "detect progress" option is enabled, a
progress bar appears on the compacted Headline:

![snap2](https://cloud.githubusercontent.com/assets/4536448/22610018/afad370a-ea20-11e6-91e7-f89a33499101.png)

Hovering the mouse pointer over any one of the "compacted" Headlines causes it
to expand to full size for viewing.  Leaving the Headline returns it to its
compacted size.

### YahooChartAPI

The newest Reporter to Newsroom is the YahooChartAPI Reporter.  When activated
with the Yahoo Chart API URL (http://chartapi.finance.yahoo.com/instrument/1.0),
it uses the API to provide minute-by-minute updates on specified New York Stock
Exchange stock symbols.  The default symbol is "^dji", which provides Dow Jones
Industrial Average information, but you can specify a stock symbol of your
choice.

This screen shot shows the "^dji" (Dow Jones Industrial Average) and "msft"
(Microsoft) symbols displayed using the default, text-based Headline display
after the market has closed:

![snap3](https://cloud.githubusercontent.com/assets/4536448/22610019/b0faf69c-ea20-11e6-9482-63a411d6b223.png)

In its parameters, the YahooChartAPI Reporter gives you the option to enable
chart drawing alongside text information (the grey line is the "open" point of
the chart's day):

![snap4](https://cloud.githubusercontent.com/assets/4536448/22610023/b233fc84-ea20-11e6-9fa5-9604e3a8cced.png)

(As an aside, this custom drawing on a Headline is enabled by the new
ReporterDraw() functionality of the IReporter2 interface, which allows the
Reporter to completely take over responsibility for the Headline display.)

When the market is closed, the chart mode will also indicate this:

![snap5](https://cloud.githubusercontent.com/assets/4536448/22610025/b35490ce-ea20-11e6-9bde-de3dee8e120e.png)

## Documentation
You're reading it now.
