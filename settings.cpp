#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include "settings.h"

// https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html

const int ItemType = 0;

Settings::Settings(const QString& application, const QString &filename)
    : application(application),
      filename(filename)
{
}

QString Settings::fix_type_name(const QString& type_name)
{
    QString name = type_name.toLower();
    if(name.startsWith("q"))
        name.remove(0, 1);
    return name;
}

QString Settings::get_current_path(const QStringList& current_path)
{
    QString path = current_path.join("/");
    while(path.contains("//"))
        path.replace("//", "/");
    return path;
}

QStringList Settings::construct_path(const QString& path)
{
    QString section_path;

    if(!default_section.isEmpty())
        section_path = default_section.join("/");
    if(!default_array.isEmpty())
    {
        if(!section_path.isEmpty())
            section_path = QString("%1/%2").arg(section_path).arg(default_array.join("/"));
        else
            section_path = default_array.join("/");
    }

    if(section_path.isEmpty())
        section_path = "/General";

    if(!path.isEmpty())
        section_path = QString("%1/%2").arg(section_path).arg(path);

    section_path.replace("//", "/");

    QStringList elements = section_path.split("/");
    QString target_name = elements.back();
    elements.pop_back();
    QStringList result;
    result << elements.join("/") << target_name;

    return result;
}

Settings::Item* Settings::create_path(const QString& path)
{
    Item* section = nullptr;

    if(section_path_map.contains(path))
        section = section_path_map[path];

    if(!section)
    {
        QStringList elements = path.split("/");
        while(elements[0].isEmpty())
            elements.pop_front();

        QString path = "/";
        Item* parent = tree_root.data();

        foreach(const QString& element, elements)
        {
            if(!path.endsWith("/"))
                path += "/";
            path += element;
            if(section_path_map.contains(path))
                parent = section_path_map[path];    // already exists
            else
            {
                Item* item = new Item(parent, QStringList() << "Section" << element);
                section_path_map[path] = item;
                parent = item;
            }
        }

        section = parent;
    }

    return section;
}

bool Settings::cache()
{
    if(!tree_root.isNull())
    {
        error_string = QObject::tr("Settings has already been initialized.");
        return false;
    }

    tree_root = ItemPointer(new Item(ItemType));

    if(!QFile::exists(filename))
    {
        // create the default section
//        Item* general = new Item(tree_root.data(), QStringList() << "Section");
//        general->setText(0, "General");
//        section_path_map["/General"] = general;

        return true;
    }

    QDomDocument note_database(application);
    QDomElement root = note_database.createElement(application);
    root.setAttribute("version", "1.0");

    note_database.appendChild(root);

    QDomNode node(note_database.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF8\""));
    note_database.insertBefore(node, note_database.firstChild());

    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        error_string = QObject::tr("The specified Settings file \"%1\" could not be opened.").arg(filename);
        return false;
    }

    QString errorStr, versionStr = "1.0";
    int errorLine;
    int errorColumn;

    if(!note_database.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
    {
        error_string = QObject::tr("The contents of the Settings file \"%1\" could not be read:\n%2:%3:%2").arg(filename).arg(errorLine).arg(errorColumn).arg(errorStr);
        return false;
    }

    root = note_database.documentElement();
    if(root.tagName().compare(application))
    {
        error_string = QObject::tr("The Settings file \"%1\" is not for this application (\"%2\").").arg(filename).arg(application);
        return false;
    }
    else if(root.hasAttribute("version"))
        versionStr = root.attribute("version");

    QStringList current_path;
    current_path << "/";

    QDomNodeList children = root.childNodes();
    for(int i = 0;i < children.length();i++)
    {
        QDomNode node = children.at(i);
        if(!node.nodeName().compare("Section"))
            // top-level items should always and only be Sections
            (void)read_section(&node, tree_root.data(), current_path);
    }

    return true;
}

Settings::Item* Settings::read_section(QDomNode* node, Settings::Item* parent, QStringList &current_path)
{
    QDomElement element = node->toElement();
    QString name = element.attribute("name");
    Item* section = new Item(parent, QStringList() << "Section" << name);

    current_path.append(name);
    section_path_map[get_current_path(current_path)] = section;

    QDomNodeList children = node->childNodes();
    for(int i = 0;i < children.length();i++)
    {
        QDomNode child_node = children.at(i);
        if(!child_node.nodeName().compare("Section"))
            (void)read_section(&child_node, section, current_path);
        else if(!child_node.nodeName().compare("Array"))
            (void)read_array(&child_node, section, current_path);
        else if(!child_node.nodeName().compare("Item"))
            (void)read_item(&child_node, section);
        else
        {
            // any other type is an error!
        }
    }

    current_path.pop_back();

    return section;
}

