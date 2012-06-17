/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "msvc_objectmodel.h"
#include "msvc_vcproj.h"
#include "msvc_vcxproj.h"
#include <qstringlist.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

// XML Tags ---------------------------------------------------------
const char _Configuration[]                     = "Configuration";
const char _Configurations[]                    = "Configurations";
const char q_File[]                              = "File";
const char _FileConfiguration[]                 = "FileConfiguration";
const char q_Files[]                             = "Files";
const char _Filter[]                            = "Filter";
const char _Globals[]                           = "Globals";
const char _Platform[]                          = "Platform";
const char _Platforms[]                         = "Platforms";
const char _Tool[]                              = "Tool";
const char _VisualStudioProject[]               = "VisualStudioProject";

// XML Properties ---------------------------------------------------
const char _AddModuleNamesToAssembly[]          = "AddModuleNamesToAssembly";
const char _AdditionalDependencies[]            = "AdditionalDependencies";
const char _AdditionalFiles[]                   = "AdditionalFiles";
const char _AdditionalIncludeDirectories[]      = "AdditionalIncludeDirectories";
const char _AdditionalLibraryDirectories[]      = "AdditionalLibraryDirectories";
const char _AdditionalOptions[]                 = "AdditionalOptions";
const char _AdditionalUsingDirectories[]        = "AdditionalUsingDirectories";
const char _AssemblerListingLocation[]          = "AssemblerListingLocation";
const char _AssemblerOutput[]                   = "AssemblerOutput";
const char _ATLMinimizesCRunTimeLibraryUsage[]  = "ATLMinimizesCRunTimeLibraryUsage";
const char _BaseAddress[]                       = "BaseAddress";
const char _BasicRuntimeChecks[]                = "BasicRuntimeChecks";
const char _BrowseInformation[]                 = "BrowseInformation";
const char _BrowseInformationFile[]             = "BrowseInformationFile";
const char _BufferSecurityCheck[]               = "BufferSecurityCheck";
const char _BuildBrowserInformation[]           = "BuildBrowserInformation";
const char _CPreprocessOptions[]                = "CPreprocessOptions";
const char _CallingConvention[]                 = "CallingConvention";
const char _CharacterSet[]                      = "CharacterSet";
const char _CommandLine[]                       = "CommandLine";
const char _CompileAs[]                         = "CompileAs";
const char _CompileAsManaged[]                  = "CompileAsManaged";
const char _CompileOnly[]                       = "CompileOnly";
const char _ConfigurationType[]                 = "ConfigurationType";
const char _Culture[]                           = "Culture";
const char _DLLDataFileName[]                   = "DLLDataFileName";
const char _DebugInformationFormat[]            = "DebugInformationFormat";
const char _DefaultCharIsUnsigned[]             = "DefaultCharIsUnsigned";
const char _DefaultCharType[]                   = "DefaultCharType";
const char _DelayLoadDLLs[]                     = "DelayLoadDLLs";
const char _DeleteExtensionsOnClean[]           = "DeleteExtensionsOnClean";
const char _Description[]                       = "Description";
const char _Detect64BitPortabilityProblems[]    = "Detect64BitPortabilityProblems";
const char _DisableLanguageExtensions[]         = "DisableLanguageExtensions";
const char _DisableSpecificWarnings[]           = "DisableSpecificWarnings";
const char _EnableCOMDATFolding[]               = "EnableCOMDATFolding";
const char _EnableErrorChecks[]                 = "EnableErrorChecks";
const char _EnableEnhancedInstructionSet[]      = "EnableEnhancedInstructionSet";
const char _EnableFiberSafeOptimizations[]      = "EnableFiberSafeOptimizations";
const char _EnableFunctionLevelLinking[]        = "EnableFunctionLevelLinking";
const char _EnableIntrinsicFunctions[]          = "EnableIntrinsicFunctions";
const char _EntryPointSymbol[]                  = "EntryPointSymbol";
const char _ErrorCheckAllocations[]             = "ErrorCheckAllocations";
const char _ErrorCheckBounds[]                  = "ErrorCheckBounds";
const char _ErrorCheckEnumRange[]               = "ErrorCheckEnumRange";
const char _ErrorCheckRefPointers[]             = "ErrorCheckRefPointers";
const char _ErrorCheckStubData[]                = "ErrorCheckStubData";
const char _ExceptionHandling[]                 = "ExceptionHandling";
const char _ExcludedFromBuild[]                 = "ExcludedFromBuild";
const char _ExpandAttributedSource[]            = "ExpandAttributedSource";
const char _ExportNamedFunctions[]              = "ExportNamedFunctions";
const char _FavorSizeOrSpeed[]                  = "FavorSizeOrSpeed";
const char _FloatingPointModel[]                = "FloatingPointModel";
const char _FloatingPointExceptions[]           = "FloatingPointExceptions";
const char _ForceConformanceInForLoopScope[]    = "ForceConformanceInForLoopScope";
const char _ForceSymbolReferences[]             = "ForceSymbolReferences";
const char _ForcedIncludeFiles[]                = "ForcedIncludeFiles";
const char _ForcedUsingFiles[]                  = "ForcedUsingFiles";
const char _FullIncludePath[]                   = "FullIncludePath";
const char _FunctionOrder[]                     = "FunctionOrder";
const char _GenerateDebugInformation[]          = "GenerateDebugInformation";
const char _GenerateMapFile[]                   = "GenerateMapFile";
const char _GeneratePreprocessedFile[]          = "GeneratePreprocessedFile";
const char _GenerateStublessProxies[]           = "GenerateStublessProxies";
const char _GenerateTypeLibrary[]               = "GenerateTypeLibrary";
const char _GlobalOptimizations[]               = "GlobalOptimizations";
const char _HeaderFileName[]                    = "HeaderFileName";
const char _HeapCommitSize[]                    = "HeapCommitSize";
const char _HeapReserveSize[]                   = "HeapReserveSize";
const char _IgnoreAllDefaultLibraries[]         = "IgnoreAllDefaultLibraries";
const char _IgnoreDefaultLibraryNames[]         = "IgnoreDefaultLibraryNames";
const char _IgnoreEmbeddedIDL[]                 = "IgnoreEmbeddedIDL";
const char _IgnoreImportLibrary[]               = "IgnoreImportLibrary";
const char _IgnoreStandardIncludePath[]         = "IgnoreStandardIncludePath";
const char _ImportLibrary[]                     = "ImportLibrary";
const char _ImproveFloatingPointConsistency[]   = "ImproveFloatingPointConsistency";
const char _InlineFunctionExpansion[]           = "InlineFunctionExpansion";
const char _InterfaceIdentifierFileName[]       = "InterfaceIdentifierFileName";
const char _IntermediateDirectory[]             = "IntermediateDirectory";
const char _KeepComments[]                      = "KeepComments";
const char _LargeAddressAware[]                 = "LargeAddressAware";
const char _LinkDLL[]                           = "LinkDLL";
const char _LinkIncremental[]                   = "LinkIncremental";
const char _LinkTimeCodeGeneration[]            = "LinkTimeCodeGeneration";
const char _LinkToManagedResourceFile[]         = "LinkToManagedResourceFile";
const char _MapExports[]                        = "MapExports";
const char _MapFileName[]                       = "MapFileName";
const char _MapLines[]                          = "MapLines ";
const char _MergeSections[]                     = "MergeSections";
const char _MergedIDLBaseFileName[]             = "MergedIDLBaseFileName";
const char _MidlCommandFile[]                   = "MidlCommandFile";
const char _MinimalRebuild[]                    = "MinimalRebuild";
const char _MkTypLibCompatible[]                = "MkTypLibCompatible";
const char _ModuleDefinitionFile[]              = "ModuleDefinitionFile";
const char _Name[]                              = "Name";
const char _ObjectFile[]                        = "ObjectFile";
const char _OmitFramePointers[]                 = "OmitFramePointers";
const char _OpenMP[]                            = "OpenMP";
const char _Optimization[]                      = "Optimization ";
const char _OptimizeForProcessor[]              = "OptimizeForProcessor";
const char _OptimizeForWindows98[]              = "OptimizeForWindows98";
const char _OptimizeForWindowsApplication[]     = "OptimizeForWindowsApplication";
const char _OptimizeReferences[]                = "OptimizeReferences";
const char _OutputDirectory[]                   = "OutputDirectory";
const char _OutputFile[]                        = "OutputFile";
const char _Outputs[]                           = "Outputs";
const char _ParseFiles[]                        = "ParseFiles";
const char _PrecompiledHeaderFile[]             = "PrecompiledHeaderFile";
const char _PrecompiledHeaderThrough[]          = "PrecompiledHeaderThrough";
const char _PreprocessorDefinitions[]           = "PreprocessorDefinitions";
const char _PrimaryOutput[]                     = "PrimaryOutput";
const char _ProjectGUID[]                       = "ProjectGUID";
const char _Keyword[]                           = "Keyword";
const char _ProjectType[]                       = "ProjectType";
const char _ProgramDatabase[]                   = "ProgramDatabase";
const char _ProgramDataBaseFileName[]           = "ProgramDataBaseFileName";
const char _ProgramDatabaseFile[]               = "ProgramDatabaseFile";
const char _ProxyFileName[]                     = "ProxyFileName";
const char _RedirectOutputAndErrors[]           = "RedirectOutputAndErrors";
const char _RegisterOutput[]                    = "RegisterOutput";
const char _RelativePath[]                      = "RelativePath";
const char _RemoteDirectory[]                   = "RemoteDirectory";
const char _ResourceOnlyDLL[]                   = "ResourceOnlyDLL";
const char _ResourceOutputFileName[]            = "ResourceOutputFileName";
const char _RuntimeLibrary[]                    = "RuntimeLibrary";
const char _RuntimeTypeInfo[]                   = "RuntimeTypeInfo";
const char _SccProjectName[]                    = "SccProjectName";
const char _SccLocalPath[]                      = "SccLocalPath";
const char _SetChecksum[]                       = "SetChecksum";
const char _ShowIncludes[]                      = "ShowIncludes";
const char _ShowProgress[]                      = "ShowProgress";
const char _SmallerTypeCheck[]                  = "SmallerTypeCheck";
const char _StackCommitSize[]                   = "StackCommitSize";
const char _StackReserveSize[]                  = "StackReserveSize";
const char _StringPooling[]                     = "StringPooling";
const char _StripPrivateSymbols[]               = "StripPrivateSymbols";
const char _StructMemberAlignment[]             = "StructMemberAlignment";
const char _SubSystem[]                         = "SubSystem";
const char _SupportUnloadOfDelayLoadedDLL[]     = "SupportUnloadOfDelayLoadedDLL";
const char _SuppressStartupBanner[]             = "SuppressStartupBanner";
const char _SwapRunFromCD[]                     = "SwapRunFromCD";
const char _SwapRunFromNet[]                    = "SwapRunFromNet";
const char _TargetEnvironment[]                 = "TargetEnvironment";
const char _TargetMachine[]                     = "TargetMachine";
const char _TerminalServerAware[]               = "TerminalServerAware";
const char _Path[]                              = "Path";
const char _TreatWChar_tAsBuiltInType[]         = "TreatWChar_tAsBuiltInType";
const char _TurnOffAssemblyGeneration[]         = "TurnOffAssemblyGeneration";
const char _TypeLibraryFile[]                   = "TypeLibraryFile";
const char _TypeLibraryName[]                   = "TypeLibraryName";
const char _TypeLibraryResourceID[]             = "TypeLibraryResourceID";
const char _UndefineAllPreprocessorDefinitions[]= "UndefineAllPreprocessorDefinitions";
const char _UndefinePreprocessorDefinitions[]   = "UndefinePreprocessorDefinitions";
const char _UniqueIdentifier[]                  = "UniqueIdentifier";
const char _UseOfATL[]                          = "UseOfATL";
const char _UseOfMfc[]                          = "UseOfMfc";
const char _UsePrecompiledHeader[]              = "UsePrecompiledHeader";
const char _ValidateParameters[]                = "ValidateParameters";
const char _VCCLCompilerTool[]                  = "VCCLCompilerTool";
const char _VCLibrarianTool[]                   = "VCLibrarianTool";
const char _VCLinkerTool[]                      = "VCLinkerTool";
const char _VCCustomBuildTool[]                 = "VCCustomBuildTool";
const char _VCResourceCompilerTool[]            = "VCResourceCompilerTool";
const char _VCMIDLTool[]                        = "VCMIDLTool";
const char _Version[]                           = "Version";
const char _WarnAsError[]                       = "WarnAsError";
const char _WarningLevel[]                      = "WarningLevel";
const char _WholeProgramOptimization[]          = "WholeProgramOptimization";
const char _CompileForArchitecture[]            = "CompileForArchitecture";
const char _InterworkCalls[]                    = "InterworkCalls";

// XmlOutput stream functions ------------------------------
inline XmlOutput::xml_output attrT(const char *name, const triState v)
{
    if(v == unset)
        return noxml();
    return attr(name, (v == _True ? "true" : "false"));
}

inline XmlOutput::xml_output attrE(const char *name, int v)
{
    return attr(name, QString::number(v));
}

/*ifNot version*/
inline XmlOutput::xml_output attrE(const char *name, int v, int ifn)
{
    if (v == ifn)
        return noxml();
    return attr(name, QString::number(v));
}

inline XmlOutput::xml_output attrL(const char *name, qint64 v)
{
    return attr(name, QString::number(v));
}

/*ifNot version*/
inline XmlOutput::xml_output attrL(const char *name, qint64 v, qint64 ifn)
{
    if (v == ifn)
        return noxml();
    return attr(name, QString::number(v));
}

inline XmlOutput::xml_output attrS(const char *name, const QString &v)
{
    if(v.isEmpty())
        return noxml();
    return attr(name, v);
}

inline XmlOutput::xml_output attrX(const char *name, const QStringList &v, const char *s = ",")
{
    if(v.isEmpty())
        return noxml();
    return attr(name, v.join(s));
}

triState operator!(const triState &rhs)
{
    if (rhs == unset)
        return rhs;
    triState lhs = (rhs == _True ? _False : _True);
    return lhs;
}

// VCToolBase -------------------------------------------------
QStringList VCToolBase::fixCommandLine(const QString &input)
{
    // The splitting regexp is a bit bizarre for backwards compat reasons (why else ...).
    return input.split(QRegExp(QLatin1String("\n\t|\r\\\\h|\r\n")));
}

