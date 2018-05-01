/*
 * Copyright (c) 2013-2015 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PARAMETER_H__
#define PARAMETER_H__

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QColor>

class Parameter
{
public:
    enum ParameterType {
        INVALID_P = -1,
        BOOL_P,
        BUTTON_P,
        CHOICE_P,
        COLOR_P,
        CONST_P,
        FILE_P,
        FLOAT_P,
        FOLDER_P,
        INT_P,
        LINK_P,
        NOTE_P,
        TEXT_P,
        SEPARATOR_P,
    };

    Parameter(const QString &name, bool updatePreview = true);
    virtual ~Parameter(){}

    QString m_name;
    ParameterType m_type;
    bool m_updatePreview;

    virtual QString toString();
    // if the parameter is only GUI option, return null string
    virtual QString value() const;
    virtual void setValue(const QString &value);

    virtual void parseValues(const QString& typeDefinition);

    QString name() const { return m_name; }
    bool isPresentationalOnly() const;

    virtual void reset() { };

    static Parameter::ParameterType nameToType(const QString &typeName);
    static bool isTypeDefined(const QString &typeName);
    QString typeName() const;


protected:
    // strips parameter type (int, note, etc.) and enclosing brackets
    QString extractValues(const QString& typeDefinition);
    // returns list of parameter values or empty item if parameter list is empty
    QStringList getValues(const QString& typeDefinition);
    static QString stripQuotes(const QString& str);
    static QString addQuotes(const QString& str);


    static QMap<Parameter::ParameterType, QString> initMap();

public:
    static const QMap<Parameter::ParameterType, QString> PARAMETER_NAMES;
    static const QList<QString> PARAMETER_NAMES_STRINGS;

};

class FloatParameter : public Parameter
{
public:
    FloatParameter(const QString& name, bool updatePreview = true);

    float m_defaultValue;
    float m_value;
    float m_minValue;
    float m_maxValue;

    virtual QString value() const;
    virtual void setValue(const QString& value);

    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual void reset();
};

class IntParameter : public Parameter
{
public:
    IntParameter(const QString& name, bool updatePreview = true);
    virtual ~IntParameter(){}

    int m_defaultValue;
    int m_value;
    int m_minValue;
    int m_maxValue;

    virtual QString value() const;
    virtual void setValue(const QString& value);

    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    // reset parameter to default value from gmic definition
    // some parameters do not need reset, e.g. const is not mutable
    virtual void reset();
};

class SeparatorParameter : public Parameter
{
public:
    SeparatorParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
};

class ChoiceParameter : public Parameter
{
public:
    ChoiceParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);

    // default index
    int m_defaultValue;
    // current index
    int m_value;
    QStringList m_choices;

    virtual QString value() const;
    // you can use int or name, if it is int, it will be set as index,
    // if you use name of choice, index will be determined
    virtual void setValue(const QString& value);
    void setIndex(int i);
    virtual QString toString();
    virtual void reset();
};

class NoteParameter : public Parameter
{
public:
    NoteParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();

    QString m_label;

};

class LinkParameter : public Parameter
{
public:
    LinkParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();


    QString m_link;
};

class BoolParameter : public Parameter
{
public:
    BoolParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void reset();

    void initValue(bool value);

    bool m_value;
    bool m_defaultValue;

};

class ColorParameter : public Parameter
{
public:
    ColorParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void reset();

    QColor m_value;
    QColor m_defaultValue;
    bool m_hasAlpha;
};

class TextParameter : public Parameter
{
public:
    TextParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void setValue(const QString& value);
    virtual void reset();

    QString toUiValue() const;
    void fromUiValue(const QString &uiValue);

    bool m_multiline;

private:
    QString m_value;
    QString m_defaultValue;
};

class FolderParameter : public Parameter
{
public:
    FolderParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void setValue(const QString& value);
    virtual void reset();

    QString toUiValue();
    void fromUiValue(const QString &uiValue);

    QString m_folderPath;
    QString m_defaultFolderPath;
};

class FileParameter : public Parameter
{
public:
    FileParameter(const QString& name, bool updatePreview = true);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void setValue(const QString& value);
    virtual void reset();

    QString toUiValue();
    void fromUiValue(const QString &uiValue);

    QString m_filePath;
    QString m_defaultFilePath;
};

class ConstParameter : public Parameter
{
public:
    ConstParameter(const QString& name, bool updatePreview = false);
    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;

    QStringList m_values;
};

class ButtonParameter : public Parameter
{
public:
    ButtonParameter(const QString& name, bool updatePreview = false);
    enum Aligment { AlignLeft, AlignRight, AlignCenter };

    virtual void parseValues(const QString& typeDefinition);
    virtual QString toString();
    virtual QString value() const;
    virtual void setValue(const QString& value);
    virtual void reset();
    void initValue(bool value);

    bool m_value;
    bool m_defaultValue;
    Aligment m_buttonAligment;


};


#endif
