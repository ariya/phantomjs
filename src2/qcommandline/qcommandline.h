/* This file is part of QCommandLine
 *
 * Copyright (C) 2010-2011 Corentin Chary <corentin.chary@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef QCOMMAND_LINE_H
# define QCOMMAND_LINE_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QStringList>

#ifndef QCOMMANDLINE_EXPORT
# ifndef QCOMMANDLINE_STATIC
#  if defined(QCOMMANDLINE_MAKEDLL)
    /* We are building this library */
#   define QCOMMANDLINE_EXPORT Q_DECL_EXPORT
#  else
    /* We are using this library */
#   define QCOMMANDLINE_EXPORT Q_DECL_IMPORT
#  endif
# endif
#endif
#ifndef QCOMMANDLINE_EXPORT
# define QCOMMANDLINE_EXPORT
#endif

class QCoreApplication;

struct QCommandLineConfigEntry;
typedef QList< QCommandLineConfigEntry > QCommandLineConfig;

class QCommandLinePrivate {
public:
    bool version;
    bool help;
    QStringList args;
    QCommandLineConfig config;
};

/**
 * Use this macro to mark the end of a QCommandLineConfigEntry array
 */
#define QCOMMANDLINE_CONFIG_ENTRY_END      \
    { QCommandLine::None, '\0', NULL, NULL, QCommandLine::Default }

/**
 * @brief Main class used to convert parse command line
 */
class QCOMMANDLINE_EXPORT QCommandLine : public QObject
{
  Q_OBJECT
public:
    /**
     * Enum used to determine entry type in QCommandLineConfigEntry
     */
    typedef enum {
	None = 0, /**< can be used for the last line of a QCommandLineConfigEntry[] . */
	Switch, /**< a simple switch wihout argument (eg: ls -l) */
	Option, /**< an option with an argument (eg: tar -f test.tar) */
	Param /**< a parameter without '-' delimiter (eg: cp foo bar) */
    } Type;

    /**
     * Flags that can be applied to a QCommandLineConfigEntry
     */
    typedef enum {
	Default = 0, /**< can be used for the last line of a QCommandLineConfigEntry[] . */
	Mandatory = 0x01, /**< mandatory argument, will produce a parse error if not present */
	Optional = 0x02, /**< optional argument */
	Multiple = 0x04, /**< argument can be used multiple time and will produce multiple signals. */
	ParameterFence = 0x08, //**< all arguments after this point are considered parameters, not options.  */
	MandatoryMultiple = Mandatory|Multiple,
    OptionalMultiple = Optional|Multiple
    } Flags;

    /**
     * QCommandLine constructor
     * QCoreApplication::instance()->arguments() will be called to get the arguments.
     */
    QCommandLine(QObject * parent = 0);

    /**
     * QCommandLine constructor
     * @param app arguments() will be called on this app to get the list of arguments.
     * @param config The parser config
     * @param parent The parent for this object
     * @sa setConfig
     */
    QCommandLine(const QCoreApplication & app,
		 const QCommandLineConfig & config = QCommandLineConfig(),
		 QObject * parent = 0);

    /**
     * QCommandLine constructor
     * @param argc Size of the argv array
     * @param argv Argument array
     * @param config The parser config
     * @param parent The parent for this object
     * @sa setArguments
     * @sa setConfig
     */
    QCommandLine(int argc, char *argv[],
		 const QCommandLineConfig & config = QCommandLineConfig(),
		 QObject * parent = 0);

    /**
     * QCommandLine constructor
     * @param args Command line arguments
     * @param config The parser config
     * @param parent The parent for this object
     * @sa setArguments
     * @sa setConfig
     */
    QCommandLine(const QStringList args,
		 const QCommandLineConfig & config = QCommandLineConfig(),
		 QObject * parent = 0);

    /**
     * QCommandLine destructor
     */
   ~QCommandLine();

    /**
     * Set the parser configuration
     * @param config The configuration
     * @sa config
     */
    void setConfig(const QCommandLineConfig & config);

    /**
     * Set the parser configuration
     * @param config An array containing the configuration
     * @sa config
     */
    void setConfig(const QCommandLineConfigEntry config[]);

    /**
     * Get the current parser configuration
     * @returns The parser configuration
     * @sa setConfig
     */
    QCommandLineConfig config();

    /**
     * Set command line arguments
     * @param argc Size of the argv array
     * @param argv Array of arguments
     * @sa arguments
     */
    void setArguments(int argc, char *argv[]);

    /**
     * Set command line arguments
     * @param args A list of arguments
     * @sa arguments
     */
    void setArguments(QStringList args);

    /**
     * Get command line arguments
     * @returns Command line arguments (like QApplication::arguments())
     * @sa arguments
     */
    QStringList arguments();