static QString vcCommandSeparator()
{
    // MSVC transforms the build tree into a single batch file, simply pasting the contents
    // of the custom commands into it, and putting an "if errorlevel goto" statement behind it.
    // As we want every sub-command to be error-checked (as is done by makefile-based
    // backends), we insert the checks ourselves, using the undocumented jump target.
    static QString cmdSep =
    QLatin1String("&#x000D;&#x000A;if errorlevel 1 goto VCReportError&#x000D;&#x000A;");
    return cmdSep;
}

// VCCLCompilerTool -------------------------------------------------
VCCLCompilerTool::VCCLCompilerTool()
    :        AssemblerOutput(asmListingNone),
        BasicRuntimeChecks(runtimeBasicCheckNone),
        BrowseInformation(brInfoNone),
        BufferSecurityCheck(_False),
        CallingConvention(callConventionDefault),
        CompileAs(compileAsDefault),
        CompileAsManaged(managedDefault),
        CompileOnly(unset),
        DebugInformationFormat(debugDisabled),
        DefaultCharIsUnsigned(unset),
        Detect64BitPortabilityProblems(unset),
        DisableLanguageExtensions(unset),
        EnableEnhancedInstructionSet(archNotSet),
        EnableFiberSafeOptimizations(unset),
        EnableFunctionLevelLinking(unset),
        EnableIntrinsicFunctions(unset),
        ExceptionHandling(ehDefault),
        ExpandAttributedSource(unset),
        FavorSizeOrSpeed(favorNone),
        FloatingPointModel(floatingPointNotSet),
        FloatingPointExceptions(unset),
        ForceConformanceInForLoopScope(unset),
        GeneratePreprocessedFile(preprocessNo),
        PreprocessSuppressLineNumbers(unset),
        GlobalOptimizations(unset),
        IgnoreStandardIncludePath(unset),
        ImproveFloatingPointConsistency(unset),
        InlineFunctionExpansion(expandDefault),
        KeepComments(unset),
        MinimalRebuild(unset),
        OmitDefaultLibName(unset),
        OmitFramePointers(unset),
        OpenMP(unset),
        Optimization(optimizeCustom),
        OptimizeForProcessor(procOptimizeBlended),
        OptimizeForWindowsApplication(unset),
        ProgramDataBaseFileName(""),
        RuntimeLibrary(rtMultiThreaded),
        RuntimeTypeInfo(unset),
        ShowIncludes(unset),
        SmallerTypeCheck(unset),
        StringPooling(unset),
        StructMemberAlignment(alignNotSet),
        SuppressStartupBanner(unset),
        TreatWChar_tAsBuiltInType(unset),
        TurnOffAssemblyGeneration(unset),
        UndefineAllPreprocessorDefinitions(unset),
        UsePrecompiledHeader(pchUnset),
        UseUnicodeForAssemblerListing(unset),
        WarnAsError(unset),
        WarningLevel(warningLevel_0),
        WholeProgramOptimization(unset),
        CompileForArchitecture(archUnknown),
        InterworkCalls(unset),
        EnablePREfast(unset),
        DisplayFullPaths(unset),
        MultiProcessorCompilation(unset),
        GenerateXMLDocumentationFiles(unset),
        CreateHotpatchableImage(unset)
{
}

/*
 * Some values for the attribute UsePrecompiledHeader have changed from VS 2003 to VS 2005,
 * see the following chart, so we need a function that transforms those values if we are
 * using NET2005:
 *
 * Meaning                      2003    2005
 * -----------------------------------------
 * Don't use PCH                0       0
 * Create PCH (/Yc)             1       1
 * Automatically generate (/YX) 2       (seems that it was removed)
 * Use specific PCH (/Yu)       3       2
 *
 */
inline XmlOutput::xml_output xformUsePrecompiledHeaderForNET2005(pchOption whatPch, DotNET compilerVersion)
{
    if (compilerVersion >= NET2005) {
        if (whatPch ==  pchGenerateAuto) whatPch = (pchOption)0;
        if (whatPch ==  pchUseUsingSpecific) whatPch = (pchOption)2;
    }
    return attrE(_UsePrecompiledHeader, whatPch, /*ifNot*/ pchUnset);
}

inline XmlOutput::xml_output xformExceptionHandlingNET2005(exceptionHandling eh, DotNET compilerVersion)
{
    if (eh == ehDefault)
        return noxml();

    if (compilerVersion >= NET2005)
        return attrE(_ExceptionHandling, eh);

    return attrS(_ExceptionHandling, (eh == ehNoSEH ? "true" : "false"));
}

bool VCCLCompilerTool::parseOption(const char* option)
{
    // skip index 0 ('/' or '-')
    char first  = option[1];
    char second = option[2];
    char third  = option[3];
    char fourth = option[4];
    bool found = true;

    switch (first) {
    case '?':
    case 'h':
        if(second == 'o' && third == 't' && fourth == 'p') {
            CreateHotpatchableImage = _True;
            break;
        }
        qWarning("Generator: Option '/?', '/help': MSVC.NET projects do not support outputting help info");
        found = false;
        break;
    case '@':
        qWarning("Generator: Option '/@': MSVC.NET projects do not support the use of a response file");
        found = false;
        break;
    case 'l':
        qWarning("Generator: Option '/link': qmake generator does not support passing link options through the compiler tool");
        found = false;
        break;
    case 'A':
        if(second != 'I') {
            found = false; break;
        }
        AdditionalUsingDirectories += option+3;
        break;
    case 'C':
        KeepComments = _True;
        break;
    case 'D':
        PreprocessorDefinitions += option+2;
        break;
    case 'E':
        if(second == 'H') {
            QByteArray opt(option + 2);
            if (opt.contains('a') && !opt.contains('s') && !opt.contains('c'))
                ExceptionHandling = ehSEH;
            else if (!opt.contains('a') && opt.contains("s-") && opt.contains("c-"))
                ExceptionHandling = ehNone;
            else if (!opt.contains('a') && opt.contains('s') && opt.contains('c'))
                ExceptionHandling = ehNoSEH;
            else {
                // ExceptionHandling must be false, or it will override
                // with an /EHsc option
                ExceptionHandling = ehNone;
                AdditionalOptions += option;
            }
            if (config->CompilerVersion < NET2005
                && ExceptionHandling == ehSEH) {
                ExceptionHandling = ehNone;
                AdditionalOptions += option;
            }
            break;
        } else if (second == 'P') {
            PreprocessSuppressLineNumbers = _True;
        }
        GeneratePreprocessedFile = preprocessYes;
        break;
    case 'F':
        if(second <= '9' && second >= '0') {
            AdditionalOptions += option;
            break;
        } else {
            switch (second) {
            case 'A':
                if(third == 'c') {
                    AssemblerOutput = asmListingAsmMachine;
                    if(fourth == 's')
                        AssemblerOutput = asmListingAsmMachineSrc;
                } else if(third == 's') {
                    AssemblerOutput = asmListingAsmSrc;
                } else if (third == 'u') {
                    UseUnicodeForAssemblerListing = _True;
                } else {
                    AssemblerOutput = asmListingAssemblyOnly;
                }
                break;
            case 'C':
                DisplayFullPaths = _True;
                break;
            case 'a':
                AssemblerListingLocation = option+3;
                break;
            case 'I':
                ForcedIncludeFiles += option+3;
                break;
            case 'i':
                PreprocessOutputPath += option+3;
                break;
            case 'R':
                BrowseInformation = brAllInfo;
                BrowseInformationFile = option+3;
                break;
            case 'r':
                BrowseInformation = brNoLocalSymbols;
                BrowseInformationFile = option+3;
                break;
            case 'U':
                ForcedUsingFiles += option+3;
                break;
            case 'd':
                ProgramDataBaseFileName = option+3;
                break;
            case 'e':
                OutputFile = option+3;
                break;
            case 'm':
                AdditionalOptions += option;
                break;
            case 'o':
                ObjectFile = option+3;
                break;
            case 'p':
                PrecompiledHeaderFile = option+3;
                break;
            case 'x':
                ExpandAttributedSource = _True;
                break;
            default:
                found = false; break;
            }
        }
        break;
    case 'G':
        switch (second) {
        case '3':
        case '4':
            qWarning("Option '/G3' and '/G4' were phased out in Visual C++ 5.0");
            found = false; break;
        case '5':
            OptimizeForProcessor = procOptimizePentium;
            break;
        case '6':
        case 'B':
            OptimizeForProcessor = procOptimizePentiumProAndAbove;
            break;
        case '7':
            OptimizeForProcessor = procOptimizePentium4AndAbove;
            break;
        case 'A':
            OptimizeForWindowsApplication = _True;
            break;
        case 'F':
            StringPooling = _True;
            break;
        case 'H':
            AdditionalOptions += option;
            break;
        case 'L':
            WholeProgramOptimization = _True;
            if(third == '-')
                WholeProgramOptimization = _False;
            break;
        case 'R':
            RuntimeTypeInfo = _True;
            if(third == '-')
                RuntimeTypeInfo = _False;
            break;
        case 'S':
            BufferSecurityCheck = _True;
            if(third == '-')
                BufferSecurityCheck = _False;
            break;
        case 'T':
            EnableFiberSafeOptimizations = _True;
            break;
        case 'X':
            // Same as the /EHsc option, which is Exception Handling without SEH
            ExceptionHandling = ehNoSEH;
            if (third == '-')
                ExceptionHandling = ehNone;
            break;
        case 'Z':
        case 'e':
        case 'h':
            AdditionalOptions += option;
            break;
        case 'd':
            CallingConvention = callConventionCDecl;
            break;
        case 'f':
            StringPooling = _True;
            AdditionalOptions += option;
            break;
        case 'm':
            MinimalRebuild = _True;
            if(third == '-')
                MinimalRebuild = _False;
            break;
        case 'r':
            CallingConvention = callConventionFastCall;
            break;
        case 's':
            // Warning: following [num] is not used,
            // were should we put it?
            BufferSecurityCheck = _True;
            break;
        case 'y':
            EnableFunctionLevelLinking = _True;
            break;
        case 'z':
            CallingConvention = callConventionStdCall;
            break;
        default:
            found = false; break;
        }
        break;
    case 'H':
        AdditionalOptions += option;
        break;
    case 'I':
        AdditionalIncludeDirectories += option+2;
        break;
    case 'J':
        DefaultCharIsUnsigned = _True;
        break;
    case 'L':
        if(second == 'D') {
            AdditionalOptions += option;
            break;
        }
        found = false; break;
    case 'M':
        if(second == 'D') {
            RuntimeLibrary = rtMultiThreadedDLL;
            if(third == 'd')
                RuntimeLibrary = rtMultiThreadedDebugDLL;
            break;
        } else if(second == 'L') {
            RuntimeLibrary = rtSingleThreaded;
            if(third == 'd')
                RuntimeLibrary = rtSingleThreadedDebug;
            break;
        } else if(second == 'T') {
            RuntimeLibrary = rtMultiThreaded;
            if(third == 'd')
                RuntimeLibrary = rtMultiThreadedDebug;
            break;
        } else if (second == 'P') {
            if (config->CompilerVersion >= NET2010) {
                MultiProcessorCompilation = _True;
                MultiProcessorCompilationProcessorCount = option+3;
            } else if (config->CompilerVersion >= NET2005) {
                AdditionalOptions += option;
            } else {
                warn_msg(WarnLogic, "/MP option is not supported in Visual C++ < 2005, ignoring.");
            }
            break;
        }
        found = false; break;
    case 'O':
        switch (second) {
        case '1':
            Optimization = optimizeMinSpace;
            break;
        case '2':
            Optimization = optimizeMaxSpeed;
            break;
        case 'a':
            AdditionalOptions += option;
            break;
        case 'b':
            if(third == '0')
                InlineFunctionExpansion = expandDisable;
            else if(third == '1')
                InlineFunctionExpansion = expandOnlyInline;
            else if(third == '2')
                InlineFunctionExpansion = expandAnySuitable;
            else
                found = false;
            break;
        case 'd':
            Optimization = optimizeDisabled;
            break;
        case 'g':
            GlobalOptimizations = _True;
            break;
        case 'i':
            EnableIntrinsicFunctions = _True;
            break;
        case 'p':
            ImproveFloatingPointConsistency = _True;
            if(third == '-')
                ImproveFloatingPointConsistency = _False;
            break;
        case 's':
            FavorSizeOrSpeed = favorSize;
            break;
        case 't':
            FavorSizeOrSpeed = favorSpeed;
            break;
        case 'w':
            AdditionalOptions += option;
            break;
        case 'x':
            Optimization = optimizeFull;
            break;
        case 'y':
            OmitFramePointers = _True;
            if(third == '-')
                OmitFramePointers = _False;
            break;
        default:
            found = false; break;
        }
        break;
    case 'P':
        GeneratePreprocessedFile = preprocessYes;
        break;
    case 'Q':
        if(second == 'I') {
            AdditionalOptions += option;
            break;
        } else if (second == 'R') {
            QString opt = option + 3;
            if (opt == "interwork-return") {
                InterworkCalls = _True;
                break;
            } else if (opt == "arch4") {
                CompileForArchitecture = archArmv4;
                break;
            } else if (opt == "arch5") {
                CompileForArchitecture = archArmv5;
                break;
            } else if (opt == "arch4T") {
                CompileForArchitecture = archArmv4T;
                break;
            } else if (opt == "arch5T") {
                CompileForArchitecture = archArmv5T;
                break;
            }
        } else if (second == 'M') {
            QString opt = option + 3;
            if (opt == "mips1") {
                CompileForArchitecture = archMips1;
                break;
            }
            else if (opt == "mips2") {
                CompileForArchitecture = archMips2;
                break;
            }
            else if (opt == "mips3") {
                CompileForArchitecture = archMips3;
                break;
            }
            else if (opt == "mips4") {
                CompileForArchitecture = archMips4;
                break;
            }
            else if (opt == "mips5") {
                CompileForArchitecture = archMips5;
                break;
            }
            else if (opt == "mips16") {
                CompileForArchitecture = archMips16;
                break;
            }
            else if (opt == "mips32") {
                CompileForArchitecture = archMips32;
                break;
            }
            else if (opt == "mips64") {
                CompileForArchitecture = archMips64;
                break;
            }
        }
        found = false; break;
    case 'R':
        if(second == 'T' && third == 'C') {
            if(fourth == '1')
                BasicRuntimeChecks = runtimeBasicCheckAll;
            else if(fourth == 'c')
                SmallerTypeCheck = _True;
            else if(fourth == 's')
                BasicRuntimeChecks = runtimeCheckStackFrame;
            else if(fourth == 'u')
                BasicRuntimeChecks = runtimeCheckUninitVariables;
            else
                found = false; break;
        }
        break;
    case 'T':
        if(second == 'C') {
            CompileAs = compileAsC;
        } else if(second == 'P') {
            CompileAs = compileAsCPlusPlus;
        } else {
            qWarning("Generator: Options '/Tp<filename>' and '/Tc<filename>' are not supported by qmake");
            found = false; break;
        }
        break;
    case 'U':
        UndefinePreprocessorDefinitions += option+2;
        break;
    case 'V':
        AdditionalOptions += option;
        break;
    case 'W':
        switch (second) {
        case 'a':
        case '4':
            WarningLevel = warningLevel_4;
            break;
        case '3':
            WarningLevel = warningLevel_3;
            break;
        case '2':
            WarningLevel = warningLevel_2;
            break;
        case '1':
            WarningLevel = warningLevel_1;
            break;
        case '0':
            WarningLevel = warningLevel_0;
            break;
        case 'L':
            AdditionalOptions += option;
            break;
        case 'X':
            WarnAsError = _True;
            break;
        case 'p':
            if(third == '6' && fourth == '4') {
                if (config->CompilerVersion >= NET2010) {
                     // Deprecated for VS2010 but can be used under Additional Options.
                    AdditionalOptions += option;
                } else {
                   Detect64BitPortabilityProblems = _True;
                }
                break;
            }
            // Fallthrough
        default:
            found = false; break;
        }
        break;
    case 'X':
        IgnoreStandardIncludePath = _True;
        break;
    case 'Y':
        switch (second) {
        case '\0':
        case '-':
            AdditionalOptions += option;
            break;
        case 'X':
            UsePrecompiledHeader = pchGenerateAuto;
            PrecompiledHeaderThrough = option+3;
            break;
        case 'c':
            UsePrecompiledHeader = pchCreateUsingSpecific;
            PrecompiledHeaderThrough = option+3;
            break;
        case 'd':
        case 'l':
            AdditionalOptions += option;
            break;
        case 'u':
            UsePrecompiledHeader = pchUseUsingSpecific;
            PrecompiledHeaderThrough = option+3;
            break;
        default:
            found = false; break;
        }
        break;
    case 'Z':
        switch (second) {
        case '7':
            DebugInformationFormat = debugOldStyleInfo;
            break;
        case 'I':
            DebugInformationFormat = debugEditAndContinue;
            break;
        case 'd':
            DebugInformationFormat = debugLineInfoOnly;
            break;
        case 'i':
            DebugInformationFormat = debugEnabled;
            break;
        case 'l':
            OmitDefaultLibName = _True;
            break;
        case 'a':
            DisableLanguageExtensions = _True;
            break;
        case 'e':
            DisableLanguageExtensions = _False;
            break;
        case 'c':
            if(third == ':') {
                const char *c = option + 4;
                // Go to the end of the option
                while ( *c != '\0' && *c != ' ' && *c != '-')
                    ++c;
                if(fourth == 'f')
                    ForceConformanceInForLoopScope = ((*c) == '-' ? _False : _True);
                else if(fourth == 'w')
                    TreatWChar_tAsBuiltInType = ((*c) == '-' ? _False : _True);
                else
                    found = false;
            } else {
                found = false; break;
            }
            break;
        case 'g':
        case 'm':
        case 's':
            AdditionalOptions += option;
            break;
        case 'p':
            switch (third)
            {
            case '\0':
            case '1':
                StructMemberAlignment = alignSingleByte;
                if(fourth == '6')
                    StructMemberAlignment = alignSixteenBytes;
                break;
            case '2':
                StructMemberAlignment = alignTwoBytes;
                break;
            case '4':
                StructMemberAlignment = alignFourBytes;
                break;
            case '8':
                StructMemberAlignment = alignEightBytes;
                break;
            default:
                found = false; break;
            }
            break;
        default:
            found = false; break;
        }
        break;
    case 'a':
        if (second == 'r' && third == 'c' && fourth == 'h') {
            if (option[5] == ':') {
                const char *o = option;
                if (o[6] == 'S' && o[7] == 'S' && o[8] == 'E') {
                    EnableEnhancedInstructionSet = o[9] == '2' ? archSSE2 : archSSE;
                    break;
                }
            }
        } else if (second == 'n' && third == 'a' && fourth == 'l') {
            EnablePREfast = _True;
            break;
        }
        found = false;
        break;
    case 'b':   // see http://msdn2.microsoft.com/en-us/library/ms173499.aspx
        if (second == 'i' && third == 'g' && fourth == 'o') {
            const char *o = option;
            if (o[5] == 'b' && o[6] == 'j') {
                AdditionalOptions += option;
                break;
            }
        }
        found = false;
        break;
    case 'c':
        if(second == '\0') {
            CompileOnly = _True;
        } else if(second == 'l') {
            if (config->CompilerVersion < NET2005) {
                if(*(option+5) == 'n') {
                    CompileAsManaged = managedAssemblyPure;
                    TurnOffAssemblyGeneration = _True;
                } else if(*(option+5) == 'p') {
                    CompileAsManaged = managedAssemblyPure;
                    warn_msg(WarnLogic, "/clr:pure option only for .NET >= 2005, using /clr");
                } else if(*(option+5) == 's') {
                    CompileAsManaged = managedAssemblyPure;
                    warn_msg(WarnLogic, "/clr:safe option only for .NET >= 2005, using /clr");
                } else if(*(option+5) == 'o') {
                    CompileAsManaged = managedAssemblyPure;
                    warn_msg(WarnLogic, "/clr:oldSyntax option only for .NET >= 2005, using /clr");
                } else if(*(option+5) == 'i') {
                    CompileAsManaged = managedAssemblyPure;
                    warn_msg(WarnLogic, "initialAppDomain enum value unknown, using /crl");
                } else {
                    CompileAsManaged = managedAssemblyPure;
                }
            } else {
                if(*(option+5) == 'n') {
                    CompileAsManaged = managedAssembly;
                    TurnOffAssemblyGeneration = _True;
                } else if(*(option+5) == 'p') {
                    CompileAsManaged = managedAssemblyPure;
                } else if(*(option+5) == 's') {
                    CompileAsManaged = managedAssemblySafe;
                } else if(*(option+5) == 'o') {
                    CompileAsManaged = managedAssemblyOldSyntax;
                } else if(*(option+5) == 'i') {
                    CompileAsManaged = managedAssembly;
                    warn_msg(WarnLogic, "initialAppDomain enum value unknown, using /crl default");
                } else {
                    CompileAsManaged = managedAssembly;
                }
            }
        } else {
            found = false; break;
        }
        break;
    case 'd':
        if (second == 'r') {
            CompileAsManaged = managedAssembly;
            break;
        } else if (second != 'o' && third == 'c') {
            GenerateXMLDocumentationFiles = _True;
            XMLDocumentationFileName += option+4;
            break;
        }
        found = false;
        break;
    case 'e':
        if (second == 'r' && third == 'r' && fourth == 'o') {
            if (option[12] == ':') {
                if ( option[13] == 'n') {
                    ErrorReporting = "None";
                } else if (option[13] == 'p') {
                    ErrorReporting = "Prompt";
                } else if (option[13] == 'q') {
                    ErrorReporting = "Queue";
                } else if (option[13] == 's') {
                    ErrorReporting = "Send";
                } else {
                    found = false;
                }
                break;
            }
        }
        found = false;
        break;
    case 'f':
        if(second == 'p' && third == ':') {
            // Go to the end of the option
            const char *c = option + 4;
            while (*c != '\0' && *c != ' ' && *c != '-')
                ++c;
            switch (fourth) {
            case 'e':
                FloatingPointExceptions = ((*c) == '-' ? _False : _True);
                break;
            case 'f':
                FloatingPointModel = floatingPointFast;
                break;
            case 'p':
                FloatingPointModel = floatingPointPrecise;
                break;
            case 's':
                FloatingPointModel = floatingPointStrict;
                break;
            default:
                found = false;
                break;
            }
        }
        break;
    case 'n':
        if(second == 'o' && third == 'B' && fourth == 'o') {
            AdditionalOptions += "/noBool";
            break;
        }
        if(second == 'o' && third == 'l' && fourth == 'o') {
            SuppressStartupBanner = _True;
            break;
        }
        found = false; break;
    case 'o':
        if (second == 'p' && third == 'e' && fourth == 'n') {
            OpenMP = _True;
            break;
        }
        found = false; break;
    case 's':
        if(second == 'h' && third == 'o' && fourth == 'w') {
            ShowIncludes = _True;
            break;
        }
        found = false; break;
    case 'u':
        UndefineAllPreprocessorDefinitions = _True;
        break;
    case 'v':
        if(second == 'd' || second == 'm') {
            AdditionalOptions += option;
            break;
        }
        found = false; break;
    case 'w':
        switch (second) {
        case '\0':
            WarningLevel = warningLevel_0;
            break;
        case 'd':
            DisableSpecificWarnings += option+3;
            break;
        default:
            AdditionalOptions += option;
        }
        break;
    default:
        AdditionalOptions += option;
        break;
    }
    if(!found) {
        warn_msg(WarnLogic, "Could not parse Compiler option: %s, added as AdditionalOption", option);
        AdditionalOptions += option;
    }
    return true;
}

