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
#include <QtCore>
#include <QtXml>

class XKBLayout
{
public:
    QString name;
    QString description;
    QStringList variants;
};

QDomElement find(const QString &tagName, const QDomElement &e)
{
    QDomNodeList children = e.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        const QDomNode &n = children.at(i);
        if (n.isElement()) {
            QDomElement c = n.toElement();
            if (c.tagName() == tagName)
                return c;
        }
    }
    return QDomElement();
}

QString parseVariant(const QDomElement &e)
{
    QDomElement configItem = find("configItem", e);
    return find("name", configItem).text();
}

QStringList findVariants(const QDomElement &e)
{
    QStringList variants;

    QDomNodeList children = e.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        const QDomNode &n = children.at(i);
        if (n.isElement())
            variants += parseVariant(n.toElement());
    }

    return variants;
}

XKBLayout parseLayout(const QDomElement &e)
{
    QDomElement configItem = find("configItem", e);

    XKBLayout layout;
    layout.name = find("name", configItem).text();
    layout.description = find("description", configItem).text();

    QDomElement variantList = find("variantList", e);
    if (!variantList.isNull())
        layout.variants = findVariants(variantList);

    return layout;
}

QList<XKBLayout> findLayouts(const QDomElement &layoutList)
{
    QList<XKBLayout> layouts;

    QDomNodeList children = layoutList.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        const QDomNode &n = children.at(i);
        if (n.isElement())
            layouts += parseLayout(n.toElement());
    }

    return layouts;
}

QString mapCountry(const QString &v)
{
    static QMap<QString, QString> map;
    static bool initialized = false;
    if (!initialized) {
        map["U.S. English"] = "UnitedStates";
        map["PC-98xx Series"] = "Japan";
        map["Bosnia and Herzegovina"] = "BosniaAndHerzegowina";
        map["Czechia"] = "CzechRepublic";
        map["Faroe Islands"] = "FaroeIslands";
        map["Laos"] = "Laos";
        map["Latin American"] = "Mexico";
        map["Russia"] = "Russia";
        map["Syria"] = "Syria";
        map["Sri Lanka"] = "SriLanka";
        map["United Kingdom"] = "UnitedKingdom";
        map["Vietnam"] = "Vietnam";
        map["Macedonian"] = "Macedonia";
        map["Serbian"] = "Serbia";
        map["Turkish "] = "Turkey";
        map["Maori"] = "NewZealand";
        map["Arabic"] = "UnitedArabEmirates";
        initialized = true;
    }
    return map.value(v, v);
}

