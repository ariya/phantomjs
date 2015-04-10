/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "localemodel.h"

#include <QLocale>
#include <QDate>
#include <qdebug.h>

static const int g_model_cols = 6;

struct LocaleListItem
{
    int language;
    int country;
};

const LocaleListItem g_locale_list[] = {
    {      1,     0 }, // C/AnyCountry
    {      3,    69 }, // Afan/Ethiopia
    {      3,   111 }, // Afan/Kenya
    {      4,    59 }, // Afar/Djibouti
    {      4,    67 }, // Afar/Eritrea
    {      4,    69 }, // Afar/Ethiopia
    {      5,   195 }, // Afrikaans/SouthAfrica
    {      5,   148 }, // Afrikaans/Namibia
    {      6,     2 }, // Albanian/Albania
    {      7,    69 }, // Amharic/Ethiopia
    {      8,   186 }, // Arabic/SaudiArabia
    {      8,     3 }, // Arabic/Algeria
    {      8,    17 }, // Arabic/Bahrain
    {      8,    64 }, // Arabic/Egypt
    {      8,   103 }, // Arabic/Iraq
    {      8,   109 }, // Arabic/Jordan
    {      8,   115 }, // Arabic/Kuwait
    {      8,   119 }, // Arabic/Lebanon
    {      8,   122 }, // Arabic/LibyanArabJamahiriya
    {      8,   145 }, // Arabic/Morocco
    {      8,   162 }, // Arabic/Oman
    {      8,   175 }, // Arabic/Qatar
    {      8,   201 }, // Arabic/Sudan
    {      8,   207 }, // Arabic/SyrianArabRepublic
    {      8,   216 }, // Arabic/Tunisia
    {      8,   223 }, // Arabic/UnitedArabEmirates
    {      8,   237 }, // Arabic/Yemen
    {      9,    11 }, // Armenian/Armenia
    {     10,   100 }, // Assamese/India
    {     12,    15 }, // Azerbaijani/Azerbaijan
    {     14,   197 }, // Basque/Spain
    {     15,    18 }, // Bengali/Bangladesh
    {     15,   100 }, // Bengali/India
    {     16,    25 }, // Bhutani/Bhutan
    {     20,    33 }, // Bulgarian/Bulgaria
    {     22,    20 }, // Byelorussian/Belarus
    {     23,    36 }, // Cambodian/Cambodia
    {     24,   197 }, // Catalan/Spain
    {     25,    44 }, // Chinese/China
    {     25,    97 }, // Chinese/HongKong
    {     25,   126 }, // Chinese/Macau
    {     25,   190 }, // Chinese/Singapore
    {     25,   208 }, // Chinese/Taiwan
    {     27,    54 }, // Croatian/Croatia
    {     28,    57 }, // Czech/CzechRepublic
    {     29,    58 }, // Danish/Denmark
    {     30,   151 }, // Dutch/Netherlands
    {     30,    21 }, // Dutch/Belgium
    {     31,   225 }, // English/UnitedStates
    {     31,     4 }, // English/AmericanSamoa
    {     31,    13 }, // English/Australia
    {     31,    21 }, // English/Belgium
    {     31,    22 }, // English/Belize
    {     31,    28 }, // English/Botswana
    {     31,    38 }, // English/Canada
    {     31,    89 }, // English/Guam
    {     31,    97 }, // English/HongKong
    {     31,   100 }, // English/India
    {     31,   104 }, // English/Ireland
    {     31,   107 }, // English/Jamaica
    {     31,   133 }, // English/Malta
    {     31,   134 }, // English/MarshallIslands
    {     31,   148 }, // English/Namibia
    {     31,   154 }, // English/NewZealand
    {     31,   160 }, // English/NorthernMarianaIslands
    {     31,   163 }, // English/Pakistan
    {     31,   170 }, // English/Philippines
    {     31,   190 }, // English/Singapore
    {     31,   195 }, // English/SouthAfrica
    {     31,   215 }, // English/TrinidadAndTobago
    {     31,   224 }, // English/UnitedKingdom
    {     31,   226 }, // English/UnitedStatesMinorOutlyingIslands
    {     31,   234 }, // English/USVirginIslands
    {     31,   240 }, // English/Zimbabwe
    {     33,    68 }, // Estonian/Estonia
    {     34,    71 }, // Faroese/FaroeIslands
    {     36,    73 }, // Finnish/Finland
    {     37,    74 }, // French/France
    {     37,    21 }, // French/Belgium
    {     37,    38 }, // French/Canada
    {     37,   125 }, // French/Luxembourg
    {     37,   142 }, // French/Monaco
    {     37,   206 }, // French/Switzerland
    {     40,   197 }, // Galician/Spain
    {     41,    81 }, // Georgian/Georgia
    {     42,    82 }, // German/Germany
    {     42,    14 }, // German/Austria
    {     42,    21 }, // German/Belgium
    {     42,   123 }, // German/Liechtenstein
    {     42,   125 }, // German/Luxembourg
    {     42,   206 }, // German/Switzerland
    {     43,    85 }, // Greek/Greece
    {     43,    56 }, // Greek/Cyprus
    {     44,    86 }, // Greenlandic/Greenland
    {     46,   100 }, // Gujarati/India
    {     47,    83 }, // Hausa/Ghana
    {     47,   156 }, // Hausa/Niger
    {     47,   157 }, // Hausa/Nigeria
    {     48,   105 }, // Hebrew/Israel
    {     49,   100 }, // Hindi/India
    {     50,    98 }, // Hungarian/Hungary
    {     51,    99 }, // Icelandic/Iceland
    {     52,   101 }, // Indonesian/Indonesia
    {     57,   104 }, // Irish/Ireland
    {     58,   106 }, // Italian/Italy
    {     58,   206 }, // Italian/Switzerland
    {     59,   108 }, // Japanese/Japan
    {     61,   100 }, // Kannada/India
    {     63,   110 }, // Kazakh/Kazakhstan
    {     64,   179 }, // Kinyarwanda/Rwanda
    {     65,   116 }, // Kirghiz/Kyrgyzstan
    {     66,   114 }, // Korean/RepublicOfKorea
    {     67,   102 }, // Kurdish/Iran
    {     67,   103 }, // Kurdish/Iraq
    {     67,   207 }, // Kurdish/SyrianArabRepublic
    {     67,   217 }, // Kurdish/Turkey
    {     69,   117 }, // Laothian/Lao
    {     71,   118 }, // Latvian/Latvia
    {     72,    49 }, // Lingala/DemocraticRepublicOfCongo
    {     72,    50 }, // Lingala/PeoplesRepublicOfCongo
    {     73,   124 }, // Lithuanian/Lithuania
    {     74,   127 }, // Macedonian/Macedonia
    {     76,   130 }, // Malay/Malaysia
    {     76,    32 }, // Malay/BruneiDarussalam
    {     77,   100 }, // Malayalam/India
    {     78,   133 }, // Maltese/Malta
    {     80,   100 }, // Marathi/India
    {     82,   143 }, // Mongolian/Mongolia
    {     84,   150 }, // Nepali/Nepal
    {     85,   161 }, // Norwegian/Norway
    {     87,   100 }, // Oriya/India
    {     88,     1 }, // Pashto/Afghanistan
    {     89,   102 }, // Persian/Iran
    {     89,     1 }, // Persian/Afghanistan
    {     90,   172 }, // Polish/Poland
    {     91,   173 }, // Portuguese/Portugal
    {     91,    30 }, // Portuguese/Brazil
    {     92,   100 }, // Punjabi/India
    {     92,   163 }, // Punjabi/Pakistan
    {     95,   177 }, // Romanian/Romania
    {     96,   178 }, // Russian/RussianFederation
    {     96,   222 }, // Russian/Ukraine
    {     99,   100 }, // Sanskrit/India
    {    100,   241 }, // Serbian/SerbiaAndMontenegro
    {    100,    27 }, // Serbian/BosniaAndHerzegowina
    {    100,   238 }, // Serbian/Yugoslavia
    {    101,   241 }, // SerboCroatian/SerbiaAndMontenegro
    {    101,    27 }, // SerboCroatian/BosniaAndHerzegowina
    {    101,   238 }, // SerboCroatian/Yugoslavia
    {    102,   195 }, // Sesotho/SouthAfrica
    {    103,   195 }, // Setswana/SouthAfrica
    {    107,   195 }, // Siswati/SouthAfrica
    {    108,   191 }, // Slovak/Slovakia
    {    109,   192 }, // Slovenian/Slovenia
    {    110,   194 }, // Somali/Somalia
    {    110,    59 }, // Somali/Djibouti
    {    110,    69 }, // Somali/Ethiopia
    {    110,   111 }, // Somali/Kenya
    {    111,   197 }, // Spanish/Spain
    {    111,    10 }, // Spanish/Argentina
    {    111,    26 }, // Spanish/Bolivia
    {    111,    43 }, // Spanish/Chile
    {    111,    47 }, // Spanish/Colombia
    {    111,    52 }, // Spanish/CostaRica
    {    111,    61 }, // Spanish/DominicanRepublic
    {    111,    63 }, // Spanish/Ecuador
    {    111,    65 }, // Spanish/ElSalvador
    {    111,    90 }, // Spanish/Guatemala
    {    111,    96 }, // Spanish/Honduras
    {    111,   139 }, // Spanish/Mexico
    {    111,   155 }, // Spanish/Nicaragua
    {    111,   166 }, // Spanish/Panama
    {    111,   168 }, // Spanish/Paraguay
    {    111,   169 }, // Spanish/Peru
    {    111,   174 }, // Spanish/PuertoRico
    {    111,   225 }, // Spanish/UnitedStates
    {    111,   227 }, // Spanish/Uruguay
    {    111,   231 }, // Spanish/Venezuela
    {    113,   111 }, // Swahili/Kenya
    {    113,   210 }, // Swahili/Tanzania
    {    114,   205 }, // Swedish/Sweden
    {    114,    73 }, // Swedish/Finland
    {    116,   209 }, // Tajik/Tajikistan
    {    117,   100 }, // Tamil/India
    {    118,   178 }, // Tatar/RussianFederation
    {    119,   100 }, // Telugu/India
    {    120,   211 }, // Thai/Thailand
    {    122,    67 }, // Tigrinya/Eritrea
    {    122,    69 }, // Tigrinya/Ethiopia
    {    124,   195 }, // Tsonga/SouthAfrica
    {    125,   217 }, // Turkish/Turkey
    {    129,   222 }, // Ukrainian/Ukraine
    {    130,   100 }, // Urdu/India
    {    130,   163 }, // Urdu/Pakistan
    {    131,   228 }, // Uzbek/Uzbekistan
    {    131,     1 }, // Uzbek/Afghanistan
    {    132,   232 }, // Vietnamese/VietNam
    {    134,   224 }, // Welsh/UnitedKingdom
    {    136,   195 }, // Xhosa/SouthAfrica
    {    138,   157 }, // Yoruba/Nigeria
    {    140,   195 }, // Zulu/SouthAfrica
    {    141,   161 }, // Nynorsk/Norway
    {    142,    27 }, // Bosnian/BosniaAndHerzegowina
    {    143,   131 }, // Divehi/Maldives
    {    144,   224 }, // Manx/UnitedKingdom
    {    145,   224 }, // Cornish/UnitedKingdom
    {    146,    83 }, // Akan/Ghana
    {    147,   100 }, // Konkani/India
    {    148,    83 }, // Ga/Ghana
    {    149,   157 }, // Igbo/Nigeria
    {    150,   111 }, // Kamba/Kenya
    {    151,   207 }, // Syriac/SyrianArabRepublic
    {    152,    67 }, // Blin/Eritrea
    {    153,    67 }, // Geez/Eritrea
    {    153,    69 }, // Geez/Ethiopia
    {    154,   157 }, // Koro/Nigeria
    {    155,    69 }, // Sidamo/Ethiopia
    {    156,   157 }, // Atsam/Nigeria
    {    157,    67 }, // Tigre/Eritrea
    {    158,   157 }, // Jju/Nigeria
    {    159,   106 }, // Friulian/Italy
    {    160,   195 }, // Venda/SouthAfrica
    {    161,    83 }, // Ewe/Ghana
    {    161,   212 }, // Ewe/Togo
    {    163,   225 }, // Hawaiian/UnitedStates
    {    164,   157 }, // Tyap/Nigeria
    {    165,   129 }, // Chewa/Malawi
};
static const int g_locale_list_count = sizeof(g_locale_list)/sizeof(g_locale_list[0]);