// VCLinkerTool -----------------------------------------------------
VCLinkerTool::VCLinkerTool()
    :   DataExecutionPrevention(unset),
        EnableCOMDATFolding(optFoldingDefault),
        GenerateDebugInformation(unset),
        GenerateMapFile(unset),
        HeapCommitSize(-1),
        HeapReserveSize(-1),
        IgnoreAllDefaultLibraries(unset),
        IgnoreEmbeddedIDL(unset),
        IgnoreImportLibrary(_True),
        LargeAddressAware(addrAwareDefault),
        LinkDLL(unset),
        LinkIncremental(linkIncrementalDefault),
        LinkTimeCodeGeneration(optLTCGDefault),
        MapExports(unset),
        MapLines(unset),
        OptimizeForWindows98(optWin98Default),
        OptimizeReferences(optReferencesDefault),
        RandomizedBaseAddress(unset),
        RegisterOutput(unset),
        ResourceOnlyDLL(unset),
        SetChecksum(unset),
        ShowProgress(linkProgressNotSet),
        StackCommitSize(-1),
        StackReserveSize(-1),
        SubSystem(subSystemNotSet),
        SupportUnloadOfDelayLoadedDLL(unset),
        SuppressStartupBanner(unset),
        SwapRunFromCD(unset),
        SwapRunFromNet(unset),
        TargetMachine(machineNotSet),
        TerminalServerAware(termSvrAwareDefault),
        TreatWarningsAsErrors(unset),
        TurnOffAssemblyGeneration(unset),
        TypeLibraryResourceID(0),
        GenerateManifest(unset),
        EnableUAC(unset),
        UACUIAccess(unset),
        SectionAlignment(-1),
        PreventDllBinding(unset),
        AllowIsolation(unset),
        AssemblyDebug(unset),
        CLRUnmanagedCodeCheck(unset),
        DelaySign(unset)
{
}

// Hashing routine to do fast option lookups ----
// Slightly rewritten to stop on ':' ',' and '\0'
// Original routine in qtranslator.cpp ----------
static uint elfHash(const char* name)
{
    const uchar *k;
    uint h = 0;
    uint g;

    if(name) {
        k = (const uchar *) name;
        while((*k) &&
                (*k)!= ':' &&
                (*k)!=',' &&
                (*k)!=' ') {
            h = (h << 4) + *k++;
            if((g = (h & 0xf0000000)) != 0)
                h ^= g >> 24;
            h &= ~g;
        }
    }
    if(!h)
        h = 1;
    return h;
}

//#define USE_DISPLAY_HASH
#ifdef USE_DISPLAY_HASH
static void displayHash(const char* str)
{
    printf("case 0x%07x: // %s\n    break;\n", elfHash(str), str);
}
#endif