QString mapLanguage(const QString &v)
{
    static QMap<QString, QString> map;
    static bool initialized = false;
    if (!initialized) {
        map["us"] = "English";
        map["us:intl"] = "English";
        map["us:alt-intl"] = "English";
        map["us:dvorak"] = "English";
        map["us:rus"] = "Russian";
        map["ara"] = "Arabic";
        map["ara:azerty"] = "Arabic";
        map["ara:azerty_digits"] = "Arabic";
        map["ara:digits"] = "Arabic";
        map["ara:qwerty"] = "Arabic";
        map["ara:qwerty_digits"] = "Arabic";
        map["al"] = "Albanian";
        map["am"] = "Armenian";
        map["am:phonetic"] = "Armenian";
        map["az"] = "Azerbaijani";
        map["az:cyrillic"] = "Azerbaijani";
        map["by"] = "Belarusian";
        map["by:winkeys"] = "Belarusian";
        map["be"] = "Dutch";
        map["be:iso-alternate"] = "Dutch";
        map["be:nodeadkeys"] = "Dutch";
        map["be:sundeadkeys"] = "Dutch";
        map["bd"] = "Bengali";
        map["bd:probhat"] = "Bengali";
        map["in"] = "Hindi";
        map["in:ben"] = "Bengali";
        map["in:ben_probhat"] = "Bengali";
        map["in:guj"] = "Gujarati";
        map["in:guru"] = "Punjabi";
        map["in:kan"] = "Kannada";
        map["in:mal"] = "Malayalam";
        map["in:ori"] = "Oriya";
        map["in:tam_unicode"] = "Tamil";
        map["in:tam_TAB"] = "Tamil";
        map["in:tam_TSCII"] = "Tamil";
        map["in:tam"] = "Tamil";
        map["in:tel"] = "Telugu";
        map["in:urd"] = "Urdu";
        map["ba"] = "Bosnian";
        map["br"] = "Portuguese";
        map["br:nodeadkeys"] = "Portuguese";
        map["bg"] = "Bulgarian";
        map["bg:phonetic"] = "Bulgarian";
        map["mm"] = "Burmese";
        map["ca"] = "English";
        map["ca:fr-dvorak"] = "French";
        map["ca:fr-legacy"] = "French";
        map["ca:multi"] = "English";
        map["ca:multi-2gr"] = "English";
        map["ca:ike"] = "Inuktitut";
        map["hr"] = "Croatian";
        map["hr:us"] = "Croatian";
        map["cz"] = "Czech";
        map["cz:bksl"] = "Czech";
        map["cz:qwerty"] = "Czech";
        map["cz:qwerty_bksl"] = "Czech";
        map["dk"] = "Danish";
        map["dk:nodeadkeys"] = "Danish";
        map["nl"] = "Dutch";
        map["bt"] = "Bhutani";
        map["ee"] = "Estonian";
        map["ee:nodeadkeys"] = "Estonian";
        map["ir"] = "Persian";
        map["fo"] = "Faroese";
        map["fo:nodeadkeys"] = "Faroese";
        map["fi"] = "Finnish";
        map["fi:nodeadkeys"] = "Finnish";
        map["fi:smi"] = "Finnish";
        map["fr"] = "French";
        map["fr:nodeadkeys"] = "French";
        map["fr:sundeadkeys"] = "French";
        map["fr:latin9"] = "French";
        map["fr:latin9_nodeadkeys"] = "French";
        map["fr:latin9_sundeadkeys"] = "French";
        map["fr:dvorak"] = "French";
        map["ge"] = "Georgian";
        map["ge:ru"] = "Russian";
        map["de"] = "German";
        map["de:deadacute"] = "German";
        map["de:deadgraveacute"] = "German";
        map["de:nodeadkeys"] = "German";
        map["de:ro"] = "Romanian";
        map["de:ro_nodeadkeys"] = "Romanian";
        map["de:dvorak"] = "German";
        map["gr"] = "Greek";
        map["gr:extended"] = "Greek";
        map["gr:nodeadkeys"] = "Greek";
        map["gr:polytonic"] = "Greek";
        map["hu"] = "Hungarian";
        map["hu:standard"] = "Hungarian";
        map["hu:nodeadkeys"] = "Hungarian";
        map["hu:qwerty"] = "Hungarian";
        map["hu:101_qwertz_comma_dead"] = "Hungarian";
        map["hu:101_qwertz_comma_nodead"] = "Hungarian";
        map["hu:101_qwertz_dot_dead"] = "Hungarian";
        map["hu:101_qwertz_dot_nodead"] = "Hungarian";
        map["hu:101_qwerty_comma_dead"] = "Hungarian";
        map["hu:101_qwerty_comma_nodead"] = "Hungarian";
        map["hu:101_qwerty_dot_dead"] = "Hungarian";
        map["hu:101_qwerty_dot_nodead"] = "Hungarian";
        map["hu:102_qwertz_comma_dead"] = "Hungarian";
        map["hu:102_qwertz_comma_nodead"] = "Hungarian";
        map["hu:102_qwertz_dot_dead"] = "Hungarian";
        map["hu:102_qwertz_dot_nodead"] = "Hungarian";
        map["hu:102_qwerty_comma_dead"] = "Hungarian";
        map["hu:102_qwerty_comma_nodead"] = "Hungarian";
        map["hu:102_qwerty_dot_dead"] = "Hungarian";
        map["hu:102_qwerty_dot_nodead"] = "Hungarian";
        map["is"] = "Icelandic";
        map["is:Sundeadkeys"] = "Icelandic";
        map["is:nodeadkeys"] = "Icelandic";
        map["il"] = "Hebrew";
        map["il:lyx"] = "Hebrew";
        map["il:si1452"] = "Hebrew";
        map["il:phonetic"] = "Hebrew";
        map["it"] = "Italian";
        map["it:nodeadkeys"] = "Italian";
        map["jp"] = "Japanese";
        map["kg"] = "Kirghiz";
        map["la"] = "Laothian";
        map["latam"] = "Spanish";
        map["latam:nodeadkeys"] = "Spanish";
        map["latam:sundeadkeys"] = "Spanish";
        map["lt"] = "Lithuanian";
        map["lt:std"] = "Lithuanian";
        map["lt:us"] = "Lithuanian";
        map["lv"] = "Latvian";
        map["lv:apostrophe"] = "Latvian";
        map["lv:tilde"] = "Latvian";
        map["lv:fkey"] = "Latvian";
        map["mao"] = "Maori";
        map["mkd"] = "Macedonian";
        map["mkd:nodeadkeys"] = "Macedonian";
        map["mt"] = "Maltese";
        map["mt:us"] = "Maltese";
        map["mn"] = "Mongolian";
        map["no"] = "Norwegian";
        map["no:nodeadkeys"] = "Norwegian";
        map["no:dvorak"] = "Norwegian";
        map["no:smi"] = "Norwegian";
        map["no:smi_nodeadkeys"] = "Norwegian";
        map["pl"] = "Polish";
        map["pl:qwertz"] = "Polish";
        map["pl:dvorak"] = "Polish";
        map["pl:dvorak_quotes"] = "Polish";
        map["pl:dvorak_altquotes"] = "Polish";
        map["pt"] = "Portuguese";
        map["pt:nodeadkeys"] = "Portuguese";
        map["pt:sundeadkeys"] = "Portuguese";
        map["ro"] = "Romanian";
        map["ro:us"] = "English";
        map["ro:de"] = "German";
        map["ru"] = "Russian";
        map["ru:phonetic"] = "Russian";
        map["ru:typewriter"] = "Russian";
        map["ru:winkeys"] = "Russian";
        map["srp"] = "Serbian";
        map["srp:yz"] = "Serbian";
        map["srp:latin"] = "Serbian";
        map["srp:latinunicode"] = "Serbian";
        map["srp:latinyz"] = "Serbian";
        map["srp:latinunicodeyz"] = "Serbian";
        map["srp:alternatequotes"] = "Serbian";
        map["srp:latinalternatequotes"] = "Serbian";
        map["si"] = "Slovenian";
        map["sk"] = "Slovak";
        map["sk:bksl"] = "Slovak";
        map["sk:qwerty"] = "Slovak";
        map["sk:qwerty_bksl"] = "Slovak";
        map["es"] = "Spanish";
        map["es:nodeadkeys"] = "Spanish";
        map["es:sundeadkeys"] = "Spanish";
        map["es:dvorak"] = "Spanish";
        map["se"] = "Swedish";
        map["se:nodeadkeys"] = "Swedish";
        map["se:dvorak"] = "Swedish";
        map["se:rus"] = "Russian";
        map["se:rus_nodeadkeys"] = "Russian";
        map["se:smi"] = "Swedish";
        map["ch"] = "German";
        map["ch:de_nodeadkeys"] = "German";
        map["ch:de_sundeadkeys"] = "German";
        map["ch:fr"] = "French";
        map["ch:fr_nodeadkeys"] = "French";
        map["ch:fr_sundeadkeys"] = "French";
        map["sy"] = "Arabic";
        map["sy:syc"] = "Arabic";
        map["sy:syc_phonetic"] = "Arabic";
        map["tj"] = "Tajik";
        map["lk"] = "Sinhala";
        map["lk:tam_unicode"] = "Tamil";
        map["lk:tam_TAB"] = "Tamil";
        map["lk:tam_TSCII"] = "Tamil";
        map["lk:sin_phonetic"] = "Sinhala";
        map["th"] = "Thai";
        map["th:tis"] = "Thai";
        map["th:pat"] = "Thai";
        map["tr"] = "Turkish";
        map["tr:f"] = "Turkish";
        map["tr:alt"] = "Turkish";
        map["ua"] = "Ukrainian";
        map["ua:phonetic"] = "Ukrainian";
        map["ua:typewriter"] = "Ukrainian";
        map["ua:winkeys"] = "Ukrainian";
        map["ua:rstu"] = "Ukrainian";
        map["ua:rstu_ru"] = "Ukrainian";
        map["gb"] = "English";
        map["gb:intl"] = "English";
        map["gb:dvorak"] = "English";
        map["uz"] = "Uzbek";
        map["vn"] = "Vietnamese";
        map["nec_vndr/jp"] = "Japanese";
        map["ie"] = "Irish";
        map["ie:CloGaelach"] = "Gaelic";
        map["ie:UnicodeExpert"] = "Irish";
        map["ie:ogam"] = "Gaelic";
        map["ie:ogam_is434"] = "Gaelic";
        map["pk"] = "Urdu";
        initialized = true;
    }
    return map.value(v, v);
}