LocaleModel::LocaleModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_data_list.append(1234.5678);
    m_data_list.append(QDate::currentDate());
    m_data_list.append(QDate::currentDate());
    m_data_list.append(QTime::currentTime());
    m_data_list.append(QTime::currentTime());
}

QVariant LocaleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole
        || index.column() >= g_model_cols
        || index.row() >= g_locale_list_count + 2)
        return QVariant();

    QVariant data;
    if (index.column() < g_model_cols - 1)
        data = m_data_list.at(index.column());

    if (index.row() == 0) {
        if (role == Qt::ToolTipRole)
            return QVariant();
        switch (index.column()) {
            case 0:
                return data.toDouble();
            case 1:
                return data.toDate();
            case 2:
                return data.toDate();
            case 3:
                return data.toTime();
            case 4:
                return data.toTime();
            case 5:
                return QVariant();
            default:
                break;
        }
    } else {
        QLocale locale;
        if (index.row() == 1) {
            locale = QLocale::system();
        } else {
            LocaleListItem item = g_locale_list[index.row() - 2];
            locale = QLocale((QLocale::Language)item.language, (QLocale::Country)item.country);
        }

        switch (index.column()) {
            case 0:
                if (role == Qt::ToolTipRole)
                    return QVariant();
                return locale.toString(data.toDouble());
            case 1:
                if (role == Qt::ToolTipRole)
                    return locale.dateFormat(QLocale::LongFormat);
                return locale.toString(data.toDate(), QLocale::LongFormat);
            case 2:
                if (role == Qt::ToolTipRole)
                    return locale.dateFormat(QLocale::ShortFormat);
                return locale.toString(data.toDate(), QLocale::ShortFormat);
            case 3:
                if (role == Qt::ToolTipRole)
                    return locale.timeFormat(QLocale::LongFormat);
                return locale.toString(data.toTime(), QLocale::LongFormat);
            case 4:
                if (role == Qt::ToolTipRole)
                    return locale.timeFormat(QLocale::ShortFormat);
                return locale.toString(data.toTime(), QLocale::ShortFormat);
            case 5:
                if (role == Qt::ToolTipRole)
                    return QVariant();
                return locale.name();
            default:
                break;
        }
    }

    return QVariant();
}