bool VCLinkerTool::parseOption(const char* option)
{
#ifdef USE_DISPLAY_HASH
    // Main options
    displayHash("/ALIGN"); displayHash("/ALLOWBIND"); displayHash("/ASSEMBLYMODULE");
    displayHash("/ASSEMBLYRESOURCE"); displayHash("/BASE"); displayHash("/DEBUG");
    displayHash("/DEF"); displayHash("/DEFAULTLIB"); displayHash("/DELAY");
    displayHash("/DELAYLOAD"); displayHash("/DLL"); displayHash("/DRIVER");
    displayHash("/ENTRY"); displayHash("/EXETYPE"); displayHash("/EXPORT");
    displayHash("/FIXED"); displayHash("/FORCE"); displayHash("/HEAP");
    displayHash("/IDLOUT"); displayHash("/IGNORE"); displayHash("/IGNOREIDL"); displayHash("/IMPLIB");
    displayHash("/INCLUDE"); displayHash("/INCREMENTAL"); displayHash("/LARGEADDRESSAWARE");
    displayHash("/LIBPATH"); displayHash("/LTCG"); displayHash("/MACHINE");
    displayHash("/MAP"); displayHash("/MAPINFO"); displayHash("/MERGE");
    displayHash("/MIDL"); displayHash("/NOASSEMBLY"); displayHash("/NODEFAULTLIB");
    displayHash("/NOENTRY"); displayHash("/NOLOGO"); displayHash("/OPT");
    displayHash("/ORDER"); displayHash("/OUT"); displayHash("/PDB");
    displayHash("/PDBSTRIPPED"); displayHash("/RELEASE"); displayHash("/SECTION");
    displayHash("/STACK"); displayHash("/STUB"); displayHash("/SUBSYSTEM");
    displayHash("/SWAPRUN"); displayHash("/TLBID"); displayHash("/TLBOUT");
    displayHash("/TSAWARE"); displayHash("/VERBOSE"); displayHash("/VERSION");
    displayHash("/VXD"); displayHash("/WS "); displayHash("/libpath");

#endif
#ifdef USE_DISPLAY_HASH
    // Sub options
    displayHash("UNLOAD"); displayHash("NOBIND"); displayHash("no"); displayHash("NOSTATUS"); displayHash("STATUS");
    displayHash("AM33"); displayHash("ARM"); displayHash("CEE"); displayHash("EBC"); displayHash("IA64"); displayHash("X86"); displayHash("X64"); displayHash("M32R");
    displayHash("MIPS"); displayHash("MIPS16"); displayHash("MIPSFPU"); displayHash("MIPSFPU16"); displayHash("MIPSR41XX"); displayHash("PPC");
    displayHash("SH3"); displayHash("SH3DSP"); displayHash("SH4"); displayHash("SH5"); displayHash("THUMB"); displayHash("TRICORE"); displayHash("EXPORTS");
    displayHash("LINES"); displayHash("REF"); displayHash("NOREF"); displayHash("ICF"); displayHash("WIN98"); displayHash("NOWIN98");
    displayHash("CONSOLE"); displayHash("EFI_APPLICATION"); displayHash("EFI_BOOT_SERVICE_DRIVER"); displayHash("EFI_ROM"); displayHash("EFI_RUNTIME_DRIVER"); displayHash("NATIVE");
    displayHash("POSIX"); displayHash("WINDOWS"); displayHash("WINDOWSCE"); displayHash("NET"); displayHash("CD"); displayHash("NO");
#endif
    bool found = true;
    const uint optionHash = elfHash(option);
    if (config->CompilerVersion < NET2010) {
        switch (optionHash) {
        case 0x3360dbe: // /ALIGN[:number]
        case 0x1485c34: // /ALLOWBIND[:NO]
        case 0x33aec94: // /FIXED[:NO]
        case 0x7988f7e: // /SECTION:name,[E][R][W][S][D][K][L][P][X][,ALIGN=#]
        case 0x0348992: // /STUB:filename
            AdditionalOptions += option;
            return true;
        }
    }

    switch (optionHash) {
    case 0x6b21972: // /DEFAULTLIB:library
    case 0x396ea92: // /DRIVER[:UPONLY | :WDM]
    case 0xaca9d75: // /EXETYPE[:DYNAMIC | :DEV386]
    case 0x3ad5444: // /EXPORT:entryname[,@ordinal[,NONAME]][,DATA]
    case 0x33b4675: // /FORCE:[MULTIPLE|UNRESOLVED]
    case 0x3dc3455: // /IGNORE:number,number,number,number  ### NOTE: This one is undocumented, but it is even used by Microsoft.
                    //                                      In recent versions of the Microsoft linker they have disabled this undocumented feature.
    case 0x0034bc4: // /VXD
        AdditionalOptions += option;
        break;
    case 0x3360dbe: // /ALIGN[:number]
        SectionAlignment = QString(option+7).toLongLong();
        break;
    case 0x1485c34: // /ALLOWBIND[:NO]
        if(*(option+10) == ':' && (*(option+11) == 'n' || *(option+11) == 'N'))
            PreventDllBinding = _False;
        else
            PreventDllBinding = _True;
        break;
    case 0x312011e: // /ALLOWISOLATION[:NO]
        if(*(option+15) == ':' && (*(option+16) == 'n' || *(option+16) == 'N'))
            AllowIsolation = _False;
        else
            AllowIsolation = _True;
        break;
    case 0x679c075: // /ASSEMBLYMODULE:filename
        AddModuleNamesToAssembly += option+15;
        break;
    case 0x75f35f7: // /ASSEMBLYDEBUG[:DISABLE]
        if(*(option+14) == ':' && (*(option+15) == 'D'))
            AssemblyDebug = _False;
        else
            AssemblyDebug = _True;
        break;
    case 0x43294a5: // /ASSEMBLYLINKRESOURCE:filename
            AssemblyLinkResource += option+22;
        break;
    case 0x062d065: // /ASSEMBLYRESOURCE:filename
        LinkToManagedResourceFile = option+18;
        break;
    case 0x0336675: // /BASE:{address | @filename,key}
        // Do we need to do a manual lookup when '@filename,key'?
        // Seems BaseAddress only can contain the location...
        // We don't use it in Qt, so keep it simple for now
        BaseAddress = option+6;
        break;
    case 0x63bf065: // /CLRIMAGETYPE:{IJW|PURE|SAFE}
        if(*(option+14) == 'I')
            CLRImageType = "ForceIJWImage";
        else if(*(option+14) == 'P')
            CLRImageType = "ForcePureILImage";
        else if(*(option+14) == 'S')
            CLRImageType = "ForceSafeILImage";
        break;
    case 0x5f2a6a2: // /CLRSUPPORTLASTERROR{:NO | SYSTEMDLL}
        if(*(option+20) == ':') {
            if(*(option+21) == 'N') {
                CLRSupportLastError = "Disabled";
            } else if(*(option+21) == 'S') {
                CLRSupportLastError = "SystemDlls";
            }
        } else {
            CLRSupportLastError = "Enabled";
        }
        break;
    case 0xc7984f5: // /CLRTHREADATTRIBUTE:{STA|MTA|NONE}
        if(*(option+20) == 'N')
            CLRThreadAttribute = "DefaultThreadingAttribute";
        else if(*(option+20) == 'M')
            CLRThreadAttribute = "MTAThreadingAttribute";
        else if(*(option+20) == 'S')
            CLRThreadAttribute = "STAThreadingAttribute";
        break;
    case 0xa8c637b: // /CLRUNMANAGEDCODECHECK[:NO]
        if(*(option+23) == 'N')
            CLRUnmanagedCodeCheck = _False;
        else
            CLRUnmanagedCodeCheck = _True;
        break;
    case 0x62d9e94: // /MANIFEST[:NO]
        if ((*(option+9) == ':' && (*(option+10) == 'N' || *(option+10) == 'n')))
            GenerateManifest = _False;
        else
            GenerateManifest = _True;
        break;
    case 0x8b64559: // /MANIFESTDEPENDENCY:manifest_dependency
        AdditionalManifestDependencies += option+20;
        break;
    case 0xe9e8195: // /MANIFESTFILE:filename
        ManifestFile = option+14;
        break;
    case 0x9e9fb83: // /MANIFESTUAC http://msdn.microsoft.com/en-us/library/bb384691%28VS.100%29.aspx
        if ((*(option+12) == ':' && (*(option+13) == 'N' || *(option+13) == 'n')))
            EnableUAC = _False;
        else if((*(option+12) == ':' && (*(option+13) == 'l' || *(option+14) == 'e'))) { // level
            if(*(option+20) == 'a')
                UACExecutionLevel = "AsInvoker";
            else if(*(option+20) == 'h')
                UACExecutionLevel = "HighestAvailable";
            else if(*(option+20) == 'r')
                UACExecutionLevel = "RequireAdministrator";
        } else if((*(option+12) == ':' && (*(option+13) == 'u' || *(option+14) == 'i'))) { // uiAccess
            if(*(option+22) == 't')
                UACUIAccess = _True;
            else
                UACUIAccess = _False;
        } else if((*(option+12) == ':' && (*(option+13) == 'f' || *(option+14) == 'r'))) { // fragment
            AdditionalOptions += option;
        }else
            EnableUAC = _True;
        break;
    case 0x3389797: // /DEBUG
        GenerateDebugInformation = _True;
        break;
    case 0x0033896: // /DEF:filename
        ModuleDefinitionFile = option+5;
        break;
    case 0x338a069: // /DELAY:{UNLOAD | NOBIND}
        // MS documentation does not specify what to do with
        // this option, so we'll put it in AdditionalOptions
        AdditionalOptions += option;
        break;
    case 0x06f4bf4: // /DELAYLOAD:dllname
        DelayLoadDLLs += option+11;
        break;
    case 0x06d451e: // /DELAYSIGN[:NO]
        if(*(option+10) == ':' && (*(option+11) == 'n' || *(option+11) == 'N'))
            DelaySign = _False;
        else
            DelaySign = _True;
        break;
    case 0x003390c: // /DLL
        // This option is not used for vcproj files
        break;
    case 0x2ee8415: // /DYNAMICBASE[:NO]
        if(*(option+12) == ':' && (*(option+13) == 'n' || *(option+13) == 'N'))
            RandomizedBaseAddress = _False;
        else
            RandomizedBaseAddress = _True;
        break;
    case 0x33a3979: // /ENTRY:function
        EntryPointSymbol = option+7;
        break;
    case 0x4504334: // /ERRORREPORT:[ NONE | PROMPT | QUEUE | SEND ]
        if(*(option+12) == ':' ) {
            if(*(option+13) == 'N')
                LinkErrorReporting = "NoErrorReport";
            else if(*(option+13) == 'P')
                LinkErrorReporting = "PromptImmediately";
            else if(*(option+13) == 'Q')
                LinkErrorReporting = "QueueForNextLogin";
            else if(*(option+13) == 'S')
                LinkErrorReporting = "SendErrorReport";
        }
        break;
    case 0x033c960: // /HEAP:reserve[,commit]
        {
            QStringList both = QString(option+6).split(",");
            HeapReserveSize = both[0].toLongLong();
            if(both.count() == 2)
                HeapCommitSize = both[1].toLongLong();
        }
        break;
    case 0x3d91494: // /IDLOUT:[path\]filename
        MergedIDLBaseFileName = option+8;
        break;
    case 0x345a04c: // /IGNOREIDL
        IgnoreEmbeddedIDL = _True;
        break;
    case 0x3e250e2: // /IMPLIB:filename
        ImportLibrary = option+8;
        break;
    case 0xe281ab5: // /INCLUDE:symbol
        ForceSymbolReferences += option+9;
        break;
    case 0xb28103c: // /INCREMENTAL[:no]
        if(*(option+12) == ':' &&
             (*(option+13) == 'n' || *(option+13) == 'N'))
            LinkIncremental = linkIncrementalNo;
        else
            LinkIncremental = linkIncrementalYes;
        break;
    case 0x07f1ab2: // /KEYCONTAINER:name
        KeyContainer = option+14;
        break;
    case 0xfadaf35: // /KEYFILE:filename
        KeyFile = option+9;
        break;
    case 0x26e4675: // /LARGEADDRESSAWARE[:no]
        if(*(option+18) == ':' &&
             *(option+19) == 'n')
            LargeAddressAware = addrAwareNoLarge;
        else
            LargeAddressAware = addrAwareLarge;
        break;
    case 0x2f96bc8: // /libpath:dir
    case 0x0d745c8: // /LIBPATH:dir
        AdditionalLibraryDirectories += option+9;
        break;
    case 0x0341877: // /LTCG[:NOSTATUS|:STATUS]
        config->WholeProgramOptimization = _True;
        if (config->CompilerVersion >= NET2005) {
            LinkTimeCodeGeneration = optLTCGEnabled;
            if(*(option+5) == ':') {
                const char* str = option+6;
                if (*str == 'S')
                    ShowProgress = linkProgressAll;
#ifndef Q_OS_WIN
                else if (strncasecmp(str, "pginstrument", 12))
                    LinkTimeCodeGeneration = optLTCGInstrument;
                else if (strncasecmp(str, "pgoptimize", 10))
                    LinkTimeCodeGeneration = optLTCGOptimize;
                else if (strncasecmp(str, "pgupdate", 8 ))
                    LinkTimeCodeGeneration = optLTCGUpdate;
#else
                else if (_stricmp(str, "pginstrument"))
                    LinkTimeCodeGeneration = optLTCGInstrument;
                else if (_stricmp(str, "pgoptimize"))
                    LinkTimeCodeGeneration = optLTCGOptimize;
                else if (_stricmp(str, "pgupdate"))
                    LinkTimeCodeGeneration = optLTCGUpdate;
#endif
            }
        } else {
            AdditionalOptions.append(option);
        }
        break;
	case 0x379ED25:
    case 0x157cf65: // /MACHINE:{AM33|ARM|CEE|IA64|X86|M32R|MIPS|MIPS16|MIPSFPU|MIPSFPU16|MIPSR41XX|PPC|SH3|SH4|SH5|THUMB|TRICORE}
        switch (elfHash(option+9)) {
        // Very limited documentation on all options but X86,
        case 0x0005bb6: // X86
            TargetMachine = machineX86;
            break;
        case 0x0005b94: // X64
            TargetMachine = machineX64;
            break;
        // so we put the others in AdditionalOptions...
        case 0x0046063: // AM33
        case 0x000466d: // ARM
        case 0x0004795: // CEE
        case 0x0004963: // EBC
        case 0x004d494: // IA64
        case 0x0050672: // M32R
        case 0x0051e53: // MIPS
        case 0x51e5646: // MIPS16
        case 0x1e57b05: // MIPSFPU
        case 0x57b09a6: // MIPSFPU16
        case 0x5852738: // MIPSR41XX
        case 0x0005543: // PPC
        case 0x00057b3: // SH3
        case 0x57b7980: // SH3DSP
        case 0x00057b4: // SH4
        case 0x00057b5: // SH5
        case 0x058da12: // THUMB
        case 0x96d8435: // TRICORE
        default:
            AdditionalOptions += option;
            break;
        }
        break;
    case 0x0034160: // /MAP[:filename]
        GenerateMapFile = _True;
        if (option[4] == ':')
            MapFileName = option+5;
        break;
    case 0x164e1ef: // /MAPINFO:{EXPORTS|LINES}
        if(*(option+9) == 'E')
            MapExports = _True;
        else if(*(option+9) == 'L')
            MapLines = _True;
        break;
    case 0x341a6b5: // /MERGE:from=to
        MergeSections = option+7;
        break;
    case 0x0341d8c: // /MIDL:@file
        MidlCommandFile = option+7;
        break;
    case 0x84e2679: // /NOASSEMBLY
        TurnOffAssemblyGeneration = _True;
        break;
    case 0x2b21942: // /NODEFAULTLIB[:library]
        if(*(option+13) == '\0')
            IgnoreAllDefaultLibraries = _True;
        else
            IgnoreDefaultLibraryNames += option+14;
        break;
    case 0x33a3a39: // /NOENTRY
        ResourceOnlyDLL = _True;
        break;
    case 0x434138f: // /NOLOGO
        SuppressStartupBanner = _True;
        break;
    case 0xc841054: // /NXCOMPAT[:NO]
        if ((*(option+9) == ':' && (*(option+10) == 'N' || *(option+10) == 'n')))
            DataExecutionPrevention = _False;
        else
            DataExecutionPrevention = _True;
        break;
    case 0x0034454: // /OPT:{REF | NOREF | ICF[=iterations] | NOICF | WIN98 | NOWIN98}
        {
            char third = *(option+7);
            switch (third) {
            case 'F': // REF
                if(*(option+5) == 'R') {
                    OptimizeReferences = optReferences;
                } else { // ICF[=iterations]
                    EnableCOMDATFolding = optFolding;
                    // [=iterations] case is not documented
                }
                break;
            case 'R': // NOREF
                OptimizeReferences = optNoReferences;
                break;
            case 'I': // NOICF
                EnableCOMDATFolding = optNoFolding;
                break;
            case 'N': // WIN98
                OptimizeForWindows98 = optWin98Yes;
                break;
            case 'W': // NOWIN98
                OptimizeForWindows98 = optWin98No;
                break;
            default:
                found = false;
            }
        }
        break;
    case 0x34468a2: // /ORDER:@filename
        FunctionOrder = option+8;
        break;
    case 0x00344a4: // /OUT:filename
        OutputFile = option+5;
        break;
    case 0x0034482: // /PDB:filename
        ProgramDatabaseFile = option+5;
        break;
    case 0xa2ad314: // /PDBSTRIPPED:pdb_file_name
        StripPrivateSymbols = option+13;
        break;
    case 0x6a09535: // /RELEASE
        SetChecksum = _True;
        break;
    case 0x348857b: // /STACK:reserve[,commit]
        {
            QStringList both = QString(option+7).split(",");
            StackReserveSize = both[0].toLongLong();
            if(both.count() == 2)
                StackCommitSize = both[1].toLongLong();
        }
        break;
    case 0x75AA4D8: // /SAFESH:{NO}
        {
            AdditionalOptions += option;
            break;
        }
	case 0x9B3C00D:
    case 0x78dc00d: // /SUBSYSTEM:{CONSOLE|EFI_APPLICATION|EFI_BOOT_SERVICE_DRIVER|EFI_ROM|EFI_RUNTIME_DRIVER|NATIVE|POSIX|WINDOWS|WINDOWSCE}[,major[.minor]]
        {
            // Split up in subsystem, and version number
            QStringList both = QString(option+11).split(",");
            switch (elfHash(both[0].toLatin1())) {
            case 0x8438445: // CONSOLE
                SubSystem = subSystemConsole;
                break;
            case 0xbe29493: // WINDOWS
                SubSystem = subSystemWindows;
                break;
            // The following are undocumented, so add them to AdditionalOptions
            case 0x240949e: // EFI_APPLICATION
            case 0xe617652: // EFI_BOOT_SERVICE_DRIVER
            case 0x9af477d: // EFI_ROM
            case 0xd34df42: // EFI_RUNTIME_DRIVER
            case 0x5268ea5: // NATIVE
            case 0x05547e8: // POSIX
            case 0x2949c95: // WINDOWSCE
            case 0x4B69795: // windowsce
                AdditionalOptions += option;
                break;
            default:
                found = false;
            }
        }
        break;
    case 0x8b654de: // /SWAPRUN:{NET | CD}
        if(*(option+9) == 'N')
            SwapRunFromNet = _True;
        else if(*(option+9) == 'C')
            SwapRunFromCD = _True;
        else
            found = false;
        break;
    case 0x34906d4: // /TLBID:id
        TypeLibraryResourceID = QString(option+7).toLongLong();
        break;
    case 0x4907494: // /TLBOUT:[path\]filename
        TypeLibraryFile = option+8;
        break;
    case 0x976b525: // /TSAWARE[:NO]
        if(*(option+8) == ':')
            TerminalServerAware = termSvrAwareNo;
        else
            TerminalServerAware = termSvrAwareYes;
        break;
    case 0xaa67735: // /VERBOSE[:lib]
        if(*(option+9) == ':') {
            ShowProgress = linkProgressLibs;
            AdditionalOptions += option;
        } else {
            ShowProgress = linkProgressAll;
        }
        break;
    case 0xaa77f7e: // /VERSION:major[.minor]
        Version = option+9;
        break;
    case 0x0034c50: // /WS[:NO]
        if (config->CompilerVersion >= NET2010) {
            if(*(option+3) == ':')
                TreatWarningsAsErrors = _False;
            else
                TreatWarningsAsErrors = _True;
        } else {
            AdditionalOptions += option;
        }
        break;
    default:
        AdditionalOptions += option;
        break;
    }
    if(!found) {
        warn_msg(WarnLogic, "Could not parse Linker options: %s, added as AdditionalOption", option);
        AdditionalOptions += option;
    }
    return found;
}