QString mapDirection(const QString &v)
{
    static QMap<QString, QString> map;
    static bool initialized = false;
    if (!initialized) {
        // 1. xkbdata-X11R7.0-1.0.1/symbols% grep -l '\([Hh]ebrew\|[Aa]rabic\)' **/*
        map["Arabic"] = "Qt::RightToLeft";
        map["Persian"] = "Qt::RightToLeft";
        map["Urdu"] = "Qt::RightToLeft";
        initialized = true;
    }
    return map.value(v, "Qt::LeftToRight");
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <xml input file>\n", argv[0]);
        return 1;
    }

    QCoreApplication app(argc, argv);

    QFile file;
    file.setFileName(argv[1]);
    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "cannot open %s: %s\n", qPrintable(file.fileName()), qPrintable(file.errorString()));
        return 1;
    }

    QDomDocument dom;
    dom.setContent(&file);

    if (dom.documentElement().tagName() != QLatin1String("xkbConfigRegistry")) {
        fprintf(stderr, "cannot parse %s, this is not an XKB config file\n", qPrintable(file.fileName()));
        return 1;
    }

    QDomElement layoutList = find("layoutList", dom.documentElement());
    if (layoutList.isNull()) {
        fprintf(stderr, "cannot parse %s, this is not an XKB config file\n", qPrintable(file.fileName()));
        return 1;
    }

    QList<XKBLayout> layouts = findLayouts(layoutList);

    // copyright and stuff
    printf("/****************************************************************************\n"
           "**\n"
           "** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).\n"
           "** Contact: http://www.qt-project.org/legal\n"
           "**\n"
           "** This file is part of the QtGui module of the Qt Toolkit.\n"
           "**\n"
           "** $QT_BEGIN_LICENSE:LGPL$\n"
           "** Commercial License Usage\n"
           "** Licensees holding valid commercial Qt licenses may use this file in\n"
           "** accordance with the commercial license agreement provided with the\n"
           "** Software or, alternatively, in accordance with the terms contained in\n"
           "** a written agreement between you and Digia.  For licensing terms and\n"
           "** conditions see http://qt.digia.com/licensing.  For further information\n"
           "** use the contact form at http://qt.digia.com/contact-us.\n"
           "**\n"
           "** GNU Lesser General Public License Usage\n"
           "** Alternatively, this file may be used under the terms of the GNU Lesser\n"
           "** General Public License version 2.1 as published by the Free Software\n"
           "** Foundation and appearing in the file LICENSE.LGPL included in the\n"
           "** packaging of this file.  Please review the following information to\n"
           "** ensure the GNU Lesser General Public License version 2.1 requirements\n"
           "** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
           "**\n"
           "** In addition, as a special exception, Digia gives you certain additional\n"
           "** rights.  These rights are described in the Digia Qt LGPL Exception\n"
           "** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.\n"
           "**\n"
           "** GNU General Public License Usage\n"
           "** Alternatively, this file may be used under the terms of the GNU\n"
           "** General Public License version 3.0 as published by the Free Software\n"
           "** Foundation and appearing in the file LICENSE.GPL included in the\n"
           "** packaging of this file.  Please review the following information to\n"
           "** ensure the GNU General Public License version 3.0 requirements will be\n"
           "** met: http://www.gnu.org/copyleft/gpl.html.\n"
           "**\n"
           "**\n"
           "** $QT_END_LICENSE$\n"
           "**\n"
           "****************************************************************************/\n"
           "\n"
           "// This file is auto-generated, do not edit!\n"
           "// (Generated using util/xkbdatagen)\n"
           "\n");

    // data structure
    printf("static struct {\n"
           "    const char *layout;\n"
           "    const char *variant; // 0 means any variant\n"
           "    Qt::LayoutDirection direction;\n"
           "    QLocale::Language language;\n"
           "    QLocale::Country country;\n"
           "} xkbLayoutData[] = {\n");

    // contents
    foreach (const XKBLayout &l, layouts) {
        const QString country = mapCountry(l.description);
        QString lang = mapLanguage(l.name);
        if (lang.isEmpty())
            lang = "C";
        printf("    // name = %s, description = %s\n"
               "    { \"%s\", \"\", %s, QLocale::%s, QLocale::%s },\n",
               l.name.toLatin1().constData(),
               l.description.toLatin1().constData(),
               l.name.toLatin1().constData(),
               mapDirection(lang).toLatin1().constData(),
               lang.toLatin1().constData(),
               country.toLatin1().constData());
        foreach (const QString &v, l.variants) {
            QString vlang = mapLanguage(l.name + ":" + v);
            if (vlang.isEmpty())
                vlang = "C";
            printf("    // name = %s:%s, description = %s\n"
                   "    { \"%s\", \"%s\", %s, QLocale::%s, QLocale::%s },\n",
                   l.name.toLatin1().constData(),
                   v.toLatin1().constData(),
                   l.description.toLatin1().constData(),
                   l.name.toLatin1().constData(),
                   v.toLatin1().constData(),
                   mapDirection(vlang).toLatin1().constData(),
                   vlang.toLatin1().constData(),
                   country.toLatin1().constData());
        }
    }

    // wrapping up
    printf("    { 0, 0, Qt::LeftToRight, QLocale::C, QLocale::AnyCountry }\n"
           "};\n");

    return 0;
}
