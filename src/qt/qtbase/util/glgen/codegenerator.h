/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utilities of the Qt Toolkit.
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

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "specparser.h"

class QTextStream;

class CodeGenerator
{
public:
    explicit CodeGenerator();

    void setParser(SpecParser *parser) {m_parser = parser;}

    void generateCoreClasses(const QString &baseFileName) const;

    void generateExtensionClasses(const QString &baseFileName) const;

private:
    // Generic support
    enum ClassVisibility {
        Public,
        Private
    };

    enum ClassComponent {
        Declaration = 0,
        Definition
    };

    bool isLegacyVersion(Version v) const;
    bool versionHasProfiles(Version v) const;

    FunctionCollection functionCollection( const VersionProfile& classVersionProfile ) const;

    void writePreamble(const QString &baseFileName, QTextStream &stream, const QString replacement = QString()) const;
    void writePostamble(const QString &baseFileName, QTextStream &stream) const;

    QString passByType(const Argument &arg) const;
    QString safeArgumentName(const QString& arg) const;

    // Core functionality support
    QString coreClassFileName(const VersionProfile &versionProfile,
                              const QString& fileExtension) const;
    void writeCoreHelperClasses(const QString &fileName, ClassComponent component) const;
    void writeCoreClasses(const QString &baseFileName) const;

    void writeCoreFactoryHeader(const QString &fileName) const;
    void writeCoreFactoryImplementation(const QString &fileName) const;

    QString generateClassName(const VersionProfile &classVersion, ClassVisibility visibility = Public) const;

    void writeBackendClassDeclaration(QTextStream &stream,
                                      const VersionProfile &versionProfile,
                                      const QString &baseClass) const;
    void writeBackendClassImplementation(QTextStream &stream,
                                         const VersionProfile &versionProfile,
                                         const QString &baseClass) const;

    void writePublicClassDeclaration(const QString &baseFileName,
                                     const VersionProfile &versionProfile,
                                     const QString &baseClass) const;
    void writePublicClassImplementation(const QString &baseFileName,
                                        const VersionProfile &versionProfile,
                                        const QString& baseClass) const;

    void writeClassFunctionDeclarations(QTextStream &stream,
                                        const FunctionCollection &functionSets,
                                        ClassVisibility visibility) const;

    void writeFunctionDeclaration(QTextStream &stream, const Function &f, ClassVisibility visibility) const;

    void writeClassInlineFunctions(QTextStream &stream,
                                   const QString &className,
                                   const FunctionCollection &functionSet) const;

    void writeInlineFunction(QTextStream &stream, const QString &className,
                             const QString &backendVar, const Function &f) const;

    void writeEntryPointResolutionCode(QTextStream &stream,
                                       const FunctionCollection &functionSet) const;

    void writeEntryPointResolutionStatement(QTextStream &stream, const Function &f,
                                            const QString &prefix = QString(), bool useGetProcAddress = false) const;

    QList<VersionProfile> backendsForFunctionCollection(const FunctionCollection &functionSet) const;
    QString backendClassName(const VersionProfile &v) const;
    QString backendVariableName(const VersionProfile &v) const;
    void writeBackendVariableDeclarations(QTextStream &stream, const QList<VersionProfile> &backends) const;


    // Extension class support
    void writeExtensionHeader(const QString &fileName) const;
    void writeExtensionImplementation(const QString &fileName) const;

    void writeExtensionClassDeclaration(QTextStream &stream,
                                        const QString &extension,
                                        ClassVisibility visibility = Public) const;
    void writeExtensionClassImplementation(QTextStream &stream, const QString &extension) const;

    QString generateExtensionClassName(const QString &extension, ClassVisibility visibility = Public) const;
    void writeExtensionInlineFunction(QTextStream &stream, const QString &className, const Function &f) const;

    SpecParser *m_parser;
    mutable QMap<QString, int> m_extensionIds;
};

#endif // CODEGENERATOR_H
