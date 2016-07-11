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

#include <QtCore/QCoreApplication>
#include <QtCore/QQueue>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QDebug>
#include <iostream>

#include "qcommandline.h"

const QCommandLineConfigEntry QCommandLine::helpEntry = { QCommandLine::Switch, QLatin1Char('h'), QLatin1String("help"), tr("Display this help and exit"), QCommandLine::Optional };

const QCommandLineConfigEntry QCommandLine::versionEntry = { QCommandLine::Switch, QLatin1Char('V'), QLatin1String("version"), tr("Display version and exit"), QCommandLine::Optional };

QCommandLine::QCommandLine(QObject * parent)
  : QObject(parent), d(new QCommandLinePrivate)
{
  setArguments(QCoreApplication::instance()->arguments());
}

QCommandLine::QCommandLine(const QCoreApplication & app,
			   const QCommandLineConfig & config,
			   QObject * parent)
  : QObject(parent), d(new QCommandLinePrivate)
{
  setArguments(app.arguments());
  setConfig(config);
  enableHelp(true);
  enableVersion(true);
}

QCommandLine::QCommandLine(int argc, char *argv[],
			   const QCommandLineConfig & config,
			   QObject * parent)
  : QObject(parent), d(new QCommandLinePrivate)
{
  setArguments(argc, argv);
  setConfig(config);
  enableHelp(true);
  enableVersion(true);
}

QCommandLine::QCommandLine(const QStringList args,
			   const QCommandLineConfig & config,
			   QObject * parent)
  : QObject(parent), d(new QCommandLinePrivate)
{
  setArguments(args);
  setConfig(config);
  enableHelp(true);
  enableVersion(true);
}

QCommandLine::~QCommandLine()
{
  delete d;
}

void
QCommandLine::setConfig(const QCommandLineConfig & config)
{
  d->config = config;
}

void
QCommandLine::setConfig(const QCommandLineConfigEntry config[])
{
  d->config.clear();

  while (config->type) {
    d->config << *config;
    config++;
  }
}

QCommandLineConfig
QCommandLine::config()
{
  return d->config;
}

void
QCommandLine::setArguments(int argc, char *argv[])
{
  d->args.clear();
  for (int i = 0; i < argc; i++)
    d->args.append(QLatin1String(argv[i]));
}

void
QCommandLine::setArguments(QStringList args)
{
  d->args = args;
}

QStringList
QCommandLine::arguments()
{
  return d->args;
}

void
QCommandLine::enableHelp(bool enable)
{
  d->help = enable;
}

bool
QCommandLine::helpEnabled()
{
  return d->help;
}

void
QCommandLine::enableVersion(bool enable)
{
  d->version = enable;
}

bool
QCommandLine::versionEnabled()
{
  return d->version;
}