// VCMIDLTool -------------------------------------------------------
VCMIDLTool::VCMIDLTool()
    :        DefaultCharType(midlCharUnsigned),
        EnableErrorChecks(midlDisableAll),
        ErrorCheckAllocations(unset),
        ErrorCheckBounds(unset),
        ErrorCheckEnumRange(unset),
        ErrorCheckRefPointers(unset),
        ErrorCheckStubData(unset),
        GenerateStublessProxies(unset),
        GenerateTypeLibrary(unset),
        IgnoreStandardIncludePath(unset),
        MkTypLibCompatible(unset),
        StructMemberAlignment(midlAlignNotSet),
        SuppressStartupBanner(unset),
        TargetEnvironment(midlTargetNotSet),
        ValidateParameters(unset),
        WarnAsError(unset),
        WarningLevel(midlWarningLevel_0),
        ApplicationConfigurationMode(unset),
        ValidateAllParameters(unset),
        SuppressCompilerWarnings(unset),
        LocaleID(-1)
{
}

bool VCMIDLTool::parseOption(const char* option)
{
#ifdef USE_DISPLAY_HASH
    displayHash("/D name[=def]"); displayHash("/I directory-list"); displayHash("/Oi");
    displayHash("/Oic"); displayHash("/Oicf"); displayHash("/Oif"); displayHash("/Os");
    displayHash("/U name"); displayHash("/WX"); displayHash("/W{0|1|2|3|4}");
    displayHash("/Zp {N}"); displayHash("/Zs"); displayHash("/acf filename");
    displayHash("/align {N}"); displayHash("/app_config"); displayHash("/c_ext");
    displayHash("/char ascii7"); displayHash("/char signed"); displayHash("/char unsigned");
    displayHash("/client none"); displayHash("/client stub"); displayHash("/confirm");
    displayHash("/cpp_cmd cmd_line"); displayHash("/cpp_opt options");
    displayHash("/cstub filename"); displayHash("/dlldata filename"); displayHash("/env win32");
    displayHash("/env win64"); displayHash("/error all"); displayHash("/error allocation");
    displayHash("/error bounds_check"); displayHash("/error enum"); displayHash("/error none");
    displayHash("/error ref"); displayHash("/error stub_data"); displayHash("/h filename");
    displayHash("/header filename"); displayHash("/iid filename"); displayHash("/lcid");
    displayHash("/mktyplib203"); displayHash("/ms_ext"); displayHash("/ms_union");
    displayHash("/msc_ver <nnnn>"); displayHash("/newtlb"); displayHash("/no_cpp");
    displayHash("/no_def_idir"); displayHash("/no_default_epv"); displayHash("/no_format_opt");
    displayHash("/no_warn"); displayHash("/nocpp"); displayHash("/nologo"); displayHash("/notlb");
    displayHash("/o filename"); displayHash("/oldnames"); displayHash("/oldtlb");
    displayHash("/osf"); displayHash("/out directory"); displayHash("/pack {N}");
    displayHash("/prefix all"); displayHash("/prefix client"); displayHash("/prefix server");
    displayHash("/prefix switch"); displayHash("/protocol all"); displayHash("/protocol dce");
    displayHash("/protocol ndr64"); displayHash("/proxy filename"); displayHash("/robust");
    displayHash("/rpcss"); displayHash("/savePP"); displayHash("/server none");
    displayHash("/server stub"); displayHash("/sstub filename"); displayHash("/syntax_check");
    displayHash("/target {system}"); displayHash("/tlb filename"); displayHash("/use_epv");
    displayHash("/win32"); displayHash("/win64");
#endif
    bool found = true;
    int offset = 0;

    const uint optionHash = elfHash(option);

    if (config->CompilerVersion < NET2010) {
        switch (optionHash) {
        case 0x5b1cb97: // /app_config
        case 0x5a2fc64: // /client {none|stub}
        case 0x35aabb2: // /cstub filename
        case 0x64ceb12: // /newtlb
        case 0x556dbee: // /no_warn
        case 0x662bb12: // /oldtlb
        case 0x69c9cf2: // /server {none|stub}
        case 0x36aabb2: // /sstub filename
            AdditionalOptions += option;
            return true;
        }
    }

    switch(optionHash) {
    case 0x0000334: // /D name[=def]
        PreprocessorDefinitions += option+3;
        break;
    case 0x0000339: // /I directory-list
        AdditionalIncludeDirectories += option+3;
        break;
    case 0x0345f96: // /Oicf
    case 0x00345f6: // /Oif
        GenerateStublessProxies = _True;
        break;
    case 0x0000345: // /U name
        UndefinePreprocessorDefinitions += option+3;
        break;
    case 0x00034c8: // /WX
        WarnAsError = _True;
        break;
    case 0x3582fde: // /align {N}
        offset = 3;  // Fallthrough
    case 0x0003510: // /Zp {N}
        switch (*(option+offset+4)) {
        case '1':
            StructMemberAlignment = (*(option+offset+5) == '\0') ? midlAlignSingleByte : midlAlignSixteenBytes;
            break;
        case '2':
            StructMemberAlignment = midlAlignTwoBytes;
            break;
        case '4':
            StructMemberAlignment = midlAlignFourBytes;
            break;
        case '8':
            StructMemberAlignment = midlAlignEightBytes;
            break;
        default:
            found = false;
        }
        break;
    case 0x5b1cb97: // /app_config
        ApplicationConfigurationMode = _True;
        break;
    case 0x0359e82: // /char {ascii7|signed|unsigned}
        switch(*(option+6)) {
        case 'a':
            DefaultCharType = midlCharAscii7;
            break;
        case 's':
            DefaultCharType = midlCharSigned;
            break;
        case 'u':
            DefaultCharType = midlCharUnsigned;
            break;
        default:
            found = false;
        }
        break;
    case 0x5a2fc64: // /client {none|stub}
        if(*(option+8) == 's')
            GenerateClientFiles = "Stub";
        else
            GenerateClientFiles = "None";
        break;
    case 0xa766524: // /cpp_opt options
        CPreprocessOptions += option+9;
        break;
    case 0x35aabb2: // /cstub filename
        ClientStubFile = option+7;
        break;
    case 0xb32abf1: // /dlldata filename
        DLLDataFileName = option + 9;
        break;
    case 0x0035c56: // /env {win32|win64}
        TargetEnvironment = (*(option+8) == '6') ? midlTargetWin64 : midlTargetWin32;
        break;
    case 0x35c9962: // /error {all|allocation|bounds_check|enum|none|ref|stub_data}
        EnableErrorChecks = midlEnableCustom;
        switch (*(option+7)) {
        case 'a':
            if(*(option+10) == '\0')
                EnableErrorChecks = midlEnableAll;
            else
                ErrorCheckAllocations = _True;
            break;
        case 'b':
            ErrorCheckBounds = _True;
            break;
        case 'e':
            ErrorCheckEnumRange = _True;
            break;
        case 'n':
            EnableErrorChecks = midlDisableAll;
            break;
        case 'r':
            ErrorCheckRefPointers = _True;
            break;
        case 's':
            ErrorCheckStubData = _True;
            break;
        default:
            found = false;
        }
        break;
    case 0x5eb7af2: // /header filename
        offset = 5;
    case 0x0000358: // /h filename
        HeaderFileName = option + offset + 3;
        break;
    case 0x0035ff4: // /iid filename
        InterfaceIdentifierFileName = option+5;
        break;
    case 0x64b7933: // /mktyplib203
        MkTypLibCompatible = _True;
        break;
    case 0x64ceb12: // /newtlb
        TypeLibFormat = "NewFormat";
        break;
    case 0x8e0b0a2: // /no_def_idir
        IgnoreStandardIncludePath = _True;
        break;
    case 0x65635ef: // /nologo
        SuppressStartupBanner = _True;
        break;
    case 0x695e9f4: // /no_robust
        ValidateAllParameters = _False;
        break;
    case 0x3656b22: // /notlb
        GenerateTypeLibrary = _True;
        break;
    case 0x556dbee: // /no_warn
        SuppressCompilerWarnings = _True;
        break;
    case 0x000035f: // /o filename
        RedirectOutputAndErrors = option+3;
        break;
    case 0x662bb12: // /oldtlb
        TypeLibFormat = "OldFormat";
        break;
    case 0x00366c4: // /out directory
        OutputDirectory = option+5;
        break;
    case 0x36796f9: // /proxy filename
        ProxyFileName = option+7;
        break;
    case 0x6959c94: // /robust
        ValidateParameters = _True;
        break;
    case 0x6a88df4: // /target {system}
        if(*(option+11) == '6')
            TargetEnvironment = midlTargetWin64;
        else
            TargetEnvironment = midlTargetWin32;
        break;
    case 0x69c9cf2: // /server {none|stub}
        if(*(option+8) == 's')
            GenerateServerFiles = "Stub";
        else
            GenerateServerFiles = "None";
        break;
    case 0x36aabb2: // /sstub filename
        ServerStubFile = option+7;
        break;
    case 0x0036b22: // /tlb filename
        TypeLibraryName = option+5;
        break;
    case 0x36e0162: // /win32
        TargetEnvironment = midlTargetWin32;
        break;
    case 0x36e0194: // /win64
        TargetEnvironment = midlTargetWin64;
        break;
    case 0x0003459: // /Oi
    case 0x00345f3: // /Oic
    case 0x0003463: // /Os
    case 0x0003513: // /Zs
    case 0x0035796: // /acf filename
    case 0x3595cf4: // /c_ext
    case 0xa64d3dd: // /confirm
    case 0xa765b64: // /cpp_cmd cmd_line
    case 0x03629f4: // /lcid
    case 0x6495cc4: // /ms_ext
    case 0x96c7a1e: // /ms_union
    case 0x4996fa2: // /msc_ver <nnnn>
    case 0x6555a40: // /no_cpp
    case 0xf64d6a6: // /no_default_epv
    case 0x6dd9384: // /no_format_opt
    case 0x3655a70: // /nocpp
    case 0x2b455a3: // /oldnames
    case 0x0036696: // /osf
    case 0x036679b: // /pack {N}
    case 0x678bd38: // /prefix {all|client|server|switch}
    case 0x96b702c: // /protocol {all|dce|ndr64}
    case 0x3696aa3: // /rpcss
    case 0x698ca60: // /savePP
    case 0xce9b12b: // /syntax_check
    case 0xc9b5f16: // /use_epv
        AdditionalOptions += option;
        break;
    default:
        // /W{0|1|2|3|4} case
        if(*(option+1) == 'W') {
            switch (*(option+2)) {
            case '0':
                WarningLevel = midlWarningLevel_0;
                break;
            case '1':
                WarningLevel = midlWarningLevel_1;
                break;
            case '2':
                WarningLevel = midlWarningLevel_2;
                break;
            case '3':
                WarningLevel = midlWarningLevel_3;
                break;
            case '4':
                WarningLevel = midlWarningLevel_4;
                break;
            default:
                found = false;
            }
        }
        break;
    }
    if(!found)
        warn_msg(WarnLogic, "Could not parse MIDL option: %s", option);
    return true;
}

