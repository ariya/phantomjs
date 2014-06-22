/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "node.h"
#include "qdoctagfiles.h"
#include "qdocdatabase.h"

#include "qdom.h"
#include "atom.h"
#include "doc.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include <limits.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
  \class QDocTagFiles

  This class handles the generation of the qdoc tag file.
 */

QDocTagFiles* QDocTagFiles::qdocTagFiles_ = NULL;

/*!
  Constructs the singleton. \a qdb is the pointer to the
  qdoc database that is used when reading and writing the
  index files.
 */
QDocTagFiles::QDocTagFiles()
{
    qdb_ = QDocDatabase::qdocDB();
}

/*!
  Destroys the singleton QDocTagFiles.
 */
QDocTagFiles::~QDocTagFiles()
{
    qdb_ = 0;
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
 */
QDocTagFiles* QDocTagFiles::qdocTagFiles()
{
   if (!qdocTagFiles_)
      qdocTagFiles_ = new QDocTagFiles;
   return qdocTagFiles_;
}

/*!
  Destroys the singleton.
 */
void QDocTagFiles::destroyQDocTagFiles()
{
    if (qdocTagFiles_) {
        delete qdocTagFiles_;
        qdocTagFiles_ = 0;
    }
}

/*!
  Generate the tag file section with the given \a writer for the \a node
  specified, returning true if an element was written; otherwise returns
  false.
 */
void QDocTagFiles::generateTagFileCompounds(QXmlStreamWriter& writer, const InnerNode* inner)
{
    foreach (const Node* node, inner->childNodes()) {
        if (!node->url().isEmpty())
            continue;

        QString kind;
        switch (node->type()) {
        case Node::Namespace:
            kind = "namespace";
            break;
        case Node::Class:
            kind = "class";
            break;
        case Node::Enum:
        case Node::Typedef:
        case Node::Property:
        case Node::Function:
        case Node::Variable:
        default:
            continue;
        }

        QString access;
        switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:
        default:
            continue;
        }

        QString objName = node->name();

        // Special case: only the root node should have an empty name.
        if (objName.isEmpty() && node != qdb_->treeRoot())
            continue;

        // *** Write the starting tag for the element here. ***
        writer.writeStartElement("compound");
        writer.writeAttribute("kind", kind);

        if (node->type() == Node::Class) {
            writer.writeTextElement("name", node->fullDocumentName());
            writer.writeTextElement("filename", gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()));

            // Classes contain information about their base classes.
            const ClassNode* classNode = static_cast<const ClassNode*>(node);
            QList<RelatedClass> bases = classNode->baseClasses();
            foreach (const RelatedClass& related, bases) {
                ClassNode* baseClassNode = related.node;
                writer.writeTextElement("base", baseClassNode->name());
            }

            // Recurse to write all members.
            generateTagFileMembers(writer, static_cast<const InnerNode*>(node));
            writer.writeEndElement();

            // Recurse to write all compounds.
            generateTagFileCompounds(writer, static_cast<const InnerNode*>(node));
        }
        else {
            writer.writeTextElement("name", node->fullDocumentName());
            writer.writeTextElement("filename", gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()));

            // Recurse to write all members.
            generateTagFileMembers(writer, static_cast<const InnerNode*>(node));
            writer.writeEndElement();

            // Recurse to write all compounds.
            generateTagFileCompounds(writer, static_cast<const InnerNode*>(node));
        }
    }
}

/*!
  Writes all the members of the \a inner node with the \a writer.
  The node represents a C++ class, namespace, etc.
 */