bool
QCommandLine::parse()
{
  QMap < QString, QCommandLineConfigEntry > conf;
  QMap < QString, QCommandLineConfigEntry > confLong;
  QQueue < QCommandLineConfigEntry > params;
  QMap < QString, QList < QString > > optionsFound;
  QMap < QString, int > switchsFound;
  QStringList options, switchs;
  QStringList args = d->args;

  bool allparam = false;

  foreach (QCommandLineConfigEntry entry, d->config) {
#if 0
    if (entry.type != QCommandLine::Param && entry.shortName == QLatin1Char('\0'))
      qWarning() << QLatin1String("QCommandLine: Empty shortname detected");
    if (entry.longName.isEmpty())
      qWarning() << QLatin1String("QCommandLine: Empty shortname detected");
    if (entry.type != QCommandLine::Param && conf.find(entry.shortName) != conf.end())
      qWarning() << QLatin1String("QCommandLine: Duplicated shortname detected ") << entry.shortName;
#endif
    if (conf.find(entry.longName) != conf.end())
      qWarning() << QLatin1String("QCommandLine: Duplicated longname detected ") << entry.shortName;

    if (entry.type == QCommandLine::Param)
      params << entry;
    else
      conf[entry.shortName] = entry;
    confLong[entry.longName] = entry;
  }

  if (d->help) {
    conf[helpEntry.shortName] = helpEntry;
    confLong[helpEntry.longName] = helpEntry;
  }

  if (d->version) {
    conf[versionEntry.shortName] = versionEntry;
    confLong[versionEntry.longName] = versionEntry;
  }

  for (int i = 1; i < args.size(); ++i) {
    QString arg = args[i];
    bool param = true, shrt = false, stay = false, forward = false;

    /* A '+' was found, all remaining options are params */
    if (allparam)
      param = true;
    else if (arg.startsWith(QLatin1String("--"))) {
      param = false;
      shrt = false;
      arg = arg.mid(2);
    } else if (arg.startsWith(QLatin1Char('-')) || arg.startsWith(QLatin1Char('+'))) {
      if (arg.startsWith(QLatin1Char('+')))
	allparam = true;
      param = false;
      shrt = true;
      /* Handle stacked args like `tar -xzf` */
      if (arg.size() > 2) {
	args[i] = arg.mid(0, 2);
	args.insert(i + 1, arg.mid(0, 1) + arg.mid(2));
	arg = arg.mid(1, 1);
      } else {
	arg = arg.mid(1);
      }
    }

    /* Handle params */
    if (param) {
      if (!params.size()) {
	emit parseError(tr("Unknown param: %1").arg(arg));
	return false;
      }

      QCommandLineConfigEntry & entry = params.first();

      if (entry.flags & QCommandLine::Mandatory) {
	entry.flags = (QCommandLine::Flags) (entry.flags & ~QCommandLine::Mandatory);
	entry.flags = (QCommandLine::Flags) (entry.flags | QCommandLine::Optional);
      }

     if (entry.flags & QCommandLine::ParameterFence) {
        allparam = true;
     }

      emit paramFound(entry.longName, arg);

      if (!(entry.flags & QCommandLine::Multiple))
	params.dequeue();

    } else { /* Options and switchs* */
      QString key;
      QString value;
      int idx = arg.indexOf(QLatin1Char('='));

      if (idx != -1) {
	key = arg.mid(0, idx);
	value = arg.mid(idx + 1);
      } else {
	key = arg;
      }

      QMap < QString, QCommandLineConfigEntry > & c = shrt ? conf : confLong;

      if (c.find(key) == c.end()) {
	emit parseError(tr("Unknown option: %1").arg(key));
	return false;
      }

      QCommandLineConfigEntry & entry = c[key];

      if (entry.type == QCommandLine::Switch) {
	if (!switchsFound.contains(entry.longName))
	  switchs << entry.longName;
	if (entry.flags & QCommandLine::Multiple)
	  switchsFound[entry.longName]++;
	else
	  switchsFound[entry.longName] = 1;
      } else {
	if (stay) {
	  emit parseError(tr("Option %1 need a value").arg(key));
	  return false;
	}

	if (idx == -1) {
	  if (i+1 < args.size() && !args[i+1].startsWith(QLatin1Char('-'))) {
	    value = args[i+1];
	    forward = true;
	  } else {
	    emit parseError(tr("Option %1 need a value").arg(key));
	    return false;
	  }
	}

	if (!optionsFound.contains(entry.longName))
	  options << entry.longName;
	if (!(entry.flags & QCommandLine::Multiple))
	  optionsFound[entry.longName].clear();
	optionsFound[entry.longName].append(value);
      }

      if (entry.flags & QCommandLine::Mandatory) {
	entry.flags = (QCommandLine::Flags) (entry.flags & ~QCommandLine::Mandatory);
	entry.flags = (QCommandLine::Flags) (entry.flags | QCommandLine::Optional);
	conf[entry.shortName] = entry;
	confLong[entry.shortName] = entry;
      }
    }
    /* Stay here, stacked args */
    if (stay)
      i--;
    else if (forward)
      i++;
  }

  foreach (QCommandLineConfigEntry entry, params) {
    if (entry.flags & QCommandLine::Mandatory) {
      emit parseError(tr("Param %1 is mandatory").arg(entry.longName));
      return false;
    }
  }

  foreach (QCommandLineConfigEntry entry, conf.values()) {
    if (entry.flags & QCommandLine::Mandatory) {
      QString type;

      if (entry.type == QCommandLine::Switch)
	type = tr("Switch");
      if (entry.type == QCommandLine::Option)
	type = tr("Option");

      emit parseError(tr("%1 %2 is mandatory").arg(type).arg(entry.longName));
      return false;
    }
  }

  foreach (QString key, switchs) {
    for (int i = 0; i < switchsFound[key]; i++) {
      if (d->help && key == helpEntry.longName)
	showHelp();
      if (d->version && key == versionEntry.longName)
	showVersion();
      emit switchFound(key);
    }
  }

  foreach (QString key, options) {
    foreach (QString opt, optionsFound[key])
      emit optionFound(key, opt);
  }  return true;
}

void
QCommandLine::addOption(const QChar & shortName,
			const QString & longName,
			const QString & descr,
			QCommandLine::Flags flags)
{
  QCommandLineConfigEntry entry;

  entry.type = QCommandLine::Option;
  entry.shortName = shortName;
  entry.longName = longName;
  entry.descr = descr;
  entry.flags = flags;
  d->config << entry;
}

void
QCommandLine::addSwitch(const QChar & shortName,
			const QString & longName,
			const QString & descr,
			QCommandLine::Flags flags)
{
  QCommandLineConfigEntry entry;

  entry.type = QCommandLine::Switch;
  entry.shortName = shortName;
  entry.longName = longName;
  entry.descr = descr;
  entry.flags = flags;
  d->config << entry;
}