// VCLibrarianTool --------------------------------------------------
VCLibrarianTool::VCLibrarianTool()
    :        IgnoreAllDefaultLibraries(unset),
        SuppressStartupBanner(_True)
{
}

// VCCustomBuildTool ------------------------------------------------
VCCustomBuildTool::VCCustomBuildTool()
{
    ToolName = "VCCustomBuildTool";
}

// VCResourceCompilerTool -------------------------------------------
VCResourceCompilerTool::VCResourceCompilerTool()
    :   Culture(rcUseDefault),
        IgnoreStandardIncludePath(unset),
        ShowProgress(linkProgressNotSet),
        SuppressStartupBanner(unset)
{
    PreprocessorDefinitions = QStringList("NDEBUG");
}

// VCDeploymentTool --------------------------------------------
VCDeploymentTool::VCDeploymentTool()
    :   RegisterOutput(registerNo)
{
    DeploymentTag = "DeploymentTool";
    RemoteDirectory = "";
}

VCEventTool::VCEventTool(const QString &eventName)
    : ExcludedFromBuild(unset)
{
    EventName = eventName;
    ToolName = "VC";
    ToolName += eventName;
    ToolName += "Tool";
}

// VCPostBuildEventTool ---------------------------------------------
VCPostBuildEventTool::VCPostBuildEventTool()
    : VCEventTool("PostBuildEvent")
{
}

// VCPreBuildEventTool ----------------------------------------------
VCPreBuildEventTool::VCPreBuildEventTool()
    : VCEventTool("PreBuildEvent")
{
}

// VCPreLinkEventTool -----------------------------------------------
VCPreLinkEventTool::VCPreLinkEventTool()
    : VCEventTool("PreLinkEvent")
{
}

// VCConfiguration --------------------------------------------------

VCConfiguration::VCConfiguration()
    :        ATLMinimizesCRunTimeLibraryUsage(unset),
        BuildBrowserInformation(unset),
        CharacterSet(charSetNotSet),
        ConfigurationType(typeApplication),
        RegisterOutput(unset),
        UseOfATL(useATLNotSet),
        UseOfMfc(useMfcStdWin),
        WholeProgramOptimization(unset)
{
    compiler.config = this;
    linker.config = this;
    idl.config = this;
    custom.config = this;
}

// VCFilter ---------------------------------------------------------
VCFilter::VCFilter()
    :   ParseFiles(unset),
        Config(0)
{
    useCustomBuildTool = false;
    useCompilerTool = false;
}

void VCFilter::addFile(const QString& filename)
{
    Files += VCFilterFile(filename);
}

void VCFilter::addFile(const VCFilterFile& fileInfo)
{
    Files += VCFilterFile(fileInfo);
}

void VCFilter::addFiles(const QStringList& fileList)
{
    for (int i = 0; i < fileList.count(); ++i)
        addFile(fileList.at(i));
}

void VCFilter::modifyPCHstage(QString str)
{
    bool autogenSourceFile = Project->autogenPrecompCPP;
    bool pchThroughSourceFile = !Project->precompCPP.isEmpty();
    bool isCFile = false;
    for (QStringList::Iterator it = Option::c_ext.begin(); it != Option::c_ext.end(); ++it) {
        if (str.endsWith(*it)) {
            isCFile = true;
            break;
        }
    }
    bool isHFile = Option::hasFileExtension(str, Option::h_ext) && (str == Project->precompH);
    bool isCPPFile = pchThroughSourceFile && (str == Project->precompCPP);

    if(!isCFile && !isHFile && !isCPPFile)
        return;

    if(isHFile && pchThroughSourceFile) {
        if (autogenSourceFile) {
            useCustomBuildTool = true;
            QString toFile(Project->precompCPP);
            CustomBuildTool.Description = "Generating precompiled header source file '" + toFile + "' ...";
            CustomBuildTool.Outputs += toFile;

            QStringList lines;
            CustomBuildTool.CommandLine +=
                "echo /*-------------------------------------------------------------------- >" + toFile;
            lines << "* Precompiled header source file used by Visual Studio.NET to generate";
            lines << "* the .pch file.";
            lines << "*";
            lines << "* Due to issues with the dependencies checker within the IDE, it";
            lines << "* sometimes fails to recompile the PCH file, if we force the IDE to";
            lines << "* create the PCH file directly from the header file.";
            lines << "*";
            lines << "* This file is auto-generated by qmake since no PRECOMPILED_SOURCE was";
            lines << "* specified, and is used as the common stdafx.cpp. The file is only";
            lines << QLatin1String("* generated when creating ")
                     + (Config->CompilerVersion < NET2010 ? ".vcproj" : ".vcxproj")
                     + " project files, and is not used for";
            lines << "* command line compilations by nmake.";
            lines << "*";
            lines << "* WARNING: All changes made in this file will be lost.";
            lines << "--------------------------------------------------------------------*/";
            lines << "#include \"" + Project->precompHFilename + "\"";
            foreach(QString line, lines)
                CustomBuildTool.CommandLine += "echo " + line + ">>" + toFile;
        }
        return;
    }

    useCompilerTool = true;
    // Setup PCH options
    CompilerTool.UsePrecompiledHeader     = (isCFile ? pchNone : pchCreateUsingSpecific);
    CompilerTool.PrecompiledHeaderThrough = (isCPPFile ? Project->precompHFilename : QString("$(NOINHERIT)"));
    CompilerTool.ForcedIncludeFiles       = QStringList("$(NOINHERIT)");
}

bool VCFilter::addExtraCompiler(const VCFilterFile &info)
{
    const QStringList &extraCompilers = Project->extraCompilerSources.value(info.file);
    if (extraCompilers.isEmpty())
        return false;

    QString inFile = info.file;

    // is the extracompiler rule on a file with a built in compiler?
    const QStringList &objectMappedFile = Project->extraCompilerOutputs[inFile];
    bool hasBuiltIn = false;
    if (!objectMappedFile.isEmpty()) {
        hasBuiltIn = Project->hasBuiltinCompiler(objectMappedFile.at(0));
//        qDebug("*** Extra compiler file has object mapped file '%s' => '%s'", qPrintable(inFile), qPrintable(objectMappedFile.join(" ")));
    }

    CustomBuildTool.AdditionalDependencies.clear();
    CustomBuildTool.CommandLine.clear();
    CustomBuildTool.Description.clear();
    CustomBuildTool.Outputs.clear();
    CustomBuildTool.ToolPath.clear();
	CustomBuildTool.ToolName = QLatin1String(_VCCustomBuildTool);

    for (int x = 0; x < extraCompilers.count(); ++x) {
        const QString &extraCompilerName = extraCompilers.at(x);

        if (!Project->verifyExtraCompiler(extraCompilerName, inFile) && !hasBuiltIn)
            continue;

        // All information about the extra compiler
        QString tmp_out = Project->project->first(extraCompilerName + ".output");
        QString tmp_cmd = Project->project->variables()[extraCompilerName + ".commands"].join(" ");
        QString tmp_cmd_name = Project->project->variables()[extraCompilerName + ".name"].join(" ");
        QStringList tmp_dep = Project->project->variables()[extraCompilerName + ".depends"];
        QString tmp_dep_cmd = Project->project->variables()[extraCompilerName + ".depend_command"].join(" ");
        QStringList configs = Project->project->variables()[extraCompilerName + ".CONFIG"];
        bool combined = configs.indexOf("combine") != -1;

        QString cmd, cmd_name, out;
        QStringList deps, inputs;
        // Variabel replacement of output name
        out = Option::fixPathToTargetOS(
                    Project->replaceExtraCompilerVariables(tmp_out, inFile, QString()),
                    false);

        // If file has built-in compiler, we've swapped the input and output of
        // the command, as we in Visual Studio cannot have a Custom Buildstep on
        // a file which uses a built-in compiler. We would in this case only get
        // the result from the extra compiler. If 'hasBuiltIn' is true, we know
        // that we're actually on the _output_file_ of the result, and we
        // therefore swap inFile and out below, since the extra-compiler still
        // must see it as the original way. If the result also has a built-in
        // compiler, too bad..
        if (hasBuiltIn) {
            out = inFile;
            inFile = objectMappedFile.at(0);
        }

        // Dependency for the output
        if(!tmp_dep.isEmpty())
	    deps = tmp_dep;
	if(!tmp_dep_cmd.isEmpty()) {
            // Execute dependency command, and add every line as a dep
	    char buff[256];
	    QString dep_cmd = Project->replaceExtraCompilerVariables(tmp_dep_cmd,
							             Option::fixPathToLocalOS(inFile, true, false),
                                                                     out);
            if(Project->canExecute(dep_cmd)) {
                dep_cmd.prepend(QLatin1String("cd ")
                                + Project->escapeFilePath(Option::fixPathToLocalOS(Option::output_dir, false))
                                + QLatin1String(" && "));
                if(FILE *proc = QT_POPEN(dep_cmd.toLatin1().constData(), "r")) {
                    QString indeps;
                    while(!feof(proc)) {
                        int read_in = (int)fread(buff, 1, 255, proc);
                        if(!read_in)
                            break;
                        indeps += QByteArray(buff, read_in);
                    }
                    QT_PCLOSE(proc);
                    if(!indeps.isEmpty()) {
                        QStringList extradeps = indeps.split(QLatin1Char('\n'));
                        for (int i = 0; i < extradeps.count(); ++i) {
                            QString dd = extradeps.at(i).simplified();
                            if (!dd.isEmpty())
                                deps += Project->fileFixify(dd, QString(), Option::output_dir);
                        }
                    }
                }
            }
        }
        for (int i = 0; i < deps.count(); ++i)
	    deps[i] = Option::fixPathToTargetOS(
                        Project->replaceExtraCompilerVariables(deps.at(i), inFile, out),
                        false).trimmed();
        // Command for file
        if (combined) {
            // Add dependencies for each file
            QStringList tmp_in = Project->project->variables()[extraCompilerName + ".input"];
            for (int a = 0; a < tmp_in.count(); ++a) {
                const QStringList &files = Project->project->variables()[tmp_in.at(a)];
                for (int b = 0; b < files.count(); ++b) {
                    deps += Project->findDependencies(files.at(b));
                    inputs += Option::fixPathToTargetOS(files.at(b), false);
                }
            }
            deps += inputs; // input files themselves too..

            // Replace variables for command w/all input files
            // ### join gives path issues with directories containing spaces!
            cmd = Project->replaceExtraCompilerVariables(tmp_cmd,
                                                         inputs.join(" "),
                                                         out);
        } else {
            deps += inFile; // input file itself too..
            cmd = Project->replaceExtraCompilerVariables(tmp_cmd,
                                                         inFile,
                                                         out);
        }
        // Name for command
	if(!tmp_cmd_name.isEmpty()) {
	    cmd_name = Project->replaceExtraCompilerVariables(tmp_cmd_name, inFile, out);
	} else {
	    int space = cmd.indexOf(' ');
	    if(space != -1)
		cmd_name = cmd.left(space);
	    else
		cmd_name = cmd;
	    if((cmd_name[0] == '\'' || cmd_name[0] == '"') &&
		cmd_name[0] == cmd_name[cmd_name.length()-1])
		cmd_name = cmd_name.mid(1,cmd_name.length()-2);
	}

        // Fixify paths
        for (int i = 0; i < deps.count(); ++i)
            deps[i] = Option::fixPathToTargetOS(deps[i], false);


        // Output in info.additionalFile -----------
        if (!CustomBuildTool.Description.isEmpty())
            CustomBuildTool.Description += ", ";
        CustomBuildTool.Description += cmd_name;
        CustomBuildTool.CommandLine += VCToolBase::fixCommandLine(cmd.trimmed());
        int space = cmd.indexOf(' ');
        QFileInfo finf(cmd.left(space));
        if (CustomBuildTool.ToolPath.isEmpty())
            CustomBuildTool.ToolPath += Option::fixPathToTargetOS(finf.path());
        CustomBuildTool.Outputs += out;

        deps += CustomBuildTool.AdditionalDependencies;
        deps += cmd.left(cmd.indexOf(' '));
        // Make sure that all deps are only once
        QMap<QString, bool> uniqDeps;
        for (int c = 0; c < deps.count(); ++c) {
            QString aDep = deps.at(c).trimmed();
            if (!aDep.isEmpty())
                uniqDeps[aDep] = false;
        }
        CustomBuildTool.AdditionalDependencies = uniqDeps.keys();
    }

    // Ensure that none of the output files are also dependencies. Or else, the custom buildstep
    // will be rebuild every time, even if nothing has changed.
    foreach(QString output, CustomBuildTool.Outputs) {
        CustomBuildTool.AdditionalDependencies.removeAll(output);
    }

    useCustomBuildTool = !CustomBuildTool.CommandLine.isEmpty();
    return useCustomBuildTool;
}

// VCProjectSingleConfig --------------------------------------------
VCFilter& VCProjectSingleConfig::filterForExtraCompiler(const QString &compilerName)
{
    for (int i = 0; i < ExtraCompilersFiles.count(); ++i)
        if (ExtraCompilersFiles.at(i).Name == compilerName)
            return ExtraCompilersFiles[i];

    static VCFilter nullFilter;
    return nullFilter;
}

// Tree file generation ---------------------------------------------
void TreeNode::generateXML(XmlOutput &xml, const QString &tagName, VCProject &tool, const QString &filter) {
    if (children.size()) {
        // Filter
        ChildrenMap::ConstIterator it, end = children.constEnd();
        if (!tagName.isEmpty()) {
            xml << tag("Filter")
                << attr("Name", tagName)
                << attr("Filter", "");
        }
        // First round, do nested filters
        for (it = children.constBegin(); it != end; ++it)
            if ((*it)->children.size())
                (*it)->generateXML(xml, it.key(), tool, filter);
        // Second round, do leafs
        for (it = children.constBegin(); it != end; ++it)
            if (!(*it)->children.size())
                (*it)->generateXML(xml, it.key(), tool, filter);

        if (!tagName.isEmpty())
            xml << closetag("Filter");
    } else {
        // Leaf
        VCProjectWriter::outputFileConfigs(tool, xml, info, filter);
    }
}