QVariant LocaleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return QLatin1String("Double");
            case 1:
                return QLatin1String("Long Date");
            case 2:
                return QLatin1String("Short Date");
            case 3:
                return QLatin1String("Long Time");
            case 4:
                return QLatin1String("Short Time");
            case 5:
                return QLatin1String("Name");
            default:
                break;
        }
    } else {
        if (section >= g_locale_list_count + 2)
            return QVariant();
        if (section == 0) {
            return QLatin1String("Input");
        } else if (section == 1) {
            return QLatin1String("System");
        } else {
            LocaleListItem item = g_locale_list[section - 2];
            return QLocale::languageToString((QLocale::Language)item.language)
                    + QLatin1Char('/')
                    + QLocale::countryToString((QLocale::Country)item.country);
        }
    }

    return QVariant();
}

QModelIndex LocaleModel::index(int row, int column,
                    const QModelIndex &parent) const
{
    if (parent.isValid()
        || row >= g_locale_list_count + 2
        || column >= g_model_cols)
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex LocaleModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int LocaleModel::columnCount(const QModelIndex&) const
{
    return g_model_cols;
}

int LocaleModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return g_locale_list_count + 2;
}

Qt::ItemFlags LocaleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    if (index.row() == 0 && index.column() == g_model_cols - 1)
        return 0;
    if (index.row() == 0)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractItemModel::flags(index);
}

bool LocaleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()
        || index.row() != 0
        || index.column() >= g_model_cols - 1
        || role != Qt::EditRole
        || m_data_list.at(index.column()).type() != value.type())
        return false;

    m_data_list[index.column()] = value;
    emit dataChanged(createIndex(1, index.column()),
            createIndex(g_locale_list_count, index.column()));

    return true;
}