Settings::Item* Settings::read_array(QDomNode *node, Settings::Item* parent, QStringList &current_path)
{
    QDomElement element = node->toElement();
    QString name = element.attribute("name");
    Item* array = new Item(parent, QStringList() << "Array" << name);

    current_path.append(name);
    section_path_map[get_current_path(current_path)] = array;

    QDomNodeList children = node->childNodes();
    for(int i = 0;i < children.length();i++)
    {
        QDomNode child_node = children.at(i);
        if(!child_node.nodeName().compare("Array"))
            (void)read_array(&child_node, array, current_path);
        else if(!child_node.nodeName().compare("Element"))
            (void)read_element(&child_node, array);
        else
        {
            // any other type is an error!
        }
    }

    current_path.pop_back();

    return array;
}

Settings::Item* Settings::read_element(QDomNode *node, Settings::Item* parent)
{
    QDomElement element_node = node->toElement();
    Item* element = new Item(parent, QStringList() << "Element" << element_node.attribute("name"));

    QDomNodeList children = node->childNodes();
    for(int i = 0;i < children.length();i++)
    {
        QDomNode child_node = children.at(i);
        if(!child_node.nodeName().compare("Item"))
            (void)read_item(&child_node, element);
        else
        {
            // any other type is an error!
        }
    }

    return element;
}

Settings::Item* Settings::read_item(QDomNode *node, Settings::Item* parent)
{
    QDomElement element = node->toElement();
    Item* item = new Item(parent, QStringList() << "Item" << element.attribute("name"));

    QString type = element.attribute("type");
    if(!type.compare("stringlist"))
    {
        QStringList sl;
        QDomNodeList children = node->childNodes();
        for(int i = 0;i < children.length();++i)
        {
            QDomNode child_node = children.at(i);
            QDomNodeList sub_children = child_node.childNodes();
            for(int j = 0;j < sub_children.length();++j)
            {
                QDomNode sub_child_node = sub_children.at(j);
                QDomCDATASection cdata_node = sub_child_node.toCDATASection();
                if(!cdata_node.isNull())
                    sl << cdata_node.data();
            }
        }

        QVariant v(sl);
        item->setData(0, Qt::UserRole, v);
    }
    else
    {
        QString data;

        QDomNodeList children = node->childNodes();
        for(int i = 0;i < children.length() && data.isEmpty();++i)
        {
            QDomNode child_node = children.at(i);
            QDomCDATASection cdata_node = child_node.toCDATASection();
            if(!cdata_node.isNull())
                data = cdata_node.data();
            else
            {
                QDomText text_node = child_node.toText();
                if(!text_node.isNull())
                    data = text_node.data();
            }

            if(!data.isEmpty())
            {
                if(!type.compare("bytearray"))
                {
                    QByteArray ba = QByteArray::fromHex(data.toUtf8());
                    QVariant v(ba);
                    item->setData(0, Qt::UserRole, v);
                }
                else
                {
                    QVariant v(data);
                    item->setData(0, Qt::UserRole, v);
                }

                break;
            }
        }
    }

    return item;
}

const int IndentSize = 2;

bool Settings::flush()
{
    if(tree_root.isNull())
    {
        error_string = QObject::tr("Settings has not been initialized.");
        return false;
    }

    QDomDocument settings(application);
    QDomElement root = settings.createElement(application);
    root.setAttribute("version", "1.0");

    settings.appendChild(root);

    QDomNode node(settings.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF8\""));
    settings.insertBefore(node, settings.firstChild());

    for(int i = 0;i < tree_root->childCount();++i)
        write_section(tree_root->child(i), &root, &settings);

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out;
    out.setDevice(&file);
    out.setCodec("UTF-8");

    settings.save(out, IndentSize);

    bool result = (file.error() != QFileDevice::NoError);
    file.close();
    return result;
}

void Settings::write_section(Item* section, QDomNode* parent, QDomDocument* doc)
{
    QDomElement child = doc->createElement("Section");
    child.setAttribute("name", section->text(1));
    parent->appendChild(child);

    for(int i = 0;i < section->childCount();++i)
    {
        Item* item = section->child(i);
        if(!item->text(0).compare("Section"))
            write_section(item, &child, doc);
        else if(!item->text(0).compare("Array"))
            write_array(item, &child, doc);
        else if(!item->text(0).compare("Item"))
            write_item(section->child(i), &child, doc);
        else
        {
            // any other type is an error!
        }
    }
}