// Flat file generation ---------------------------------------------
void FlatNode::generateXML(XmlOutput &xml, const QString &/*tagName*/, VCProject &tool, const QString &filter) {
    if (children.size()) {
        ChildrenMapFlat::ConstIterator it = children.constBegin();
        ChildrenMapFlat::ConstIterator end = children.constEnd();
        for (; it != end; ++it) {
            VCProjectWriter::outputFileConfigs(tool, xml, (*it), filter);
        }
    }
}

void VCProjectWriter::write(XmlOutput &xml, VCProjectSingleConfig &tool)
{
    xml << decl("1.0", "Windows-1252")
        << tag(_VisualStudioProject)
        << attrS(_ProjectType, "Visual C++")
        << attrS(_Version, tool.Version)
        << attrS(_Name, tool.Name)
        << attrS(_ProjectGUID, tool.ProjectGUID)
        << attrS(_Keyword, tool.Keyword)
        << attrS(_SccProjectName, tool.SccProjectName)
        << attrS(_SccLocalPath, tool.SccLocalPath)
        << tag(_Platforms)
        << tag(_Platform)
        << attrS(_Name, tool.PlatformName)
        << closetag(_Platforms)
        << tag(_Configurations);
    write(xml, tool.Configuration);
    xml << closetag(_Configurations)
        << tag(q_Files);
    // Add this configuration into a multi-config project, since that's where we have the flat/tree
    // XML output functionality
    VCProject tempProj;
    tempProj.SingleProjects += tool;
    outputFilter(tempProj, xml, "Sources");
    outputFilter(tempProj, xml, "Headers");
    outputFilter(tempProj, xml, "GeneratedFiles");
    outputFilter(tempProj, xml, "LexYaccFiles");
    outputFilter(tempProj, xml, "TranslationFiles");
    outputFilter(tempProj, xml, "FormFiles");
    outputFilter(tempProj, xml, "ResourceFiles");
    for (int x = 0; x < tempProj.ExtraCompilers.count(); ++x) {
        outputFilter(tempProj, xml, tempProj.ExtraCompilers.at(x));
    }
    outputFilter(tempProj, xml, "RootFiles");
    xml     << closetag(q_Files)
            << tag(_Globals)
                << data(); // No "/>" end tag
}

void VCProjectWriter::write(XmlOutput &xml, VCProject &tool)
{
    if (tool.SingleProjects.count() == 0) {
        warn_msg(WarnLogic, "Generator: .NET: no single project in merge project, no output");
        return;
    }

    xml << decl("1.0", "Windows-1252")
        << tag(_VisualStudioProject)
            << attrS(_ProjectType, "Visual C++")
            << attrS(_Version, tool.Version)
            << attrS(_Name, tool.Name)
            << attrS(_ProjectGUID, tool.ProjectGUID)
            << attrS(_Keyword, tool.Keyword)
            << attrS(_SccProjectName, tool.SccProjectName)
            << attrS(_SccLocalPath, tool.SccLocalPath)
            << tag(_Platforms)
                << tag(_Platform)
                    << attrS(_Name, tool.PlatformName)
            << closetag(_Platforms)
            << tag(_Configurations);
    // Output each configuration
    for (int i = 0; i < tool.SingleProjects.count(); ++i)
        write(xml, tool.SingleProjects.at(i).Configuration);
    xml     << closetag(_Configurations)
            << tag(q_Files);
    outputFilter(tool, xml, "Sources");
    outputFilter(tool, xml, "Headers");
    outputFilter(tool, xml, "GeneratedFiles");
    outputFilter(tool, xml, "LexYaccFiles");
    outputFilter(tool, xml, "TranslationFiles");
    outputFilter(tool, xml, "FormFiles");
    outputFilter(tool, xml, "ResourceFiles");
    for (int x = 0; x < tool.ExtraCompilers.count(); ++x) {
        outputFilter(tool, xml, tool.ExtraCompilers.at(x));
    }
    outputFilter(tool, xml, "RootFiles");
    xml     << closetag(q_Files)
            << tag(_Globals)
            << data(); // No "/>" end tag
}

void VCProjectWriter::write(XmlOutput &xml, const VCCLCompilerTool &tool)
{
    xml << tag(_Tool)
        << attrS(_Name, _VCCLCompilerTool)
        << attrX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories)
        << attrX(_AdditionalOptions, tool.AdditionalOptions, " ")
        << attrX(_AdditionalUsingDirectories, tool.AdditionalUsingDirectories)
        << attrS(_AssemblerListingLocation, tool.AssemblerListingLocation)
        << attrE(_AssemblerOutput, tool.AssemblerOutput, /*ifNot*/ asmListingNone)
        << attrE(_BasicRuntimeChecks, tool.BasicRuntimeChecks, /*ifNot*/ runtimeBasicCheckNone)
        << attrE(_BrowseInformation, tool.BrowseInformation, /*ifNot*/ brInfoNone)
        << attrS(_BrowseInformationFile, tool.BrowseInformationFile)
        << attrT(_BufferSecurityCheck, tool.BufferSecurityCheck)
        << attrE(_CallingConvention, tool.CallingConvention, /*ifNot*/ callConventionDefault)
        << attrE(_CompileAs, tool.CompileAs, compileAsDefault)
        << attrE(_CompileAsManaged, tool.CompileAsManaged, /*ifNot*/ managedDefault)
        << attrT(_CompileOnly, tool.CompileOnly)
        << attrE(_DebugInformationFormat, tool.DebugInformationFormat, /*ifNot*/ debugUnknown)
        << attrT(_DefaultCharIsUnsigned, tool.DefaultCharIsUnsigned)
        << attrT(_Detect64BitPortabilityProblems, tool.Detect64BitPortabilityProblems)
        << attrT(_DisableLanguageExtensions, tool.DisableLanguageExtensions)
        << attrX(_DisableSpecificWarnings, tool.DisableSpecificWarnings)
        << attrE(_EnableEnhancedInstructionSet, tool.EnableEnhancedInstructionSet, /*ifnot*/ archNotSet)
        << attrT(_EnableFiberSafeOptimizations, tool.EnableFiberSafeOptimizations)
        << attrT(_EnableFunctionLevelLinking, tool.EnableFunctionLevelLinking)
        << attrT(_EnableIntrinsicFunctions, tool.EnableIntrinsicFunctions)
        << xformExceptionHandlingNET2005(tool.ExceptionHandling, tool.config->CompilerVersion)
        << attrT(_ExpandAttributedSource, tool.ExpandAttributedSource)
        << attrE(_FavorSizeOrSpeed, tool.FavorSizeOrSpeed, /*ifNot*/ favorNone)

        << attrE(_FloatingPointModel, tool.FloatingPointModel, /*ifNot*/ floatingPointNotSet)
        << attrT(_FloatingPointExceptions, tool.FloatingPointExceptions)

        << attrT(_ForceConformanceInForLoopScope, tool.ForceConformanceInForLoopScope)
        << attrX(_ForcedIncludeFiles, tool.ForcedIncludeFiles)
        << attrX(_ForcedUsingFiles, tool.ForcedUsingFiles)
        << attrE(_GeneratePreprocessedFile, tool.GeneratePreprocessedFile, /*ifNot*/ preprocessUnknown)
        << attrT(_GlobalOptimizations, tool.GlobalOptimizations)
        << attrT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
        << attrT(_ImproveFloatingPointConsistency, tool.ImproveFloatingPointConsistency)
        << attrE(_InlineFunctionExpansion, tool.InlineFunctionExpansion, /*ifNot*/ expandDefault)
        << attrT(_KeepComments, tool.KeepComments)
        << attrT(_MinimalRebuild, tool.MinimalRebuild)
        << attrS(_ObjectFile, tool.ObjectFile)
        << attrT(_OmitFramePointers, tool.OmitFramePointers)
        << attrT(_OpenMP, tool.OpenMP)
        << attrE(_Optimization, tool.Optimization, /*ifNot*/ optimizeDefault)
        << attrE(_OptimizeForProcessor, tool.OptimizeForProcessor, /*ifNot*/ procOptimizeBlended)
        << attrT(_OptimizeForWindowsApplication, tool.OptimizeForWindowsApplication)
        << attrS(_OutputFile, tool.OutputFile)
        << attrS(_PrecompiledHeaderFile, tool.PrecompiledHeaderFile)
        << attrS(_PrecompiledHeaderThrough, tool.PrecompiledHeaderThrough)
        << attrX(_PreprocessorDefinitions, tool.PreprocessorDefinitions)
        << (tool.ProgramDataBaseFileName.isNull() ? noxml() : attr(_ProgramDataBaseFileName, tool.ProgramDataBaseFileName))
        << attrE(_RuntimeLibrary, tool.RuntimeLibrary, /*ifNot*/ rtUnknown)
        << attrT(_RuntimeTypeInfo, tool.RuntimeTypeInfo)
        << attrT(_ShowIncludes, tool.ShowIncludes)
        << attrT(_SmallerTypeCheck, tool.SmallerTypeCheck)
        << attrT(_StringPooling, tool.StringPooling)
        << attrE(_StructMemberAlignment, tool.StructMemberAlignment, /*ifNot*/ alignNotSet)
        << attrT(_SuppressStartupBanner, tool.SuppressStartupBanner)
        << attrT(_TreatWChar_tAsBuiltInType, tool.TreatWChar_tAsBuiltInType)
        << attrT(_TurnOffAssemblyGeneration, tool.TurnOffAssemblyGeneration)
        << attrT(_UndefineAllPreprocessorDefinitions, tool.UndefineAllPreprocessorDefinitions)
        << attrX(_UndefinePreprocessorDefinitions, tool.UndefinePreprocessorDefinitions)
        << xformUsePrecompiledHeaderForNET2005(tool.UsePrecompiledHeader, tool.config->CompilerVersion)
        << attrT(_WarnAsError, tool.WarnAsError)
        << attrE(_WarningLevel, tool.WarningLevel, /*ifNot*/ warningLevelUnknown)
        << attrT(_WholeProgramOptimization, tool.WholeProgramOptimization)
        << attrE(_CompileForArchitecture, tool.CompileForArchitecture, /*ifNot*/ archUnknown)
        << attrT(_InterworkCalls, tool.InterworkCalls)

        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCLinkerTool &tool)
{
    xml << tag(_Tool)
        << attrS(_Name, _VCLinkerTool)
        << attrX(_AdditionalDependencies, tool.AdditionalDependencies, " ")
        << attrX(_AdditionalLibraryDirectories, tool.AdditionalLibraryDirectories)
        << attrX(_AdditionalOptions, tool.AdditionalOptions, " ")
        << attrX(_AddModuleNamesToAssembly, tool.AddModuleNamesToAssembly)
        << attrS(_BaseAddress, tool.BaseAddress)
        << attrX(_DelayLoadDLLs, tool.DelayLoadDLLs)
        << attrE(_EnableCOMDATFolding, tool.EnableCOMDATFolding, /*ifNot*/ optFoldingDefault)
        << attrS(_EntryPointSymbol, tool.EntryPointSymbol)
        << attrX(_ForceSymbolReferences, tool.ForceSymbolReferences)
        << attrS(_FunctionOrder, tool.FunctionOrder)
        << attrT(_GenerateDebugInformation, tool.GenerateDebugInformation)
        << attrT(_GenerateMapFile, tool.GenerateMapFile)
        << attrL(_HeapCommitSize, tool.HeapCommitSize, /*ifNot*/ -1)
        << attrL(_HeapReserveSize, tool.HeapReserveSize, /*ifNot*/ -1)
        << attrT(_IgnoreAllDefaultLibraries, tool.IgnoreAllDefaultLibraries)
        << attrX(_IgnoreDefaultLibraryNames, tool.IgnoreDefaultLibraryNames)
        << attrT(_IgnoreEmbeddedIDL, tool.IgnoreEmbeddedIDL)
        << attrT(_IgnoreImportLibrary, tool.IgnoreImportLibrary)
        << attrS(_ImportLibrary, tool.ImportLibrary)
        << attrE(_LargeAddressAware, tool.LargeAddressAware, /*ifNot*/ addrAwareDefault)
        << attrT(_LinkDLL, tool.LinkDLL)
        << attrE(_LinkIncremental, tool.LinkIncremental, /*ifNot*/ linkIncrementalDefault)
        << attrE(_LinkTimeCodeGeneration, tool.LinkTimeCodeGeneration)
        << attrS(_LinkToManagedResourceFile, tool.LinkToManagedResourceFile)
        << attrT(_MapExports, tool.MapExports)
        << attrS(_MapFileName, tool.MapFileName)
        << attrT(_MapLines, tool.MapLines)
        << attrS(_MergedIDLBaseFileName, tool.MergedIDLBaseFileName)
        << attrS(_MergeSections, tool.MergeSections)
        << attrS(_MidlCommandFile, tool.MidlCommandFile)
        << attrS(_ModuleDefinitionFile, tool.ModuleDefinitionFile)
        << attrE(_OptimizeForWindows98, tool.OptimizeForWindows98, /*ifNot*/ optWin98Default)
        << attrE(_OptimizeReferences, tool.OptimizeReferences, /*ifNot*/ optReferencesDefault)
        << attrS(_OutputFile, tool.OutputFile)
        << attr(_ProgramDatabaseFile, tool.ProgramDatabaseFile)
        << attrT(_RegisterOutput, tool.RegisterOutput)
        << attrT(_ResourceOnlyDLL, tool.ResourceOnlyDLL)
        << attrT(_SetChecksum, tool.SetChecksum)
        << attrE(_ShowProgress, tool.ShowProgress, /*ifNot*/ linkProgressNotSet)
        << attrL(_StackCommitSize, tool.StackCommitSize, /*ifNot*/ -1)
        << attrL(_StackReserveSize, tool.StackReserveSize, /*ifNot*/ -1)
        << attrS(_StripPrivateSymbols, tool.StripPrivateSymbols)
        << attrE(_SubSystem, tool.SubSystem)
        << attrT(_SupportUnloadOfDelayLoadedDLL, tool.SupportUnloadOfDelayLoadedDLL)
        << attrT(_SuppressStartupBanner, tool.SuppressStartupBanner)
        << attrT(_SwapRunFromCD, tool.SwapRunFromCD)
        << attrT(_SwapRunFromNet, tool.SwapRunFromNet)
        << attrE(_TargetMachine, tool.TargetMachine, /*ifNot*/ machineNotSet)
        << attrE(_TerminalServerAware, tool.TerminalServerAware, /*ifNot*/ termSvrAwareDefault)
        << attrT(_TurnOffAssemblyGeneration, tool.TurnOffAssemblyGeneration)
        << attrS(_TypeLibraryFile, tool.TypeLibraryFile)
        << attrL(_TypeLibraryResourceID, tool.TypeLibraryResourceID, /*ifNot*/ rcUseDefault)
        << attrS(_Version, tool.Version)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCMIDLTool &tool)
{
    xml << tag(_Tool)
        << attrS(_Name, _VCMIDLTool)
        << attrX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories)
        << attrX(_AdditionalOptions, tool.AdditionalOptions, " ")
        << attrX(_CPreprocessOptions, tool.CPreprocessOptions)
        << attrE(_DefaultCharType, tool.DefaultCharType)
        << attrS(_DLLDataFileName, tool.DLLDataFileName)
        << attrE(_EnableErrorChecks, tool.EnableErrorChecks)
        << attrT(_ErrorCheckAllocations, tool.ErrorCheckAllocations)
        << attrT(_ErrorCheckBounds, tool.ErrorCheckBounds)
        << attrT(_ErrorCheckEnumRange, tool.ErrorCheckEnumRange)
        << attrT(_ErrorCheckRefPointers, tool.ErrorCheckRefPointers)
        << attrT(_ErrorCheckStubData, tool.ErrorCheckStubData)
        << attrX(_FullIncludePath, tool.FullIncludePath)
        << attrT(_GenerateStublessProxies, tool.GenerateStublessProxies)
        << attrT(_GenerateTypeLibrary, tool.GenerateTypeLibrary)
        << attrS(_HeaderFileName, tool.HeaderFileName)
        << attrT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
        << attrS(_InterfaceIdentifierFileName, tool.InterfaceIdentifierFileName)
        << attrT(_MkTypLibCompatible, tool.MkTypLibCompatible)
        << attrS(_OutputDirectory, tool.OutputDirectory)
        << attrX(_PreprocessorDefinitions, tool.PreprocessorDefinitions)
        << attrS(_ProxyFileName, tool.ProxyFileName)
        << attrS(_RedirectOutputAndErrors, tool.RedirectOutputAndErrors)
        << attrE(_StructMemberAlignment, tool.StructMemberAlignment, /*ifNot*/ midlAlignNotSet)
        << attrT(_SuppressStartupBanner, tool.SuppressStartupBanner)
        << attrE(_TargetEnvironment, tool.TargetEnvironment, /*ifNot*/ midlTargetNotSet)
        << attrS(_TypeLibraryName, tool.TypeLibraryName)
        << attrX(_UndefinePreprocessorDefinitions, tool.UndefinePreprocessorDefinitions)
        << attrT(_ValidateParameters, tool.ValidateParameters)
        << attrT(_WarnAsError, tool.WarnAsError)
        << attrE(_WarningLevel, tool.WarningLevel)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCCustomBuildTool &tool)
{
    xml << tag(_Tool)
            << attrS(_Name, tool.ToolName)
            << attrX(_AdditionalDependencies, tool.AdditionalDependencies, ";")
            << attrS(_CommandLine, tool.CommandLine.join(vcCommandSeparator()))
            << attrS(_Description, tool.Description)
            << attrX(_Outputs, tool.Outputs, ";")
            << attrS(_Path, tool.ToolPath)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCLibrarianTool &tool)
{
    xml
        << tag(_Tool)
            << attrS(_Name, _VCLibrarianTool)
            << attrX(_AdditionalDependencies, tool.AdditionalDependencies)
            << attrX(_AdditionalLibraryDirectories, tool.AdditionalLibraryDirectories)
            << attrX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrX(_ExportNamedFunctions, tool.ExportNamedFunctions)
            << attrX(_ForceSymbolReferences, tool.ForceSymbolReferences)
            << attrT(_IgnoreAllDefaultLibraries, tool.IgnoreAllDefaultLibraries)
            << attrX(_IgnoreDefaultLibraryNames, tool.IgnoreDefaultLibraryNames)
            << attrS(_ModuleDefinitionFile, tool.ModuleDefinitionFile)
            << attrS(_OutputFile, tool.OutputFile)
            << attrT(_SuppressStartupBanner, tool.SuppressStartupBanner)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCResourceCompilerTool &tool)
{
    xml
        << tag(_Tool)
            << attrS(_Name, _VCResourceCompilerTool)
            << attrS(_Path, tool.ToolPath)
            << attrX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories)
            << attrX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrE(_Culture, tool.Culture, /*ifNot*/ rcUseDefault)
            << attrX(_FullIncludePath, tool.FullIncludePath)
            << attrT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
            << attrX(_PreprocessorDefinitions, tool.PreprocessorDefinitions)
            << attrS(_ResourceOutputFileName, tool.ResourceOutputFileName)
            << attrE(_ShowProgress, tool.ShowProgress, /*ifNot*/ linkProgressNotSet)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCEventTool &tool)
{
    xml
        << tag(_Tool)
            << attrS(_Name, tool.ToolName)
            << attrS(_Path, tool.ToolPath)
            << attrS(_CommandLine, tool.CommandLine.join(vcCommandSeparator()))
            << attrS(_Description, tool.Description)
            << attrT(_ExcludedFromBuild, tool.ExcludedFromBuild)
        << closetag(_Tool);
}