void QDocTagFiles::generateTagFileMembers(QXmlStreamWriter& writer, const InnerNode* inner)
{
    foreach (const Node* node, inner->childNodes()) {
        if (!node->url().isEmpty())
            continue;

        QString nodeName;
        QString kind;
        switch (node->type()) {
        case Node::Enum:
            nodeName = "member";
            kind = "enum";
            break;
        case Node::Typedef:
            nodeName = "member";
            kind = "typedef";
            break;
        case Node::Property:
            nodeName = "member";
            kind = "property";
            break;
        case Node::Function:
            nodeName = "member";
            kind = "function";
            break;
        case Node::Namespace:
            nodeName = "namespace";
            break;
        case Node::Class:
            nodeName = "class";
            break;
        case Node::Variable:
        default:
            continue;
        }

        QString access;
        switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:
        default:
            continue;
        }

        QString objName = node->name();

        // Special case: only the root node should have an empty name.
        if (objName.isEmpty() && node != qdb_->treeRoot())
            continue;

        // *** Write the starting tag for the element here. ***
        writer.writeStartElement(nodeName);
        if (!kind.isEmpty())
            writer.writeAttribute("kind", kind);

        switch (node->type()) {
        case Node::Class:
            writer.writeCharacters(node->fullDocumentName());
            writer.writeEndElement();
            break;
        case Node::Namespace:
            writer.writeCharacters(node->fullDocumentName());
            writer.writeEndElement();
            break;
        case Node::Function:
            {
                /*
                  Function nodes contain information about
                  the type of function being described.
                */

                const FunctionNode* functionNode = static_cast<const FunctionNode*>(node);
                writer.writeAttribute("protection", access);

                switch (functionNode->virtualness()) {
                case FunctionNode::NonVirtual:
                    writer.writeAttribute("virtualness", "non");
                    break;
                case FunctionNode::ImpureVirtual:
                    writer.writeAttribute("virtualness", "virtual");
                    break;
                case FunctionNode::PureVirtual:
                    writer.writeAttribute("virtual", "pure");
                    break;
                default:
                    break;
                }
                writer.writeAttribute("static", functionNode->isStatic() ? "yes" : "no");

                if (functionNode->virtualness() == FunctionNode::NonVirtual)
                    writer.writeTextElement("type", functionNode->returnType());
                else
                    writer.writeTextElement("type", "virtual " + functionNode->returnType());

                writer.writeTextElement("name", objName);
                QStringList pieces = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()).split(QLatin1Char('#'));
                writer.writeTextElement("anchorfile", pieces[0]);
                writer.writeTextElement("anchor", pieces[1]);

                // Write a signature attribute for convenience.
                QStringList signatureList;

                foreach (const Parameter& parameter, functionNode->parameters()) {
                    QString leftType = parameter.leftType();
                    const Node* leftNode = qdb_->findNode(parameter.leftType().split("::"),
                                                          0,
                                                          SearchBaseClasses|NonFunction);
                    if (!leftNode || leftNode->type() != Node::Typedef) {
                        leftNode = qdb_->findNode(parameter.leftType().split("::"),
                                                  node->parent(),
                                                  SearchBaseClasses|NonFunction);
                    }
                    if (leftNode && leftNode->type() == Node::Typedef) {
                        const TypedefNode* typedefNode = static_cast<const TypedefNode*>(leftNode);
                        if (typedefNode->associatedEnum()) {
                            leftType = "QFlags<" + typedefNode->associatedEnum()->fullDocumentName() +
                                QLatin1Char('>');
                        }
                    }
                    signatureList.append(leftType + QLatin1Char(' ') + parameter.name());
                }

                QString signature = QLatin1Char('(')+signatureList.join(", ")+QLatin1Char(')');
                if (functionNode->isConst())
                    signature += " const";
                if (functionNode->virtualness() == FunctionNode::PureVirtual)
                    signature += " = 0";
                writer.writeTextElement("arglist", signature);
            }
            writer.writeEndElement(); // member
            break;
        case Node::Property:
            {
                const PropertyNode* propertyNode = static_cast<const PropertyNode*>(node);
                writer.writeAttribute("type", propertyNode->dataType());
                writer.writeTextElement("name", objName);
                QStringList pieces = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()).split(QLatin1Char('#'));
                writer.writeTextElement("anchorfile", pieces[0]);
                writer.writeTextElement("anchor", pieces[1]);
                writer.writeTextElement("arglist", QString());
            }
            writer.writeEndElement(); // member
            break;
        case Node::Enum:
            {
                const EnumNode* enumNode = static_cast<const EnumNode*>(node);
                writer.writeTextElement("name", objName);
                QStringList pieces = gen_->fullDocumentLocation(node).split(QLatin1Char('#'));
                writer.writeTextElement("anchor", pieces[1]);
                writer.writeTextElement("arglist", QString());
                writer.writeEndElement(); // member

                for (int i = 0; i < enumNode->items().size(); ++i) {
                    EnumItem item = enumNode->items().value(i);
                    writer.writeStartElement("member");
                    writer.writeAttribute("name", item.name());
                    writer.writeTextElement("anchor", pieces[1]);
                    writer.writeTextElement("arglist", QString());
                    writer.writeEndElement(); // member
                }
            }
            break;
        case Node::Typedef:
            {
                const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
                if (typedefNode->associatedEnum())
                    writer.writeAttribute("type", typedefNode->associatedEnum()->fullDocumentName());
                else
                    writer.writeAttribute("type", QString());
                writer.writeTextElement("name", objName);
                QStringList pieces = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()).split(QLatin1Char('#'));
                writer.writeTextElement("anchorfile", pieces[0]);
                writer.writeTextElement("anchor", pieces[1]);
                writer.writeTextElement("arglist", QString());
            }
            writer.writeEndElement(); // member
            break;

        case Node::Variable:
        default:
            break;
        }
    }
}

/*!
  Writes a tag file named \a fileName.
 */
void QDocTagFiles::generateTagFile(const QString& fileName, Generator* g)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;

    gen_ = g;
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("tagfile");
    generateTagFileCompounds(writer, qdb_->treeRoot());
    writer.writeEndElement(); // tagfile
    writer.writeEndDocument();
    file.close();
}

QT_END_NAMESPACE
