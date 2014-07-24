/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qmachparser_p.h"

#if defined(Q_OF_MACH_O) && !defined(QT_NO_LIBRARY)

#include <qendian.h>
#include "qlibrary_p.h"

#include <mach-o/loader.h>
#include <mach-o/fat.h>

QT_BEGIN_NAMESPACE

#if defined(Q_PROCESSOR_X86_64)
#  define MACHO64
static const cpu_type_t my_cputype = CPU_TYPE_X86_64;
#elif defined(Q_PROCESSOR_X86_32)
static const cpu_type_t my_cputype = CPU_TYPE_X86;
#elif defined(Q_PROCESSOR_POWER_64)
#  define MACHO64
static const cpu_type_t my_cputype = CPU_TYPE_POWERPC64;
#elif defined(Q_PROCESSOR_POWER_32)
static const cpu_type_t my_cputype = CPU_TYPE_POWERPC;
#elif defined(Q_PROCESSOR_ARM)
static const cpu_type_t my_cputype = CPU_TYPE_ARM;
#else
#  error "Unknown CPU type"
#endif

#ifdef MACHO64
#  undef MACHO64
typedef mach_header_64 my_mach_header;
typedef segment_command_64 my_segment_command;
typedef section_64 my_section;
static const uint32_t my_magic = MH_MAGIC_64;
#else
typedef mach_header my_mach_header;
typedef segment_command my_segment_command;
typedef section my_section;
static const uint32_t my_magic = MH_MAGIC;
#endif

static int ns(const QString &reason, const QString &library, QString *errorString)
{
    if (errorString)
        *errorString = QLibrary::tr("'%1' is not a valid Mach-O binary (%2)")
                .arg(library, reason.isEmpty() ? QLibrary::tr("file is corrupt") : reason);
    return QMachOParser::NotSuitable;
}

int QMachOParser::parse(const char *m_s, ulong fdlen, const QString &library, QString *errorString, long *pos, ulong *sectionlen)
{
    // The minimum size of a Mach-O binary we're interested in.
    // It must have a full Mach header, at least one segment and at least one
    // section. It's probably useless with just the "qtmetadata" section, but
    // it's valid nonetheless.
    // A fat binary must have this plus the fat header, of course.
    static const size_t MinFileSize = sizeof(my_mach_header) + sizeof(my_segment_command) + sizeof(my_section);
    static const size_t MinFatHeaderSize = sizeof(fat_header) + 2 * sizeof(fat_arch);

    if (Q_UNLIKELY(fdlen < MinFileSize))
        return ns(QLibrary::tr("file too small"), library, errorString);

    // find out if this is a fat Mach-O binary first
    const my_mach_header *header = 0;
    const fat_header *fat = reinterpret_cast<const fat_header *>(m_s);
    if (fat->magic == qToBigEndian(FAT_MAGIC)) {
        // find our architecture in the binary
        const fat_arch *arch = reinterpret_cast<const fat_arch *>(fat + 1);
        if (Q_UNLIKELY(fdlen < MinFatHeaderSize)) {
            return ns(QLibrary::tr("file too small"), library, errorString);
        }

        int count = qFromBigEndian(fat->nfat_arch);
        if (Q_UNLIKELY(fdlen < sizeof(*fat) + sizeof(*arch) * count))
            return ns(QString(), library, errorString);

        for (int i = 0; i < count; ++i) {
            if (arch[i].cputype == qToBigEndian(my_cputype)) {
                // ### should we check the CPU subtype? Maybe on ARM?
                uint32_t size = qFromBigEndian(arch[i].size);
                uint32_t offset = qFromBigEndian(arch[i].offset);
                if (Q_UNLIKELY(size > fdlen) || Q_UNLIKELY(offset > fdlen)
                        || Q_UNLIKELY(size + offset > fdlen) || Q_UNLIKELY(size < MinFileSize))
                    return ns(QString(), library, errorString);

                header = reinterpret_cast<const my_mach_header *>(m_s + offset);
                fdlen = size;
                break;
            }
        }
        if (!header)
            return ns(QLibrary::tr("no suitable architecture in fat binary"), library, errorString);

        // check the magic again
        if (Q_UNLIKELY(header->magic != my_magic))
            return ns(QString(), library, errorString);
    } else {
        header = reinterpret_cast<const my_mach_header *>(m_s);
        fat = 0;

        // check magic
        if (header->magic != my_magic)
            return ns(QLibrary::tr("invalid magic %1").arg(qFromBigEndian(header->magic), 8, 16, QLatin1Char('0')),
                      library, errorString);
    }

    // from this point on, fdlen is specific to this architecture
    // from this point on, everything is in host byte order
    *pos = reinterpret_cast<const char *>(header) - m_s;

    // (re-)check the CPU type
    // ### should we check the CPU subtype? Maybe on ARM?
    if (header->cputype != my_cputype) {
        if (fat)
            return ns(QString(), library, errorString);
        return ns(QLibrary::tr("wrong architecture"), library, errorString);
    }

    // check the file type
    if (Q_UNLIKELY(header->filetype != MH_BUNDLE && header->filetype != MH_DYLIB))
        return ns(QLibrary::tr("not a dynamic library"), library, errorString);

    // find the __TEXT segment, "qtmetadata" section
    const my_segment_command *seg = reinterpret_cast<const my_segment_command *>(header + 1);
    ulong minsize = sizeof(*header);

    for (uint i = 0; i < header->ncmds; ++i,
         seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize)) {
        // We're sure that the file size includes at least one load command
        // but we have to check anyway if we're past the first
        if (Q_UNLIKELY(fdlen < minsize + sizeof(load_command)))
            return ns(QString(), library, errorString);

        // cmdsize can't be trusted until validated
        // so check it against fdlen anyway
        // (these are unsigned operations, with overflow behavior specified in the standard)
        minsize += seg->cmdsize;
        if (Q_UNLIKELY(fdlen < minsize) || Q_UNLIKELY(fdlen < seg->cmdsize))
            return ns(QString(), library, errorString);

        const uint32_t MyLoadCommand = sizeof(void *) > 4 ? LC_SEGMENT_64 : LC_SEGMENT;
        if (seg->cmd != MyLoadCommand)
            continue;

        // is this the __TEXT segment?
        if (strcmp(seg->segname, "__TEXT") == 0) {
            const my_section *sect = reinterpret_cast<const my_section *>(seg + 1);
            for (uint j = 0; j < seg->nsects; ++j) {
                // is this the "qtmetadata" section?
                if (strcmp(sect[j].sectname, "qtmetadata") != 0)
                    continue;

                // found it!
                if (Q_UNLIKELY(fdlen < sect[j].offset) || Q_UNLIKELY(fdlen < sect[j].size)
                        || Q_UNLIKELY(fdlen < sect[j].offset + sect[j].size))
                    return ns(QString(), library, errorString);

                *pos += sect[j].offset;
                *sectionlen = sect[j].size;
                return QtMetaDataSection;
            }
        }

        // other type of segment
        seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize);
    }

//    // No Qt section was found, but at least we know that where the proper architecture's boundaries are
//    return NoQtSection;
    if (errorString)
        *errorString = QLibrary::tr("'%1' is not a Qt plugin").arg(library);
    return NotSuitable;
}

QT_END_NAMESPACE

#endif