    /**
     * Enable --help,-h switch
     * @param enable true to enable, false to disable
     * @sa enableHelp
     */
    void enableHelp(bool enable);

    /**
     * Check if help is enabled or not.
     * @returns true if help is enabled; otherwise returns false.
     * @sa enableVersion
     */
    bool helpEnabled();

    /**
     * Enable --version,-V switch
     * @param enable true to enable, false to disable
     * @sa versionEnabled
     */
    void enableVersion(bool enable);

    /**
     * Check if version is enabled or not.
     * @returns true if version is enabled; otherwise returns false.
     * @sa enableHelp
     */
    bool versionEnabled();

    /**
     * Parse command line and emmit signals when switchs, options, or
     * param are found.
     * @returns true if successfully parsed; otherwise returns false.
     * @sa parseError
     */
    bool parse();

    /**
     * Define a new option
     * @param shortName Short name for this option (ex: h)
     * @param longName Long name for this option (ex: help)
     * @param descr Help text
     * @param flags Switch flags
     * @sa addSwitch
     * @sa addParam
     */
    void addOption(const QChar & shortName,
		   const QString & longName = QString(),
		   const QString & descr = QString(),
		   QCommandLine::Flags flags = QCommandLine::Optional);

    /**
     * Define a new switch
     * @param shortName Short name for this switch (ex: h)
     * @param longName Long name for this switch (ex: help)
     * @param descr Help text
     * @param flags Parameter flags
     * @sa addOption
     * @sa addParam
     */
    void addSwitch(const QChar & shortName,
		   const QString & longName = QString(),
		   const QString & descr = QString(),
		   QCommandLine::Flags flags = QCommandLine::Optional);

    /**
     * Define a new parameter
     * @param name Name, used in help, usage and error messages
     * @param descr Help text
     * @param flags Parameter flags
     * @sa addSwitch
     * @sa addOption
     */
    void addParam(const QString & name,
		  const QString & descr = QString(),
		  QCommandLine::Flags flags = QCommandLine::Optional);

    /**
     * Remove any option of type QCommandLine::Option with a given shortName or longName.
     * @param name the name of the option to remove
     * @sa removeParam
     * @sa removeSwitch
     */
    void removeOption(const QString & name);

    /**
     * Remove any option of type QCommandLine::Switch with a given shortName or longName.
     * @param name the name of the option to remove
     * @sa removeOption
     * @sa removeParam
     */
    void removeSwitch(const QString & name);

    /**
     * Remove any option of type QCommandLine::Param with a given shortName or longName.
     * @param name the name of the option to remove
     * @sa removeOption
     * @sa removeSwitch
     */
    void removeParam(const QString & name);

    /**
     * Return the help message
     * @param logo also show version message on top of the help message
     * @sa version
     */
    QString help(bool logo = true);

    /**
     * Return the version message
     * @sa help
     */
    QString version();

    /**
     * Show the help message.
     * @param exit Exit if true
     * @param returnCode return code of the program if exit is true
     * @sa showVersion
     */
    void showHelp(bool exit = true, int returnCode = 0);

    /**
     * Show the version message.
     * @param exit Exit if true
     * @param returnCode return code of the program if exit is true
     * @sa showHelp
     */
    void showVersion(bool exit = true, int returnCode = 0);

    /**
     * Standard --help, -h entry
     */
    static const QCommandLineConfigEntry helpEntry;

    /**
     * Standard --version, -V entry
     */
    static const QCommandLineConfigEntry versionEntry;

signals:
    /**
     * Signal emitted when a switch is found while parsing
     * @param name The "longName" of the switch.
     * @sa parse
     * @sa addSwitch
     */
    void switchFound(const QString & name);

    /**
     * Signal emitted when an option is found while parsing
     * @param name The "longName" of the switch.
     * @param value The value of that option
     * @sa parse
     * @sa addSwitch
     */
    void optionFound(const QString & name, const QVariant & value);

    /**
     * Signal emitted when a param is found while parsing
     * @param name The "longName" of the switch.
     * @param value The actual argument
     * @sa parse
     * @sa addSwitch
     */
    void paramFound(const QString & name, const QVariant & value);

    /**
     * Signal emitted when a parse error is detected
     * @param error Parse error description
     * @sa parse
     */
    void parseError(const QString & error);
private:
    QCommandLinePrivate *d;
    Q_DECLARE_PRIVATE(QCommandLine);
};

/**
 * @brief Configuration entry structure
 */
struct QCommandLineConfigEntry {
    /**
     * Entry Type
     */
    QCommandLine::Type type;
    /**
     * Short Name
     */
    QChar shortName;
    /**
     * Long Name
     */
    QString longName;
    /**
     * Description, used in --help
     */
    QString descr;
    /**
     * Option flags
     */
    QCommandLine::Flags flags;
};

#endif