void Settings::write_array(Item* array, QDomNode* parent, QDomDocument* doc)
{
    QDomElement child_array = doc->createElement("Array");
    child_array.setAttribute("name", array->text(1));
    parent->appendChild(child_array);

    for(int i = 0;i < array->childCount();++i)
    {
        Item* item = array->child(i);
        if(!item->text(0).compare("Array"))
            write_array(item, &child_array, doc);
        else if(!item->text(0).compare("Element"))
            write_element(item, &child_array, doc);
        else
        {
            // any other type is an error!
        }
    }
}

void Settings::write_element(Item* element, QDomNode* parent, QDomDocument* doc)
{
    QDomElement child = doc->createElement("Element");
    child.setAttribute("name", element->text(1));
    parent->appendChild(child);

    for(int i = 0;i < element->childCount();++i)
    {
        Item* item = element->child(i);
        if(!item->text(0).compare("Item"))
            write_item(item, &child, doc);
        else
        {
            // any other type is an error!
        }
    }
}

void Settings::write_item(Item* item, QDomNode* parent, QDomDocument* doc)
{
    QDomElement child = doc->createElement("Item");
    child.setAttribute("name", item->text(1));
    QString type = item->text(2);
    child.setAttribute("type", type);

    if(!type.compare("stringlist"))
    {
        // create an array-like list where each element is a string
        // in the list.  that way, we don't have to guard against
        // strings containing the separate character.

        QStringList data = item->data(0, Qt::UserRole).toStringList();
        foreach(const QString& str, data)
        {
            QDomElement element = doc->createElement("Element");
            QDomCDATASection element_text = doc->createCDATASection(str);
            element.appendChild(element_text);
            child.appendChild(element);
        }

        parent->appendChild(child);
    }
    else
    {
        QString data;
        if(!type.compare("bytearray"))
            data = item->data(0, Qt::UserRole).toByteArray().toHex();
        else
            data = item->data(0, Qt::UserRole).toString();

        QDomCDATASection child_text = doc->createCDATASection(data);
        child.appendChild(child_text);
        parent->appendChild(child);
    }
}

void Settings::begin_section(const QString& path)
{
    default_section.append(path);
}

void Settings::clear_section(const QString& path)
{
    QStringList locator = construct_path(path);
    if(!locator.isEmpty())
    {
        QString section_path = locator.join("/");
        section_path.replace("//", "/");

        Item* section = nullptr;
        if(section_path_map.contains(section_path))
            section = section_path_map[section_path];

        if(section)
        {
            // anything in the section_path_map[] that starts with
            // 'section_path' must be removed

            QStringList bad_keys;
            foreach(const QString& key, section_path_map.keys())
            {
                if(key.startsWith(section_path) && key.length() > section_path.length())
                    bad_keys << key;
            }

            foreach(const QString& key, bad_keys)
                section_path_map.remove(key);

            while(section->childCount())
                delete section->takeChild(0);
        }
    }
}

void Settings::clear_section()
{
    QStringList locator = construct_path();
    if(!locator.isEmpty())
    {
        QString section_path = locator.join("/");
        section_path.replace("//", "/");

        Item* section = nullptr;
        if(section_path_map.contains(section_path))
            section = section_path_map[section_path];

        if(section)
        {
            // anything in the section_path_map[] that starts with
            // 'section_path' must be removed

            QStringList bad_keys;
            foreach(const QString& key, section_path_map.keys())
            {
                if(key.startsWith(section_path) && key.length() > section_path.length())
                    bad_keys << key;
            }

            foreach(const QString& key, bad_keys)
                section_path_map.remove(key);

            while(section->childCount())
                delete section->takeChild(0);
        }
    }
}

void Settings::end_section()
{
    default_section.pop_back();
}

int Settings::begin_array(const QString& path)
{
    default_array.append(path);
    QStringList locator = construct_path();
    QString section_path = locator.join("/");
    section_path.replace("//", "/");

    Item* array = nullptr;
    if(section_path_map.contains(section_path))
        array = section_path_map[section_path];

    if(!array)
        return 0;

    return array->childCount();
}

void Settings::end_array()
{
    default_array.pop_back();
}

QVariant Settings::get_item(const QString& item_name, const QVariant& default_value)
{
    QStringList locator = construct_path(item_name);

    Item* section = nullptr;

    if(section_path_map.contains(locator[0]))
        section = section_path_map[locator[0]];

    if(section)
    {
        for(int i = 0;i < section->childCount(); ++i)
        {
            Item* child = section->child(i);
            if(!child->text(1).compare(locator[1]))
                return child->data(0, Qt::UserRole);
        }
    }

    return default_value;
}

