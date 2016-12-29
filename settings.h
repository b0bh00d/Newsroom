#pragma once

#include <QtWidgets/QTreeWidgetItem>

#include <QtCore/QVariant>

#include "specialize.h"

class QDomDocument;
class QDomNode;

/// @class Settings
/// @brief Handle application settings in a sane fashion
///
/// For some reason, QSettings in the INI format was not handling my
/// application data in a predictiable way.  Array names were becoming
/// section names, and settings were becoming obscured and inaccessible.
///
/// I wrote this class to provide a wrapper around a more structured backend
/// (e.g., XML) and to have a "living document" that can be updated and changed
/// on the fly while the application runs.

class Settings
{
public:
    Settings(const QString& application, const QString& filename);

    bool        cache();
    bool        flush();

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

    // format-specific I/O functions
    Item*       read_section(QDomNode* node, Item* parent, QStringList& current_path);
    Item*       read_array(QDomNode* node, Item* parent, QStringList &current_path);
    Item*       read_element(QDomNode *node, Item* parent);
    Item*       read_item(QDomNode* node, Item* parent);
    void        write_section(Item* section, QDomNode* parent, QDomDocument* doc);
    void        write_array(Item* array, QDomNode* parent, QDomDocument* doc);
    void        write_element(Item* element, QDomNode* parent, QDomDocument* doc);
    void        write_item(Item* item, QDomNode* parent, QDomDocument* doc);

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
