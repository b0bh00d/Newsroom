#pragma once

#include <QtWidgets/QTreeWidgetItem>

#include <QtCore/QVariant>

#include "specialize.h"

class QDomDocument;
class QDomNode;

/// @class Settings
/// @brief Handle application settings in a sane fashion
///
/// Settings is a base class, not intended to be instantiated directly.  It
/// contains only "generic" Qt code, and should be subclassed to implement
/// persistent storage using the backend of your choice (XML, JSON, sqlite,
/// etc.).
///
/// <aside>
/// Why not just use QSettings?  For some reason, QSettings in the INI
/// format was not handling my application data in a predictiable way.
/// Array names were becoming section names, and settings were becoming
/// obscured and inaccessible.  I wrote this class to provide a wrapper
/// around a more structured backend (e.g., XML) for more predictable
/// results, and to have a "living document" that can be updated and
/// changed on the fly while the application runs.
/// </aside>

class Settings
{
public:
    Settings(const QString& application, const QString& filename);

    virtual bool cache()    { return false; }
    virtual bool flush()    { return false; }

    QString     get_error_string()  const { return error_string; }

    void        begin_section(const QString& path);
    void        clear_section(const QString& path);
    void        clear_section();
    void        end_section();
    int         begin_array(const QString& path);
    void        end_array();

    bool        set_array_index(int index);

    QVariant    get_item(const QString& item_name, const QVariant& default_value = QVariant());
    QVariant    get_array_item(int index, const QString &element_name, const QVariant& default_value = QVariant());
    QVariant    get_array_item(const QString& array_name, int index, const QString &element_name, const QVariant& default_value = QVariant());
    void        set_item(const QString& item_name, const QVariant& value);
    void        set_array_item(int index, const QString &element_name, const QVariant& element_value = QVariant());
    void        set_array_item(const QString& array_name, int index, const QString &element_name, const QVariant& element_value = QVariant());

protected:      // typedefs and enums
    typedef QTreeWidgetItem Item;
    SPECIALIZE_SHAREDPTR(Item, Item)                // "ItemPointer"
    SPECIALIZE_MAP(QString, Item*, SectionPath)     // "SectionPathMap"

protected:      // methods
    QString     fix_type_name(const QString& type_name);
    QString     get_current_path(const QStringList& current_path);
    QStringList construct_path(const QString& path = QString());
    Item*       create_path(const QString& path);

    // overridable, format-specific I/O functions (XML, JSON, etc.)
    // the base class does nothing
    virtual     Item*       read_section(void*, Item*, QStringList&) { return nullptr; }
    virtual     Item*       read_array(void*, Item*, QStringList &)  { return nullptr; }
    virtual     Item*       read_element(void*, Item*)               { return nullptr; }
    virtual     Item*       read_item(void*, Item*)                  { return nullptr; }
    virtual     void        write_section(Item*, void*, void*)       {}
    virtual     void        write_array(Item*, void*, void*)         {}
    virtual     void        write_element(Item*, void*, void*)       {}
    virtual     void        write_item(Item*, void*, void*)          {}

protected:      // data members
    ItemPointer     tree_root;
    SectionPathMap  section_path_map;

    QString         application;
    QString         filename;

    QString         error_string;

    QStringList     default_section;
    QStringList     default_array;

    QList<int>      current_array_index;
};

SPECIALIZE_SHAREDPTR(Settings, Settings)        // "SettingsPointer"

/// @class SettingsXML
/// @brief Specialization of Settings for XML backend
///
/// This subclass of Settings implements persistent storage using XML
/// as the backend.

class SettingsXML : public Settings
{
public:
    SettingsXML(const QString& application, const QString& filename);

    bool        cache();
    bool        flush();

protected:      // methods
    // format-specific I/O functions (XML, JSON, etc.)
    Item*       read_section(QDomNode* node, Item* parent, QStringList& current_path);
    Item*       read_array(QDomNode* node, Item* parent, QStringList &current_path);
    Item*       read_element(QDomNode *node, Item* parent);
    Item*       read_item(QDomNode* node, Item* parent);
    void        write_section(Item* section, QDomNode* parent, QDomDocument* doc);
    void        write_array(Item* array, QDomNode* parent, QDomDocument* doc);
    void        write_element(Item* element, QDomNode* parent, QDomDocument* doc);
    void        write_item(Item* item, QDomNode* parent, QDomDocument* doc);
};