QVariant Settings::get_array_item(int index, const QString &element_name, const QVariant& default_value)
{
    QStringList locator = construct_path();
    if(locator.isEmpty())
        return default_value;
    QString section_path = locator.join("/");
    section_path.replace("//", "/");

    Item* array = nullptr;

    if(section_path_map.contains(section_path))
        array = section_path_map[section_path];

    if(!array || index >= array->childCount())
        return default_value;

    Item* sub_element = nullptr;
    Item* element = array->child(index);
    for(int i = 0;i < element->childCount(); ++i)
    {
        Item* child = element->child(i);
        if(!child->text(1).compare(element_name))
        {
            sub_element = child;
            break;
        }
    }

    if(!sub_element)
        return default_value;

    return sub_element->data(0, Qt::UserRole);
}

QVariant Settings::get_array_item(const QString& array_name, int index, const QString &element_name, const QVariant& default_value)
{
    Item* array = nullptr;

    if(section_path_map.contains(array_name))
        array = section_path_map[array_name];

    if(!array || index >= array->childCount())
        return default_value;

    Item* sub_element = nullptr;
    Item* element = array->child(index);
    for(int i = 0;i < element->childCount(); ++i)
    {
        Item* child = element->child(i);
        if(!child->text(1).compare(element_name))
        {
            sub_element = child;
            break;
        }
    }

    if(!sub_element)
        return default_value;

    return sub_element->data(0, Qt::UserRole);
}

void Settings::set_item(const QString& item_name, const QVariant& value)
{
    QStringList locator = construct_path(item_name);

    Item* section = nullptr;

    if(section_path_map.contains(locator[0]))
        section = section_path_map[locator[0]];

    if(!section)
        section = create_path(locator[0]);

    Item* item = nullptr;
    for(int i = 0;i < section->childCount(); ++i)
    {
        Item* child = section->child(i);
        if(!child->text(1).compare(locator[1]))
        {
            item = child;
            break;
        }
    }

    if(!item)
        item = new Item(section, QStringList() << "Item" << locator[1] << "");     // add a new item to this section
    item->setData(0, Qt::UserRole, value);
    item->setText(2, fix_type_name(QString(value.typeName())));
}

void Settings::set_array_item(int index, const QString& element_name, const QVariant& element_value)
{
    QStringList locator = construct_path();
    if(locator.isEmpty())
        return;
    QString section_path = locator.join("/");
    section_path.replace("//", "/");

    Item* array = nullptr;

    if(section_path_map.contains(section_path))
        array = section_path_map[section_path];

    if(!array)
    {
        array = create_path(section_path);
        array->setText(0, "Array");     // set the proper type
    }

    while(array->childCount() < (index + 1))
    {
        int i = array->childCount();
        new Item(array, QStringList() << "Element" << QString::number(i));
    }

    Item* sub_element = nullptr;
    Item* element = array->child(index);
    for(int i = 0;i < element->childCount(); ++i)
    {
        Item* child = element->child(i);
        if(!child->text(1).compare(element_name))
        {
            sub_element = child;
            break;
        }
    }

    if(!sub_element)
        sub_element = new Item(element, QStringList() << "Item" << element_name << "");
    sub_element->setData(0, Qt::UserRole, element_value);
    sub_element->setText(2, fix_type_name(QString(element_value.typeName())));
}

void Settings::set_array_item(const QString& array_name, int index, const QString& element_name, const QVariant& element_value)
{
    Item* array = nullptr;

    if(section_path_map.contains(array_name))
        array = section_path_map[array_name];

    if(!array)
    {
        array = create_path(array_name);
        array->setText(0, "Array");     // set the proper type
    }

    while(array->childCount() < (index + 1))
    {
        int i = array->childCount();
        new Item(array, QStringList() << "Element" << QString::number(i));
    }

    Item* sub_element = nullptr;
    Item* element = array->child(index);
    for(int i = 0;i < element->childCount(); ++i)
    {
        Item* child = element->child(i);
        if(!child->text(1).compare(element_name))
        {
            sub_element = child;
            break;
        }
    }

    if(!sub_element)
        sub_element = new Item(element, QStringList() << "Item" << element_name << "");
    sub_element->setData(0, Qt::UserRole, element_value);
    sub_element->setText(2, fix_type_name(QString(element_value.typeName())));
}