void VCProjectWriter::write(XmlOutput &xml, const VCDeploymentTool &tool)
{
    if (tool.AdditionalFiles.isEmpty())
        return;
    xml << tag(tool.DeploymentTag)
        << attrS(_RemoteDirectory, tool.RemoteDirectory)
        << attrE(_RegisterOutput, tool.RegisterOutput)
        << attrS(_AdditionalFiles, tool.AdditionalFiles)
        << closetag(tool.DeploymentTag);
}

void VCProjectWriter::write(XmlOutput &xml, const VCConfiguration &tool)
{
    xml << tag(_Configuration)
            << attrS(_Name, tool.Name)
            << attrS(_OutputDirectory, tool.OutputDirectory)
            << attrT(_ATLMinimizesCRunTimeLibraryUsage, tool.ATLMinimizesCRunTimeLibraryUsage)
            << attrT(_BuildBrowserInformation, tool.BuildBrowserInformation)
            << attrE(_CharacterSet, tool.CharacterSet, /*ifNot*/ charSetNotSet)
            << attrE(_ConfigurationType, tool.ConfigurationType)
            << attrS(_DeleteExtensionsOnClean, tool.DeleteExtensionsOnClean)
            << attrS(_ImportLibrary, tool.ImportLibrary)
            << attrS(_IntermediateDirectory, tool.IntermediateDirectory)
            << attrS(_PrimaryOutput, tool.PrimaryOutput)
            << attrS(_ProgramDatabase, tool.ProgramDatabase)
            << attrT(_RegisterOutput, tool.RegisterOutput)
            << attrE(_UseOfATL, tool.UseOfATL, /*ifNot*/ useATLNotSet)
            << attrE(_UseOfMfc, tool.UseOfMfc)
            << attrT(_WholeProgramOptimization, tool.WholeProgramOptimization);
    write(xml, tool.compiler);
    write(xml, tool.custom);
    if (tool.ConfigurationType == typeStaticLibrary)
        write(xml, tool.librarian);
    else
        write(xml, tool.linker);
    write(xml, tool.idl);
    write(xml, tool.postBuild);
    write(xml, tool.preBuild);
    write(xml, tool.preLink);
    write(xml, tool.resource);
    write(xml, tool.deployment);
    xml << closetag(_Configuration);
}

void VCProjectWriter::write(XmlOutput &xml, VCFilter &tool)
{
    if(!tool.Files.count())
        return;

    if (!tool.Name.isEmpty()) {
        xml << tag(_Filter)
                << attrS(_Name, tool.Name)
                << attrS(_Filter, tool.Filter)
                << attrS(_UniqueIdentifier, tool.Guid)
                << attrT(_ParseFiles, tool.ParseFiles);
    }
    for (int i = 0; i < tool.Files.count(); ++i) {
        const VCFilterFile &info = tool.Files.at(i);
        xml << tag(q_File)
                << attrS(_RelativePath, Option::fixPathToLocalOS(info.file))
            << data(); // In case no custom builds, to avoid "/>" endings
        outputFileConfig(tool, xml, tool.Files.at(i).file);
        xml << closetag(q_File);
    }
    if (!tool.Name.isEmpty())
        xml << closetag(_Filter);
}

// outputs a given filter for all existing configurations of a project
void VCProjectWriter::outputFilter(VCProject &project, XmlOutput &xml, const QString &filtername)
{
    Node *root;
    if (project.SingleProjects.at(0).flat_files)
        root = new FlatNode;
    else
        root = new TreeNode;

    QString name, extfilter, guid;
    triState parse;

    for (int i = 0; i < project.SingleProjects.count(); ++i) {
        VCFilter filter;
        const VCProjectSingleConfig &projectSingleConfig = project.SingleProjects.at(i);
        if (filtername == "RootFiles") {
            filter = projectSingleConfig.RootFiles;
        } else if (filtername == "Sources") {
            filter = projectSingleConfig.SourceFiles;
        } else if (filtername == "Headers") {
            filter = projectSingleConfig.HeaderFiles;
        } else if (filtername == "GeneratedFiles") {
            filter = projectSingleConfig.GeneratedFiles;
        } else if (filtername == "LexYaccFiles") {
            filter = projectSingleConfig.LexYaccFiles;
        } else if (filtername == "TranslationFiles") {
            filter = projectSingleConfig.TranslationFiles;
        } else if (filtername == "FormFiles") {
            filter = projectSingleConfig.FormFiles;
        } else if (filtername == "ResourceFiles") {
            filter = projectSingleConfig.ResourceFiles;
        } else {
            // ExtraCompilers
            filter = project.SingleProjects[i].filterForExtraCompiler(filtername);
        }

        // Merge all files in this filter to root tree
        for (int x = 0; x < filter.Files.count(); ++x)
            root->addElement(filter.Files.at(x));

        // Save filter setting from first filter. Next filters
        // may differ but we cannot handle that. (ex. extfilter)
        if (name.isEmpty()) {
            name = filter.Name;
            extfilter = filter.Filter;
            parse = filter.ParseFiles;
            guid = filter.Guid;
        }
    }

    if (!root->hasElements())
        return;

    // Actual XML output ----------------------------------
    if (!name.isEmpty()) {
        xml << tag(_Filter)
                << attrS(_Name, name)
                << attrS(_Filter, extfilter)
                << attrS(_UniqueIdentifier, guid)
                << attrT(_ParseFiles, parse);
    }
    root->generateXML(xml, "", project, filtername); // output root tree
    if (!name.isEmpty())
        xml << closetag(_Filter);
}

// Output all configurations (by filtername) for a file (by info)
// A filters config output is in VCFilter.outputFileConfig()
void VCProjectWriter::outputFileConfigs(VCProject &project, XmlOutput &xml, const VCFilterFile &info, const QString &filtername)
{
    xml << tag(q_File)
            << attrS(_RelativePath, Option::fixPathToLocalOS(info.file));
    for (int i = 0; i < project.SingleProjects.count(); ++i) {
        VCFilter filter;
        const VCProjectSingleConfig &projectSingleConfig = project.SingleProjects.at(i);
        if (filtername == "RootFiles") {
            filter = projectSingleConfig.RootFiles;
        } else if (filtername == "Sources") {
            filter = projectSingleConfig.SourceFiles;
        } else if (filtername == "Headers") {
            filter = projectSingleConfig.HeaderFiles;
        } else if (filtername == "GeneratedFiles") {
            filter = projectSingleConfig.GeneratedFiles;
        } else if (filtername == "LexYaccFiles") {
            filter = projectSingleConfig.LexYaccFiles;
        } else if (filtername == "TranslationFiles") {
            filter = projectSingleConfig.TranslationFiles;
        } else if (filtername == "FormFiles") {
            filter = projectSingleConfig.FormFiles;
        } else if (filtername == "ResourceFiles") {
            filter = projectSingleConfig.ResourceFiles;
        } else {
            // ExtraCompilers
            filter = project.SingleProjects[i].filterForExtraCompiler(filtername);
        }

        if (filter.Config) // only if the filter is not empty
            outputFileConfig(filter, xml, info.file);
    }
    xml << closetag(q_File);
}

void VCProjectWriter::outputFileConfig(VCFilter &filter, XmlOutput &xml, const QString &filename)
{
    // Clearing each filter tool
    filter.useCustomBuildTool = false;
    filter.useCompilerTool = false;
    filter.CustomBuildTool = VCCustomBuildTool();
    filter.CompilerTool = VCCLCompilerTool();

    // Unset some default options
    filter.CustomBuildTool.config = filter.Config;
    filter.CompilerTool.BufferSecurityCheck = unset;
    filter.CompilerTool.DebugInformationFormat = debugUnknown;
    filter.CompilerTool.ExceptionHandling = ehDefault;
    filter.CompilerTool.GeneratePreprocessedFile = preprocessUnknown;
    filter.CompilerTool.Optimization = optimizeDefault;
    filter.CompilerTool.ProgramDataBaseFileName.clear();
    filter.CompilerTool.RuntimeLibrary = rtUnknown;
    filter.CompilerTool.WarningLevel = warningLevelUnknown;
    filter.CompilerTool.config = filter.Config;

    bool inBuild = false;
    VCFilterFile info;
    for (int i = 0; i < filter.Files.count(); ++i) {
        if (filter.Files.at(i).file == filename) {
            info = filter.Files.at(i);
            inBuild = true;
        }
    }
    inBuild &= !info.excludeFromBuild;

    if (inBuild) {
        filter.addExtraCompiler(info);
        if(filter.Project->usePCH)
            filter.modifyPCHstage(info.file);
    } else {
        // Excluded files uses an empty compiler stage
        if(info.excludeFromBuild)
            filter.useCompilerTool = true;
    }

    // Actual XML output ----------------------------------
    if (filter.useCustomBuildTool || filter.useCompilerTool || !inBuild) {
        xml << tag(_FileConfiguration)
                << attr(_Name, filter.Config->Name)
                << (!inBuild ? attrS(_ExcludedFromBuild, "true") : noxml());
        if (filter.useCustomBuildTool)
            filter.Project->projectWriter->write(xml, filter.CustomBuildTool);
        if (filter.useCompilerTool)
            filter.Project->projectWriter->write(xml, filter.CompilerTool);
        xml << closetag(_FileConfiguration);
    }
}

QT_END_NAMESPACE