void
QCommandLine::addParam(const QString & name,
		       const QString & descr,
		       QCommandLine::Flags flags)
{
  QCommandLineConfigEntry entry;

  entry.type = QCommandLine::Param;
  entry.longName = name;
  entry.descr = descr;
  entry.flags = flags;
  d->config << entry;
}

void
QCommandLine::removeOption(const QString & name)
{
  int i;

  for (i = 0; i < d->config.size(); ++i) {
    if (d->config[i].type == QCommandLine::Option &&
	(d->config[i].shortName == name.at(0) || d->config[i].longName == name)) {
      d->config.removeAt(i);
      return ;
    }
  }
}

void
QCommandLine::removeSwitch(const QString & name)
{
  int i;

  for (i = 0; i < d->config.size(); ++i) {
    if (d->config[i].type == QCommandLine::Switch &&
	(d->config[i].shortName == name.at(0) || d->config[i].longName == name)) {
      d->config.removeAt(i);
      return ;
    }
  }
}

void
QCommandLine::removeParam(const QString & name)
{
  int i;

  for (i = 0; i < d->config.size(); ++i) {
    if (d->config[i].type == QCommandLine::Param &&
	(d->config[i].shortName == name.at(0) || d->config[i].longName == name)) {
      d->config.removeAt(i);
      return ;
    }
  }
}


QString
QCommandLine::help(bool logo)
{
  QString h;

  if (logo)
    h = version() + QLatin1String("\n");
  h = QLatin1String("Usage:\n   ");
  /* Executable name */
  if (!d->args.isEmpty())
    h += QFileInfo(d->args[0]).baseName();
  else
    h += QCoreApplication::applicationName();
  h.append(QLatin1String(" [switches] [options]"));
  /* Arguments, short */
  foreach (QCommandLineConfigEntry entry, d->config) {
    if (entry.type == QCommandLine::Option) {
      if (entry.flags & QCommandLine::Mandatory)
	h.append(QLatin1String(" --") + entry.longName + QLatin1String("=<val>"));
    }
    if (entry.type == QCommandLine::Param) {
      h.append(QLatin1String(" "));
      if (entry.flags & QCommandLine::Optional)
	h.append(QLatin1String("["));
      h.append(entry.longName);
      if (entry.flags & QCommandLine::Multiple)
	h.append(QLatin1String(" [") + entry.longName + QLatin1String(" [...]]"));
      if (entry.flags & QCommandLine::Optional)
	h.append(QLatin1String("]"));
    }
  }
  h.append(QLatin1String("\n\n"));

  h.append(QLatin1String("Options:\n"));

  QStringList vals;
  QStringList descrs;
  int max = 0;

  foreach (QCommandLineConfigEntry entry, d->config) {
    QString val;

    if (entry.type == QCommandLine::Option) {
      if (entry.shortName != QLatin1Char('\0'))
        val = QLatin1String("-") + QString(entry.shortName) + QLatin1Char(',');
	  val += QLatin1String("--") + entry.longName + QLatin1String("=<val>");
    }
    if (entry.type == QCommandLine::Switch)
      val = QLatin1String("-") + QString(entry.shortName) + QLatin1String(",--") + entry.longName;
#if 0
    if (entry.type == QCommandLine::Param)
      val = entry.longName;
#endif

    if (val.size() > max)
      max = val.size();

    vals.append(val);
    descrs.append(entry.descr + QLatin1String("\n"));
  }

  for (int i = 0; i < vals.size(); ++i) {
    if (vals[i].isEmpty()) continue;
    h.append(QLatin1String("  "));
    h.append(vals[i]);
    h.append(QString(QLatin1String(" ")).repeated(max - vals[i].size() + 2));
    h.append(descrs[i]);
  }

#if 0
  h.append(tr("\nMandatory arguments to long options are mandatory for short options too.\n"));
#endif
  return h;
}

QString
QCommandLine::version()
{
  QString v;

  v = QCoreApplication::applicationName() + QLatin1Char(' ');
  v += QCoreApplication::applicationVersion();
  if (!QCoreApplication::organizationDomain().isEmpty()
      || !QCoreApplication::organizationName().isEmpty())
    v = v + QLatin1String(" - ") +
      QCoreApplication::organizationDomain() + QLatin1String(" ") +
      QCoreApplication::organizationDomain();
  return v + QLatin1Char('\n');
}

void
QCommandLine::showHelp(bool quit, int returnCode)
{
  std::cerr << qPrintable(help());
  if (quit) {
    // Can't call QApplication::exit() here, because we may be called before app.exec()
    exit(returnCode);
  }
}

void
QCommandLine::showVersion(bool quit, int returnCode)
{
  std::cerr << qPrintable(version());
  if (quit) {
    exit(returnCode);
  }
}
