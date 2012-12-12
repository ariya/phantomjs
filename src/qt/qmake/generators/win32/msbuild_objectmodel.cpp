/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "msvc_objectmodel.h"
#include "msbuild_objectmodel.h"
#include "msvc_vcproj.h"
#include "msvc_vcxproj.h"
#include <qstringlist.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

// XML Tags ---------------------------------------------------------
const char _CLCompile[]                         = "ClCompile";
const char _ItemGroup[]                         = "ItemGroup";
const char _Link[]                              = "Link";
const char _Midl[]                              = "Midl";
const char _ResourceCompile[]                   = "ResourceCompile";

// XML Properties ---------------------------------------------------
const char _AddModuleNamesToAssembly[]          = "AddModuleNamesToAssembly";
const char _AdditionalDependencies[]            = "AdditionalDependencies";
const char _AdditionalIncludeDirectories[]      = "AdditionalIncludeDirectories";
const char _AdditionalLibraryDirectories[]      = "AdditionalLibraryDirectories";
const char _AdditionalManifestDependencies[]    = "AdditionalManifestDependencies";
const char _AdditionalOptions[]                 = "AdditionalOptions";
const char _AdditionalUsingDirectories[]        = "AdditionalUsingDirectories";
const char _AllowIsolation[]                    = "AllowIsolation";
const char _AlwaysAppend[]                      = "AlwaysAppend";
const char _ApplicationConfigurationMode[]      = "ApplicationConfigurationMode";
const char _AssemblerListingLocation[]          = "AssemblerListingLocation";
const char _AssemblerOutput[]                   = "AssemblerOutput";
const char _AssemblyDebug[]                     = "AssemblyDebug";
const char _AssemblyLinkResource[]              = "AssemblyLinkResource";
const char _ATLMinimizesCRunTimeLibraryUsage[]  = "ATLMinimizesCRunTimeLibraryUsage";
const char _BaseAddress[]                       = "BaseAddress";
const char _BasicRuntimeChecks[]                = "BasicRuntimeChecks";
const char _BrowseInformation[]                 = "BrowseInformation";
const char _BrowseInformationFile[]             = "BrowseInformationFile";
const char _BufferSecurityCheck[]               = "BufferSecurityCheck";
const char _BuildBrowserInformation[]           = "BuildBrowserInformation";
const char _CallingConvention[]                 = "CallingConvention";
const char _CharacterSet[]                      = "CharacterSet";
const char _ClientStubFile[]                    = "ClientStubFile";
const char _CLRImageType[]                      = "CLRImageType";
const char _CLRSupportLastError[]               = "CLRSupportLastError";
const char _CLRThreadAttribute[]                = "CLRThreadAttribute";
const char _CLRUnmanagedCodeCheck[]             = "CLRUnmanagedCodeCheck";
const char _Command[]                           = "Command";
const char _CompileAs[]                         = "CompileAs";
const char _CompileAsManaged[]                  = "CompileAsManaged";
const char _ConfigurationType[]                 = "ConfigurationType";
const char _CPreprocessOptions[]                = "CPreprocessOptions";
const char _CreateHotpatchableImage[]           = "CreateHotpatchableImage";
const char _CreateHotPatchableImage[]           = "CreateHotPatchableImage";
const char _Culture[]                           = "Culture";
const char _DataExecutionPrevention[]           = "DataExecutionPrevention";
const char _DebugInformationFormat[]            = "DebugInformationFormat";
const char _DefaultCharType[]                   = "DefaultCharType";
const char _DelayLoadDLLs[]                     = "DelayLoadDLLs";
const char _DelaySign[]                         = "DelaySign";
const char _DeleteExtensionsOnClean[]           = "DeleteExtensionsOnClean";
const char _DisableLanguageExtensions[]         = "DisableLanguageExtensions";
const char _DisableSpecificWarnings[]           = "DisableSpecificWarnings";
const char _DisplayLibrary[]                    = "DisplayLibrary";
const char _DLLDataFileName[]                   = "DLLDataFileName";
const char _EmbedManagedResourceFile[]          = "EmbedManagedResourceFile";
const char _EnableCOMDATFolding[]               = "EnableCOMDATFolding";
const char _EnableUAC[]                         = "EnableUAC";
const char _EnableErrorChecks[]                 = "EnableErrorChecks";
const char _EnableEnhancedInstructionSet[]      = "EnableEnhancedInstructionSet";
const char _EnableFiberSafeOptimizations[]      = "EnableFiberSafeOptimizations";
const char _EnablePREfast[]                     = "EnablePREfast";
const char _EntryPointSymbol[]                  = "EntryPointSymbol";
const char _ErrorCheckAllocations[]             = "ErrorCheckAllocations";
const char _ErrorCheckBounds[]                  = "ErrorCheckBounds";
const char _ErrorCheckEnumRange[]               = "ErrorCheckEnumRange";
const char _ErrorCheckRefPointers[]             = "ErrorCheckRefPointers";
const char _ErrorCheckStubData[]                = "ErrorCheckStubData";
const char _ErrorReporting[]                    = "ErrorReporting";
const char _ExceptionHandling[]                 = "ExceptionHandling";
const char _ExpandAttributedSource[]            = "ExpandAttributedSource";
const char _ExportNamedFunctions[]              = "ExportNamedFunctions";
const char _FavorSizeOrSpeed[]                  = "FavorSizeOrSpeed";
const char _FloatingPointModel[]                = "FloatingPointModel";
const char _FloatingPointExceptions[]           = "FloatingPointExceptions";
const char _ForceConformanceInForLoopScope[]    = "ForceConformanceInForLoopScope";
const char _ForceSymbolReferences[]             = "ForceSymbolReferences";
const char _ForcedIncludeFiles[]                = "ForcedIncludeFiles";
const char _ForcedUsingFiles[]                  = "ForcedUsingFiles";
const char _FunctionLevelLinking[]              = "FunctionLevelLinking";
const char _FunctionOrder[]                     = "FunctionOrder";
const char _GenerateClientFiles[]               = "GenerateClientFiles";
const char _GenerateDebugInformation[]          = "GenerateDebugInformation";
const char _GenerateManifest[]                  = "GenerateManifest";
const char _GenerateMapFile[]                   = "GenerateMapFile";
const char _GenerateServerFiles[]               = "GenerateServerFiles";
const char _GenerateStublessProxies[]           = "GenerateStublessProxies";
const char _GenerateTypeLibrary[]               = "GenerateTypeLibrary";
const char _GenerateXMLDocumentationFiles[]     = "GenerateXMLDocumentationFiles";
const char _HeaderFileName[]                    = "HeaderFileName";
const char _HeapCommitSize[]                    = "HeapCommitSize";
const char _HeapReserveSize[]                   = "HeapReserveSize";
const char _IgnoreAllDefaultLibraries[]         = "IgnoreAllDefaultLibraries";
const char _IgnoreEmbeddedIDL[]                 = "IgnoreEmbeddedIDL";
const char _IgnoreImportLibrary[]               = "IgnoreImportLibrary";
const char _IgnoreSpecificDefaultLibraries[]    = "IgnoreSpecificDefaultLibraries";
const char _IgnoreStandardIncludePath[]         = "IgnoreStandardIncludePath";
const char _ImportLibrary[]                     = "ImportLibrary";
const char _InlineFunctionExpansion[]           = "InlineFunctionExpansion";
const char _IntrinsicFunctions[]                = "IntrinsicFunctions";
const char _InterfaceIdentifierFileName[]       = "InterfaceIdentifierFileName";
const char _IntermediateDirectory[]             = "IntermediateDirectory";
const char _KeyContainer[]                      = "KeyContainer";
const char _KeyFile[]                           = "KeyFile";
const char _LargeAddressAware[]                 = "LargeAddressAware";
const char _LinkDLL[]                           = "LinkDLL";
const char _LinkErrorReporting[]                = "LinkErrorReporting";
const char _LinkIncremental[]                   = "LinkIncremental";
const char _LinkStatus[]                        = "LinkStatus";
const char _LinkTimeCodeGeneration[]            = "LinkTimeCodeGeneration";
const char _LocaleID[]                          = "LocaleID";
const char _ManifestFile[]                      = "ManifestFile";
const char _MapExports[]                        = "MapExports";
const char _MapFileName[]                       = "MapFileName";
const char _MergedIDLBaseFileName[]             = "MergedIDLBaseFileName";
const char _MergeSections[]                     = "MergeSections";
const char _Message[]                           = "Message";
const char _MidlCommandFile[]                   = "MidlCommandFile";
const char _MinimalRebuild[]                    = "MinimalRebuild";
const char _MkTypLibCompatible[]                = "MkTypLibCompatible";
const char _ModuleDefinitionFile[]              = "ModuleDefinitionFile";
const char _MultiProcessorCompilation[]         = "MultiProcessorCompilation";
const char _Name[]                              = "Name";
const char _NoEntryPoint[]                      = "NoEntryPoint";
const char _NullTerminateStrings[]              = "NullTerminateStrings";
const char _ObjectFiles[]                       = "ObjectFiles";
const char _ObjectFileName[]                    = "ObjectFileName";
const char _OmitDefaultLibName[]                = "OmitDefaultLibName";
const char _OmitFramePointers[]                 = "OmitFramePointers";
const char _OpenMPSupport[]                     = "OpenMPSupport";
const char _Optimization[]                      = "Optimization";
const char _OptimizeReferences[]                = "OptimizeReferences";
const char _OutputDirectory[]                   = "OutputDirectory";
const char _OutputFile[]                        = "OutputFile";
const char _PrecompiledHeader[]                 = "PrecompiledHeader";
const char _PrecompiledHeaderFile[]             = "PrecompiledHeaderFile";
const char _PrecompiledHeaderOutputFile[]       = "PrecompiledHeaderOutputFile";
const char _PreprocessorDefinitions[]           = "PreprocessorDefinitions";
const char _PreprocessKeepComments[]            = "PreprocessKeepComments";
const char _PreprocessOutputPath[]              = "PreprocessOutputPath";
const char _PreprocessSuppressLineNumbers[]     = "PreprocessSuppressLineNumbers";
const char _PreprocessToFile[]                  = "PreprocessToFile";
const char _PreventDllBinding[]                 = "PreventDllBinding";
const char _PrimaryOutput[]                     = "PrimaryOutput";
const char _ProcessorNumber[]                   = "ProcessorNumber";
const char _ProgramDatabase[]                   = "ProgramDatabase";
const char _ProgramDataBaseFileName[]           = "ProgramDataBaseFileName";
const char _ProgramDatabaseFile[]               = "ProgramDatabaseFile";
const char _ProxyFileName[]                     = "ProxyFileName";
const char _RandomizedBaseAddress[]             = "RandomizedBaseAddress";
const char _RedirectOutputAndErrors[]           = "RedirectOutputAndErrors";
const char _RegisterOutput[]                    = "RegisterOutput";
const char _RemoveObjects[]                     = "RemoveObjects";
const char _ResourceOutputFileName[]            = "ResourceOutputFileName";
const char _RuntimeLibrary[]                    = "RuntimeLibrary";
const char _RuntimeTypeInfo[]                   = "RuntimeTypeInfo";
const char _SectionAlignment[]                  = "SectionAlignment";
const char _ServerStubFile[]                    = "ServerStubFile";
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
const char _SupportNobindOfDelayLoadedDLL[]     = "SupportNobindOfDelayLoadedDLL";
const char _SupportUnloadOfDelayLoadedDLL[]     = "SupportUnloadOfDelayLoadedDLL";
const char _SuppressCompilerWarnings[]          = "SuppressCompilerWarnings";
const char _SuppressStartupBanner[]             = "SuppressStartupBanner";
const char _SwapRunFromCD[]                     = "SwapRunFromCD";
const char _SwapRunFromNet[]                    = "SwapRunFromNet";
const char _TargetEnvironment[]                 = "TargetEnvironment";
const char _TargetMachine[]                     = "TargetMachine";
const char _TerminalServerAware[]               = "TerminalServerAware";
const char _TrackerLogDirectory[]               = "TrackerLogDirectory";
const char _TreatLibWarningAsErrors[]           = "TreatLibWarningAsErrors";
const char _TreatLinkerWarningAsErrors[]        = "TreatLinkerWarningAsErrors";
const char _TreatSpecificWarningsAsErrors[]     = "TreatSpecificWarningsAsErrors";
const char _TreatWarningAsError[]               = "TreatWarningAsError";
const char _TreatWChar_tAsBuiltInType[]         = "TreatWChar_tAsBuiltInType";
const char _TurnOffAssemblyGeneration[]         = "TurnOffAssemblyGeneration";
const char _TypeLibFormat[]                     = "TypeLibFormat";
const char _TypeLibraryFile[]                   = "TypeLibraryFile";
const char _TypeLibraryName[]                   = "TypeLibraryName";
const char _TypeLibraryResourceID[]             = "TypeLibraryResourceID";
const char _UACExecutionLevel[]                 = "UACExecutionLevel";
const char _UACUIAccess[]                       = "UACUIAccess";
const char _UndefineAllPreprocessorDefinitions[]= "UndefineAllPreprocessorDefinitions";
const char _UndefinePreprocessorDefinitions[]   = "UndefinePreprocessorDefinitions";
const char _UseFullPaths[]                      = "UseFullPaths";
const char _UseOfATL[]                          = "UseOfATL";
const char _UseOfMfc[]                          = "UseOfMfc";
const char _UseUnicodeForAssemblerListing[]     = "UseUnicodeForAssemblerListing";
const char _ValidateAllParameters[]             = "ValidateAllParameters";
const char _VCCustomBuildTool[]                 = "VCCustomBuildTool";
const char _Verbose[]                           = "Verbose";
const char _Version[]                           = "Version";
const char _WarnAsError[]                       = "WarnAsError";
const char _WarningLevel[]                      = "WarningLevel";
const char _WholeProgramOptimization[]          = "WholeProgramOptimization";
const char _XMLDocumentationFileName[]          = "XMLDocumentationFileName";


// XmlOutput stream functions ------------------------------
inline XmlOutput::xml_output attrTagT(const char *name, const triState v)
{
    if(v == unset)
        return noxml();
    return tagValue(name, (v == _True ? "true" : "false"));
}

inline XmlOutput::xml_output attrTagL(const char *name, qint64 v)
{
    return tagValue(name, QString::number(v));
}

/*ifNot version*/
inline XmlOutput::xml_output attrTagL(const char *name, qint64 v, qint64 ifn)
{
    if (v == ifn)
        return noxml();
    return tagValue(name, QString::number(v));
}

inline XmlOutput::xml_output attrTagS(const char *name, const QString &v)
{
    if(v.isEmpty())
        return noxml();
    return tagValue(name, v);
}

inline XmlOutput::xml_output attrTagX(const char *name, const QStringList &v, const char *s = ",")
{
    if(v.isEmpty())
        return noxml();
    QStringList temp = v;
    temp.append(QString("%(%1)").arg(name));
    return tagValue(name, temp.join(s));
}

inline XmlOutput::xml_output valueTagX(const QStringList &v, const char *s = " ")
{
    if(v.isEmpty())
        return noxml();
    return valueTag(v.join(s));
}

inline XmlOutput::xml_output valueTagDefX(const QStringList &v, const QString &tagName, const char *s = " ")
{
    if(v.isEmpty())
        return noxml();
    QStringList temp = v;
    temp.append(QString("%(%1)").arg(tagName));
    return valueTag(temp.join(s));
}

inline XmlOutput::xml_output valueTagT( const triState v)
{
    if(v == unset)
        return noxml();
    return valueTag(v == _True ? "true" : "false");
}

static QString vcxCommandSeparator()
{
    // MSBuild puts the contents of the custom commands into a batch file and calls it.
    // As we want every sub-command to be error-checked (as is done by makefile-based
    // backends), we insert the checks ourselves, using the undocumented jump target.
    static QString cmdSep =
    QLatin1String("&#x000D;&#x000A;if errorlevel 1 goto VCEnd&#x000D;&#x000A;");
    return cmdSep;
}

// Tree file generation ---------------------------------------------
void XTreeNode::generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName, VCProject &tool, const QString &filter) {

    if (children.size()) {
        // Filter
        QString tempFilterName;
        ChildrenMap::ConstIterator it, end = children.constEnd();
        if (!tagName.isEmpty()) {
            tempFilterName.append(filter);
            tempFilterName.append("\\");
            tempFilterName.append(tagName);
            xmlFilter << tag(_ItemGroup);
            xmlFilter << tag("Filter")
                      << attrTag("Include", tempFilterName)
                      << closetag();
            xmlFilter << closetag();
        }
        // First round, do nested filters
        for (it = children.constBegin(); it != end; ++it)
            if ((*it)->children.size())
            {
                if ( !tempFilterName.isEmpty() )
                    (*it)->generateXML(xml, xmlFilter, it.key(), tool, tempFilterName);
                else
                    (*it)->generateXML(xml, xmlFilter, it.key(), tool, filter);
            }
        // Second round, do leafs
        for (it = children.constBegin(); it != end; ++it)
            if (!(*it)->children.size())
            {
                if ( !tempFilterName.isEmpty() )
                    (*it)->generateXML(xml, xmlFilter, it.key(), tool, tempFilterName);
                else
                    (*it)->generateXML(xml, xmlFilter, it.key(), tool, filter);
            }
    } else {
        // Leaf
        xml << tag(_ItemGroup);
        xmlFilter << tag(_ItemGroup);
        VCXProjectWriter::outputFileConfigs(tool, xml, xmlFilter, info, filter);
        xmlFilter << closetag();
        xml << closetag();
    }
}

// Flat file generation ---------------------------------------------
void XFlatNode::generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &/*tagName*/, VCProject &tool, const QString &filter) {
    if (children.size()) {
        ChildrenMapFlat::ConstIterator it = children.constBegin();
        ChildrenMapFlat::ConstIterator end = children.constEnd();
        xml << tag(_ItemGroup);
        xmlFilter << tag(_ItemGroup);
        for (; it != end; ++it) {
            VCXProjectWriter::outputFileConfigs(tool, xml, xmlFilter, (*it), filter);
        }
        xml << closetag();
        xmlFilter << closetag();
    }
}

void VCXProjectWriter::write(XmlOutput &xml, VCProjectSingleConfig &tool)
{
    xml.setIndentString("  ");

    xml << decl("1.0", "utf-8")
        << tag("Project")
        << attrTag("DefaultTargets","Build")
        << attrTag("ToolsVersion", "4.0")
        << attrTag("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003")
        << tag("ItemGroup")
        << attrTag("Label", "ProjectConfigurations");

    xml << tag("ProjectConfiguration")
        << attrTag("Include" , tool.Configuration.Name)
        << tagValue("Configuration", tool.Configuration.ConfigurationName)
        << tagValue("Platform", tool.PlatformName)
        << closetag();

    xml << closetag()
        << tag("PropertyGroup")
        << attrTag("Label", "Globals")
        << tagValue("ProjectGuid", tool.ProjectGUID)
        << tagValue("RootNamespace", tool.Name)
        << tagValue("Keyword", tool.Keyword)
        << closetag();

    // config part.
    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");

    write(xml, tool.Configuration);
    const QString condition = generateCondition(tool.Configuration);

    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");

    // Extension settings
    xml << tag("ImportGroup")
        << attrTag("Label", "ExtensionSettings")
        << closetag();

    // PropertySheets
    xml << tag("ImportGroup")
        << attrTag("Condition", condition)
        << attrTag("Label", "PropertySheets");

    xml << tag("Import")
        << attrTag("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props")
        << attrTag("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')")
        << closetag()
        << closetag();

    // UserMacros
    xml << tag("PropertyGroup")
        << attrTag("Label", "UserMacros")
        << closetag();

    xml << tag("PropertyGroup");

    if ( !tool.Configuration.OutputDirectory.isEmpty() ) {
        xml<< tag("OutDir")
            << attrTag("Condition", condition)
            << valueTag(tool.Configuration.OutputDirectory);
    }
    if ( !tool.Configuration.IntermediateDirectory.isEmpty() ) {
        xml<< tag("IntDir")
            << attrTag("Condition", condition)
            << valueTag(tool.Configuration.IntermediateDirectory);
    }
    if ( !tool.Configuration.PrimaryOutput.isEmpty() ) {
        xml<< tag("TargetName")
            << attrTag("Condition", condition)
            << valueTag(tool.Configuration.PrimaryOutput);
    }

    if ( tool.Configuration.linker.IgnoreImportLibrary != unset) {
        xml<< tag("IgnoreImportLibrary")
            << attrTag("Condition", condition)
            << valueTagT(tool.Configuration.linker.IgnoreImportLibrary);
    }

    if ( tool.Configuration.linker.LinkIncremental != linkIncrementalDefault) {
        const triState ts = (tool.Configuration.linker.LinkIncremental == linkIncrementalYes ? _True : _False);
        xml<< tag("LinkIncremental")
            << attrTag("Condition", condition)
            << valueTagT(ts);
    }

    if ( tool.Configuration.preBuild.ExcludedFromBuild != unset )
    {
        xml<< tag("PreBuildEventUseInBuild")
            << attrTag("Condition", condition)
            << valueTagT(!tool.Configuration.preBuild.ExcludedFromBuild);
    }

    if ( tool.Configuration.preLink.ExcludedFromBuild != unset )
    {
        xml<< tag("PreLinkEventUseInBuild")
            << attrTag("Condition", condition)
            << valueTagT(!tool.Configuration.preLink.ExcludedFromBuild);
    }

    if ( tool.Configuration.postBuild.ExcludedFromBuild != unset )
    {
        xml<< tag("PostBuildEventUseInBuild")
            << attrTag("Condition", condition)
            << valueTagT(!tool.Configuration.postBuild.ExcludedFromBuild);
    }
    xml << closetag();

    xml << tag("ItemDefinitionGroup")
        << attrTag("Condition", condition);

    // ClCompile
    write(xml, tool.Configuration.compiler);

    // Link
    write(xml, tool.Configuration.linker);

    // Midl
    write(xml, tool.Configuration.idl);

    // ResourceCompiler
    write(xml, tool.Configuration.resource);

    // Post build event
    if ( tool.Configuration.postBuild.ExcludedFromBuild != unset )
        write(xml, tool.Configuration.postBuild);

    // Pre build event
    if ( tool.Configuration.preBuild.ExcludedFromBuild != unset )
        write(xml, tool.Configuration.preBuild);

    // Pre link event
    if ( tool.Configuration.preLink.ExcludedFromBuild != unset )
        write(xml, tool.Configuration.preLink);

    xml << closetag();

    QFile filterFile;
    filterFile.setFileName(Option::output.fileName().append(".filters"));
    filterFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream ts(&filterFile);
    XmlOutput xmlFilter(ts, XmlOutput::NoConversion);

    xmlFilter.setIndentString("  ");

    xmlFilter << decl("1.0", "utf-8")
              << tag("Project")
              << attrTag("ToolsVersion", "4.0")
              << attrTag("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

    xmlFilter << tag("ItemGroup");

    VCProject tempProj;
    tempProj.SingleProjects += tool;

    addFilters(tempProj, xmlFilter, "Form Files");
    addFilters(tempProj, xmlFilter, "Generated Files");
    addFilters(tempProj, xmlFilter, "Header Files");
    addFilters(tempProj, xmlFilter, "LexYacc Files");
    addFilters(tempProj, xmlFilter, "Resource Files");
    addFilters(tempProj, xmlFilter, "Source Files");
    addFilters(tempProj, xmlFilter, "Translation Files");
    xmlFilter << closetag();

    outputFilter(tempProj, xml, xmlFilter, "Source Files");
    outputFilter(tempProj, xml, xmlFilter, "Header Files");
    outputFilter(tempProj, xml, xmlFilter, "Generated Files");
    outputFilter(tempProj, xml, xmlFilter, "LexYacc Files");
    outputFilter(tempProj, xml, xmlFilter, "Translation Files");
    outputFilter(tempProj, xml, xmlFilter, "Form Files");
    outputFilter(tempProj, xml, xmlFilter, "Resource Files");

    for (int x = 0; x < tempProj.ExtraCompilers.count(); ++x) {
        outputFilter(tempProj, xml, xmlFilter, tempProj.ExtraCompilers.at(x));
    }

    outputFilter(tempProj, xml, xmlFilter, "Root Files");

    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");

    xml << tag("ImportGroup")
        << attrTag("Label", "ExtensionTargets")
        << closetag();
}

void VCXProjectWriter::write(XmlOutput &xml, VCProject &tool)
{
    if (tool.SingleProjects.count() == 0) {
        warn_msg(WarnLogic, "Generator: .NET: no single project in merge project, no output");
        return;
    }

    xml.setIndentString("  ");

    xml << decl("1.0", "utf-8")
        << tag("Project")
        << attrTag("DefaultTargets","Build")
        << attrTag("ToolsVersion", "4.0")
        << attrTag("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003")
        << tag("ItemGroup")
        << attrTag("Label", "ProjectConfigurations");

    for (int i = 0; i < tool.SingleProjects.count(); ++i) {
        xml << tag("ProjectConfiguration")
            << attrTag("Include" , tool.SingleProjects.at(i).Configuration.Name)
            << tagValue("Configuration", tool.SingleProjects.at(i).Configuration.ConfigurationName)
            << tagValue("Platform", tool.SingleProjects.at(i).PlatformName)
            << closetag();
    }

    xml << closetag()
        << tag("PropertyGroup")
        << attrTag("Label", "Globals")
        << tagValue("ProjectGuid", tool.ProjectGUID)
        << tagValue("RootNamespace", tool.Name)
        << tagValue("Keyword", tool.Keyword)
        << closetag();

    // config part.
    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
    for (int i = 0; i < tool.SingleProjects.count(); ++i)
        write(xml, tool.SingleProjects.at(i).Configuration);
    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");

    // Extension settings
    xml << tag("ImportGroup")
        << attrTag("Label", "ExtensionSettings")
        << closetag();

    // PropertySheets
    for (int i = 0; i < tool.SingleProjects.count(); ++i) {
        xml << tag("ImportGroup")
            << attrTag("Condition", generateCondition(tool.SingleProjects.at(i).Configuration))
            << attrTag("Label", "PropertySheets");

        xml << tag("Import")
            << attrTag("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props")
            << attrTag("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')")
            << closetag()
            << closetag();
    }

    // UserMacros
    xml << tag("PropertyGroup")
        << attrTag("Label", "UserMacros")
        << closetag();

    xml << tag("PropertyGroup");
    for (int i = 0; i < tool.SingleProjects.count(); ++i) {
        const VCConfiguration &config = tool.SingleProjects.at(i).Configuration;
        const QString condition = generateCondition(config);

        if (!config.OutputDirectory.isEmpty()) {
            xml << tag("OutDir")
                << attrTag("Condition", condition)
                << valueTag(config.OutputDirectory);
        }
        if (!config.IntermediateDirectory.isEmpty()) {
            xml << tag("IntDir")
                << attrTag("Condition", condition)
                << valueTag(config.IntermediateDirectory);
        }
        if (!config.PrimaryOutput.isEmpty()) {
            xml << tag("TargetName")
                << attrTag("Condition", condition)
                << valueTag(config.PrimaryOutput);
        }

        if (config.linker.IgnoreImportLibrary != unset) {
            xml << tag("IgnoreImportLibrary")
                << attrTag("Condition", condition)
                << valueTagT(config.linker.IgnoreImportLibrary);
        }

        if (config.linker.LinkIncremental != linkIncrementalDefault) {
            const triState ts = (config.linker.LinkIncremental == linkIncrementalYes ? _True : _False);
            xml << tag("LinkIncremental")
                << attrTag("Condition", condition)
                << valueTagT(ts);
        }

        const triState generateManifest = config.linker.GenerateManifest;
        if (generateManifest != unset) {
            xml << tag("GenerateManifest")
                << attrTag("Condition", condition)
                << valueTagT(generateManifest);
        }

        if (config.preBuild.ExcludedFromBuild != unset)
        {
            xml << tag("PreBuildEventUseInBuild")
                << attrTag("Condition", condition)
                << valueTagT(!config.preBuild.ExcludedFromBuild);
        }

        if (config.preLink.ExcludedFromBuild != unset)
        {
            xml << tag("PreLinkEventUseInBuild")
                << attrTag("Condition", condition)
                << valueTagT(!config.preLink.ExcludedFromBuild);
        }

        if (config.postBuild.ExcludedFromBuild != unset)
        {
            xml << tag("PostBuildEventUseInBuild")
                << attrTag("Condition", condition)
                << valueTagT(!config.postBuild.ExcludedFromBuild);
        }
    }
    xml << closetag();

    for (int i = 0; i < tool.SingleProjects.count(); ++i) {
        const VCConfiguration &config = tool.SingleProjects.at(i).Configuration;

        xml << tag("ItemDefinitionGroup")
            << attrTag("Condition", generateCondition(config));

        // ClCompile
        write(xml, config.compiler);

        // Link
        write(xml, config.linker);

        // Midl
        write(xml, config.idl);

        // ResourceCompiler
        write(xml, config.resource);

        // Post build event
        if (config.postBuild.ExcludedFromBuild != unset)
            write(xml, config.postBuild);

        // Pre build event
        if (config.preBuild.ExcludedFromBuild != unset)
            write(xml, config.preBuild);

        // Pre link event
        if (config.preLink.ExcludedFromBuild != unset)
            write(xml, config.preLink);

        xml << closetag();
    }

    // The file filters are added in a separate file for MSBUILD.
    QFile filterFile;
    filterFile.setFileName(Option::output.fileName().append(".filters"));
    filterFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream ts(&filterFile);
    XmlOutput xmlFilter(ts, XmlOutput::NoConversion);

    xmlFilter.setIndentString("  ");

    xmlFilter << decl("1.0", "utf-8")
              << tag("Project")
              << attrTag("ToolsVersion", "4.0")
              << attrTag("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

    xmlFilter << tag("ItemGroup");

    addFilters(tool, xmlFilter, "Form Files");
    addFilters(tool, xmlFilter, "Generated Files");
    addFilters(tool, xmlFilter, "Header Files");
    addFilters(tool, xmlFilter, "LexYacc Files");
    addFilters(tool, xmlFilter, "Resource Files");
    addFilters(tool, xmlFilter, "Source Files");
    addFilters(tool, xmlFilter, "Translation Files");
    xmlFilter << closetag();

    outputFilter(tool, xml, xmlFilter, "Source Files");
    outputFilter(tool, xml, xmlFilter, "Header Files");
    outputFilter(tool, xml, xmlFilter, "Generated Files");
    outputFilter(tool, xml, xmlFilter, "LexYacc Files");
    outputFilter(tool, xml, xmlFilter, "Translation Files");
    outputFilter(tool, xml, xmlFilter, "Form Files");
    outputFilter(tool, xml, xmlFilter, "Resource Files");
    for (int x = 0; x < tool.ExtraCompilers.count(); ++x) {
        outputFilter(tool, xml, xmlFilter, tool.ExtraCompilers.at(x));
    }
    outputFilter(tool, xml, xmlFilter, "Root Files");

    xml << import("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");

    xml << tag("ImportGroup")
        << attrTag("Label", "ExtensionTargets")
        << closetag();
}

static inline QString toString(asmListingOption option)
{
    switch (option) {
    case asmListingAsmMachine:
        return "AssemblyAndMachineCode";
    case asmListingAsmMachineSrc:
        return "All";
    case asmListingAsmSrc:
        return "AssemblyAndSourceCode";
    case asmListingAssemblyOnly:
        return "AssemblyCode";
    }
    return QString();
}

static inline QString toString(basicRuntimeCheckOption option)
{
    switch (option) {
    case runtimeBasicCheckNone:
        return "";
    case runtimeCheckStackFrame:
        return "StackFrameRuntimeCheck";
    case runtimeCheckUninitVariables:
        return "UninitializedLocalUsageCheck";
    case runtimeBasicCheckAll:
        return "EnableFastChecks";
    }
    return QString();
}

static inline QString toString(callingConventionOption option)
{
    switch (option) {
    case callConventionCDecl:
        return "Cdecl";
    case callConventionFastCall:
        return "FastCall";
    case callConventionStdCall:
        return "StdCall";
    }
    return QString();
}

static inline QString toString(CompileAsOptions option)
{
    switch (option) {
    case compileAsC:
        return "CompileAsC";
    case compileAsCPlusPlus:
        return "CompileAsCpp";
    }
    return QString();
}

static inline QString toString(compileAsManagedOptions option)
{
    switch (option) {
    case managedAssembly:
        return "true";
    case managedAssemblyPure:
        return "Safe";
    case managedAssemblyOldSyntax:
        return "OldSyntax";
    }
    return QString();
}

static inline QString toString(debugOption option)
{
    switch (option) {
    case debugOldStyleInfo:
        return "OldStyle";
    case debugEditAndContinue:
        return "EditAndContinue";
    case debugEnabled:
        return "ProgramDatabase";
    }
    return QString();
}

static inline QString toString(enhancedInstructionSetOption option)
{
    switch (option) {
    case archSSE:
        return "StreamingSIMDExtensions";
    case archSSE2:
        return "StreamingSIMDExtensions2";
    }
    return QString();
}

static inline QString toString(exceptionHandling option)
{
    switch (option) {
    case ehNone:
        return "false";
    case ehNoSEH:
        return "Sync";
    case ehSEH:
        return "Async";
    }
    return QString();
}

static inline QString toString(favorSizeOrSpeedOption option)
{
    switch (option) {
    case favorSize:
        return "Size";
    case favorSpeed:
        return "Speed";
    }
    return QString();
}

static inline QString toString(floatingPointModel option)
{
    switch (option) {
    case floatingPointFast:
        return "Fast";
    case floatingPointPrecise:
        return "Precise";
    case floatingPointStrict:
        return "Strict";
    }
    return QString();
}

static inline QString toString(inlineExpansionOption option)
{
    switch (option) {
    case expandDisable:
        return "Disabled";
    case expandOnlyInline:
        return "OnlyExplicitInline";
    case expandAnySuitable:
        return "AnySuitable";
    }
    return QString();
}

static inline QString toString(optimizeOption option)
{
    switch (option) {
    case optimizeDisabled:
        return "Disabled";
    case optimizeMinSpace:
        return "MinSpace";
    case optimizeMaxSpeed:
        return "MaxSpeed";
    case optimizeFull:
        return "Full";
    }
    return QString();
}

static inline QString toString(pchOption option)
{
    switch (option) {
    case pchNone:
         return "NotUsing";
    case pchCreateUsingSpecific:
        return "Create";
    case pchUseUsingSpecific:
        return "Use";
    }
    return QString();
}

static inline QString toString(runtimeLibraryOption option)
{
    switch (option) {
    case rtMultiThreaded:
        return "MultiThreaded";
    case rtMultiThreadedDLL:
        return "MultiThreadedDLL";
    case rtMultiThreadedDebug:
        return "MultiThreadedDebug";
    case rtMultiThreadedDebugDLL:
        return "MultiThreadedDebugDLL";
    }
    return QString();
}

static inline QString toString(structMemberAlignOption option)
{
    switch (option) {
    case alignSingleByte:
        return "1Byte";
    case alignTwoBytes:
        return "2Bytes";
    case alignFourBytes:
        return "4Bytes";
    case alignEightBytes:
        return "8Bytes";
    case alignSixteenBytes:
        return "16Bytes";
    }
    return QString();
}

static inline QString toString(warningLevelOption option)
{
    switch (option) {
    case warningLevel_0:
        return "TurnOffAllWarnings";
    case warningLevel_1:
        return "Level1";
    case warningLevel_2:
        return "Level2";
    case warningLevel_3:
        return "Level3";
    case warningLevel_4:
        return "Level4";
    }
    return QString();
}

static inline QString toString(optLinkTimeCodeGenType option)
{
    switch (option) {
    case optLTCGEnabled:
        return "UseLinkTimeCodeGeneration";
    case optLTCGInstrument:
        return "PGInstrument";
    case optLTCGOptimize:
        return "PGOptimization";
    case optLTCGUpdate:
        return "PGUpdate";
    }
    return QString();
}

static inline QString toString(subSystemOption option)
{
    switch (option) {
    case subSystemConsole:
        return "Console";
    case subSystemWindows:
        return "Windows";
    }
    return QString();
}

static inline QString toString(machineTypeOption option)
{
    switch (option) {
    case machineX86:
        return "MachineX86";
    case machineX64:
        return "MachineX64";
    }
    return QString();
}

static inline QString toString(midlCharOption option)
{
    switch (option) {
    case midlCharUnsigned:
        return "Unsigned";
    case midlCharSigned:
        return "Signed";
    case midlCharAscii7:
        return "Ascii";
    }
    return QString();
}

static inline QString toString(midlErrorCheckOption option)
{
    switch (option) {
    case midlDisableAll:
        return "None";
    case midlEnableAll:
        return "All";
    }
    return QString();
}

static inline QString toString(midlStructMemberAlignOption option)
{
    switch (option) {
    case midlAlignSingleByte:
        return "1";
    case midlAlignTwoBytes:
        return "2";
    case midlAlignFourBytes:
        return "4";
    case midlAlignEightBytes:
        return "8";
    case midlAlignSixteenBytes:
        return "16";
    }
    return QString();
}

static inline QString toString(midlTargetEnvironment option)
{
    switch (option) {
    case midlTargetWin32:
        return "Win32";
    case midlTargetWin64:
        return "X64";
    }
    return QString();
}

static inline QString toString(midlWarningLevelOption option)
{
    switch (option) {
    case midlWarningLevel_0:
        return "0";
    case midlWarningLevel_1:
        return "1";
    case midlWarningLevel_2:
        return "2";
    case midlWarningLevel_3:
        return "3";
    case midlWarningLevel_4:
        return "4";
    }
    return QString();
}

static inline QString toString(enumResourceLangID option)
{
    if (option == 0)
        return QString();
    else
        return QString::number(qlonglong(option));
}

static inline QString toString(charSet option)
{
    switch (option) {
    case charSetNotSet:
        return "NotSet";
    case charSetUnicode:
        return "Unicode";
    case charSetMBCS:
        return "MultiByte";
    }
    return QString();
}

static inline QString toString(ConfigurationTypes option)
{
    switch (option) {
    case typeApplication:
        return "Application";
    case typeDynamicLibrary:
        return "DynamicLibrary";
    case typeStaticLibrary:
        return "StaticLibrary";
    }
    return QString();
}

static inline QString toString(useOfATL option)
{
    switch (option) {
    case useATLStatic:
        return "Static";
    case useATLDynamic:
        return "Dynamic";
    }
    return QString();
}

static inline QString toString(useOfMfc option)
{
    switch (option) {
    case useMfcStatic:
        return "Static";
    case useMfcDynamic:
        return "Dynamic";
    }
    return QString();
}

static inline triState toTriState(browseInfoOption option)
{
    switch (option)
    {
    case brInfoNone:
        return _False;
    case brAllInfo:
    case brNoLocalSymbols:
        return _True;
    }
    return unset;
}

static inline triState toTriState(preprocessOption option)
{
    switch (option)
    {
    case preprocessNo:
        return _False;
    case preprocessNoLineNumbers:
    case preprocessYes:
        return _True;
    }
    return unset;
}

static inline triState toTriState(optFoldingType option)
{
    switch (option)
    {
    case optNoFolding:
        return _False;
    case optFolding:
        return _True;
    }
    return unset;
}

static inline triState toTriState(addressAwarenessType option)
{
    switch (option)
    {
    case addrAwareDefault:
        return unset;
    case addrAwareNoLarge:
        return _False;
    case addrAwareLarge:
        return _True;
    }
    return unset;
}

static inline triState toTriState(linkIncrementalType option)
{
    switch (option)
    {
    case linkIncrementalDefault:
        return unset;
    case linkIncrementalNo:
        return _False;
    case linkIncrementalYes:
        return _True;
    }
    return unset;
}

static inline triState toTriState(linkProgressOption option)
{
    switch (option)
    {
    case linkProgressNotSet:
        return unset;
    case linkProgressAll:
    case linkProgressLibs:
        return _True;
    }
    return unset;
}

static inline triState toTriState(optRefType option)
{
    switch (option)
    {
    case optReferencesDefault:
        return unset;
    case optNoReferences:
        return _False;
    case optReferences:
        return _True;
    }
    return unset;
}

static inline triState toTriState(termSvrAwarenessType option)
{
    switch (option)
    {
    case termSvrAwareDefault:
        return unset;
    case termSvrAwareNo:
        return _False;
    case termSvrAwareYes:
        return _True;
    }
    return unset;
}

void VCXProjectWriter::write(XmlOutput &xml, const VCCLCompilerTool &tool)
{
    xml
        << tag(_CLCompile)
            << attrTagX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories, ";")
            << attrTagX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrTagX(_AdditionalUsingDirectories, tool.AdditionalUsingDirectories, ";")
//unused    << attrTagS(_AlwaysAppend, tool.AlwaysAppend)
            << attrTagS(_AssemblerListingLocation, tool.AssemblerListingLocation)
            << attrTagS(_AssemblerOutput, toString(tool.AssemblerOutput))
            << attrTagS(_BasicRuntimeChecks, toString(tool.BasicRuntimeChecks))
            << attrTagT(_BrowseInformation, toTriState(tool.BrowseInformation))
            << attrTagS(_BrowseInformationFile, tool.BrowseInformationFile)
            << attrTagT(_BufferSecurityCheck, tool.BufferSecurityCheck)
            << attrTagS(_CallingConvention, toString(tool.CallingConvention))
            << attrTagS(_CompileAs, toString(tool.CompileAs))
            << attrTagS(_CompileAsManaged, toString(tool.CompileAsManaged))
            << attrTagT(_CreateHotpatchableImage, tool.CreateHotpatchableImage)
            << attrTagS(_DebugInformationFormat, toString(tool.DebugInformationFormat))
            << attrTagT(_DisableLanguageExtensions, tool.DisableLanguageExtensions)
            << attrTagX(_DisableSpecificWarnings, tool.DisableSpecificWarnings, ";")
            << attrTagS(_EnableEnhancedInstructionSet, toString(tool.EnableEnhancedInstructionSet))
            << attrTagT(_EnableFiberSafeOptimizations, tool.EnableFiberSafeOptimizations)
            << attrTagT(_EnablePREfast, tool.EnablePREfast)
            << attrTagS(_ErrorReporting, tool.ErrorReporting)
            << attrTagS(_ExceptionHandling, toString(tool.ExceptionHandling))
            << attrTagT(_ExpandAttributedSource, tool.ExpandAttributedSource)
            << attrTagS(_FavorSizeOrSpeed, toString(tool.FavorSizeOrSpeed))
            << attrTagT(_FloatingPointExceptions, tool.FloatingPointExceptions)
            << attrTagS(_FloatingPointModel, toString(tool.FloatingPointModel))
            << attrTagT(_ForceConformanceInForLoopScope, tool.ForceConformanceInForLoopScope)
            << attrTagX(_ForcedIncludeFiles, tool.ForcedIncludeFiles, ";")
            << attrTagX(_ForcedUsingFiles, tool.ForcedUsingFiles, ";")
            << attrTagT(_FunctionLevelLinking, tool.EnableFunctionLevelLinking)
            << attrTagT(_GenerateXMLDocumentationFiles, tool.GenerateXMLDocumentationFiles)
            << attrTagT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
            << attrTagS(_InlineFunctionExpansion, toString(tool.InlineFunctionExpansion))
            << attrTagT(_IntrinsicFunctions, tool.EnableIntrinsicFunctions)
            << attrTagT(_MinimalRebuild, tool.MinimalRebuild)
            << attrTagT(_MultiProcessorCompilation, tool.MultiProcessorCompilation)
            << attrTagS(_ObjectFileName, tool.ObjectFile)
//unused    << attrTagX(_ObjectFiles, tool.ObjectFiles, ";")
            << attrTagT(_OmitDefaultLibName, tool.OmitDefaultLibName)
            << attrTagT(_OmitFramePointers, tool.OmitFramePointers)
            << attrTagT(_OpenMPSupport, tool.OpenMP)
            << attrTagS(_Optimization, toString(tool.Optimization))
            << attrTagS(_PrecompiledHeader, toString(tool.UsePrecompiledHeader))
            << attrTagS(_PrecompiledHeaderFile, tool.PrecompiledHeaderThrough)
            << attrTagS(_PrecompiledHeaderOutputFile, tool.PrecompiledHeaderFile)
            << attrTagT(_PreprocessKeepComments, tool.KeepComments)
            << attrTagX(_PreprocessorDefinitions, tool.PreprocessorDefinitions, ";")
            << attrTagS(_PreprocessOutputPath, tool.PreprocessOutputPath)
            << attrTagT(_PreprocessSuppressLineNumbers, tool.PreprocessSuppressLineNumbers)
            << attrTagT(_PreprocessToFile, toTriState(tool.GeneratePreprocessedFile))
            << attrTagS(_ProgramDataBaseFileName, tool.ProgramDataBaseFileName)
            << attrTagS(_ProcessorNumber, tool.MultiProcessorCompilationProcessorCount)
            << attrTagS(_RuntimeLibrary, toString(tool.RuntimeLibrary))
            << attrTagT(_RuntimeTypeInfo, tool.RuntimeTypeInfo)
            << attrTagT(_ShowIncludes, tool.ShowIncludes)
            << attrTagT(_SmallerTypeCheck, tool.SmallerTypeCheck)
            << attrTagT(_StringPooling, tool.StringPooling)
            << attrTagS(_StructMemberAlignment, toString(tool.StructMemberAlignment))
            << attrTagT(_SuppressStartupBanner, tool.SuppressStartupBanner)
//unused    << attrTagS(_TreatSpecificWarningsAsErrors, tool.TreatSpecificWarningsAsErrors)
            << attrTagT(_TreatWarningAsError, tool.WarnAsError)
            << attrTagT(_TreatWChar_tAsBuiltInType, tool.TreatWChar_tAsBuiltInType)
            << attrTagT(_UndefineAllPreprocessorDefinitions, tool.UndefineAllPreprocessorDefinitions)
            << attrTagX(_UndefinePreprocessorDefinitions, tool.UndefinePreprocessorDefinitions, ";")
            << attrTagT(_UseFullPaths, tool.DisplayFullPaths)
            << attrTagT(_UseUnicodeForAssemblerListing, tool.UseUnicodeForAssemblerListing)
            << attrTagS(_WarningLevel, toString(tool.WarningLevel))
            << attrTagT(_WholeProgramOptimization, tool.WholeProgramOptimization)
            << attrTagS(_XMLDocumentationFileName, tool.XMLDocumentationFileName)
        << closetag(_CLCompile);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCLinkerTool &tool)
{
    xml
        << tag(_Link)
            << attrTagX(_AdditionalDependencies, tool.AdditionalDependencies, ";")
            << attrTagX(_AdditionalLibraryDirectories, tool.AdditionalLibraryDirectories, ";")
            << attrTagX(_AdditionalManifestDependencies, tool.AdditionalManifestDependencies, ";")
            << attrTagX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrTagX(_AddModuleNamesToAssembly, tool.AddModuleNamesToAssembly, ";")
            << attrTagT(_AllowIsolation, tool.AllowIsolation)
            << attrTagT(_AssemblyDebug, tool.AssemblyDebug)
            << attrTagX(_AssemblyLinkResource, tool.AssemblyLinkResource, ";")
            << attrTagS(_BaseAddress, tool.BaseAddress)
            << attrTagS(_CLRImageType, tool.CLRImageType)
            << attrTagS(_CLRSupportLastError, tool.CLRSupportLastError)
            << attrTagS(_CLRThreadAttribute, tool.CLRThreadAttribute)
            << attrTagT(_CLRUnmanagedCodeCheck, tool.CLRUnmanagedCodeCheck)
//unused    << attrTagS(_CreateHotPatchableImage, tool.CreateHotPatchableImage)
            << attrTagT(_DataExecutionPrevention, tool.DataExecutionPrevention)
            << attrTagX(_DelayLoadDLLs, tool.DelayLoadDLLs, ";")
            << attrTagT(_DelaySign, tool.DelaySign)
            << attrTagS(_EmbedManagedResourceFile, tool.LinkToManagedResourceFile)
            << attrTagT(_EnableCOMDATFolding, toTriState(tool.EnableCOMDATFolding))
            << attrTagT(_EnableUAC, tool.EnableUAC)
            << attrTagS(_EntryPointSymbol, tool.EntryPointSymbol)
            << attrTagX(_ForceSymbolReferences, tool.ForceSymbolReferences, ";")
            << attrTagS(_FunctionOrder, tool.FunctionOrder)
            << attrTagT(_GenerateDebugInformation, tool.GenerateDebugInformation)
            << attrTagT(_GenerateManifest, tool.GenerateManifest)
            << attrTagT(_GenerateMapFile, tool.GenerateMapFile)
            << attrTagL(_HeapCommitSize, tool.HeapCommitSize, /*ifNot*/ -1)
            << attrTagL(_HeapReserveSize, tool.HeapReserveSize, /*ifNot*/ -1)
            << attrTagT(_IgnoreAllDefaultLibraries, tool.IgnoreAllDefaultLibraries)
            << attrTagT(_IgnoreEmbeddedIDL, tool.IgnoreEmbeddedIDL)
            << attrTagT(_IgnoreImportLibrary, tool.IgnoreImportLibrary)
            << attrTagX(_IgnoreSpecificDefaultLibraries, tool.IgnoreDefaultLibraryNames, ";")
            << attrTagS(_ImportLibrary, tool.ImportLibrary)
            << attrTagS(_KeyContainer, tool.KeyContainer)
            << attrTagS(_KeyFile, tool.KeyFile)
            << attrTagT(_LargeAddressAware, toTriState(tool.LargeAddressAware))
            << attrTagT(_LinkDLL, (tool.config->ConfigurationType == typeDynamicLibrary ? _True : unset))
            << attrTagS(_LinkErrorReporting, tool.LinkErrorReporting)
            << attrTagT(_LinkIncremental, toTriState(tool.LinkIncremental))
            << attrTagT(_LinkStatus, toTriState(tool.ShowProgress))
            << attrTagS(_LinkTimeCodeGeneration, toString(tool.LinkTimeCodeGeneration))
            << attrTagS(_ManifestFile, tool.ManifestFile)
            << attrTagT(_MapExports, tool.MapExports)
            << attrTagS(_MapFileName, tool.MapFileName)
            << attrTagS(_MergedIDLBaseFileName, tool.MergedIDLBaseFileName)
            << attrTagS(_MergeSections, tool.MergeSections)
            << attrTagS(_MidlCommandFile, tool.MidlCommandFile)
            << attrTagS(_ModuleDefinitionFile, tool.ModuleDefinitionFile)
            << attrTagT(_NoEntryPoint, tool.ResourceOnlyDLL)
            << attrTagT(_OptimizeReferences, toTriState(tool.OptimizeReferences))
            << attrTagS(_OutputFile, tool.OutputFile)
            << attrTagT(_PreventDllBinding, tool.PreventDllBinding)
            << attrTagS(_ProgramDatabaseFile, tool.ProgramDatabaseFile)
            << attrTagT(_RandomizedBaseAddress, tool.RandomizedBaseAddress)
            << attrTagT(_RegisterOutput, tool.RegisterOutput)
            << attrTagL(_SectionAlignment, tool.SectionAlignment, /*ifNot*/ -1)
            << attrTagT(_SetChecksum, tool.SetChecksum)
            << attrTagL(_StackCommitSize, tool.StackCommitSize, /*ifNot*/ -1)
            << attrTagL(_StackReserveSize, tool.StackReserveSize, /*ifNot*/ -1)
            << attrTagS(_StripPrivateSymbols, tool.StripPrivateSymbols)
            << attrTagS(_SubSystem, toString(tool.SubSystem))
//            << attrTagT(_SupportNobindOfDelayLoadedDLL, tool.SupportNobindOfDelayLoadedDLL)
            << attrTagT(_SupportUnloadOfDelayLoadedDLL, tool.SupportUnloadOfDelayLoadedDLL)
            << attrTagT(_SuppressStartupBanner, tool.SuppressStartupBanner)
            << attrTagT(_SwapRunFromCD, tool.SwapRunFromCD)
            << attrTagT(_SwapRunFromNet, tool.SwapRunFromNet)
            << attrTagS(_TargetMachine, toString(tool.TargetMachine))
            << attrTagT(_TerminalServerAware, toTriState(tool.TerminalServerAware))
            << attrTagT(_TreatLinkerWarningAsErrors, tool.TreatWarningsAsErrors)
            << attrTagT(_TurnOffAssemblyGeneration, tool.TurnOffAssemblyGeneration)
            << attrTagS(_TypeLibraryFile, tool.TypeLibraryFile)
            << attrTagL(_TypeLibraryResourceID, tool.TypeLibraryResourceID, /*ifNot*/ 0)
            << attrTagS(_UACExecutionLevel, tool.UACExecutionLevel)
            << attrTagT(_UACUIAccess, tool.UACUIAccess)
            << attrTagS(_Version, tool.Version)
        << closetag(_Link);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCMIDLTool &tool)
{
    xml
        << tag(_Midl)
            << attrTagX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories, ";")
            << attrTagX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrTagT(_ApplicationConfigurationMode, tool.ApplicationConfigurationMode)
            << attrTagS(_ClientStubFile, tool.ClientStubFile)
            << attrTagX(_CPreprocessOptions, tool.CPreprocessOptions, " ")
            << attrTagS(_DefaultCharType, toString(tool.DefaultCharType))
            << attrTagS(_DLLDataFileName, tool.DLLDataFileName)
            << attrTagS(_EnableErrorChecks, toString(tool.EnableErrorChecks))
            << attrTagT(_ErrorCheckAllocations, tool.ErrorCheckAllocations)
            << attrTagT(_ErrorCheckBounds, tool.ErrorCheckBounds)
            << attrTagT(_ErrorCheckEnumRange, tool.ErrorCheckEnumRange)
            << attrTagT(_ErrorCheckRefPointers, tool.ErrorCheckRefPointers)
            << attrTagT(_ErrorCheckStubData, tool.ErrorCheckStubData)
            << attrTagS(_GenerateClientFiles, tool.GenerateClientFiles)
            << attrTagS(_GenerateServerFiles, tool.GenerateServerFiles)
            << attrTagT(_GenerateStublessProxies, tool.GenerateStublessProxies)
            << attrTagT(_GenerateTypeLibrary, tool.GenerateTypeLibrary)
            << attrTagS(_HeaderFileName, tool.HeaderFileName)
            << attrTagT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
            << attrTagS(_InterfaceIdentifierFileName, tool.InterfaceIdentifierFileName)
            << attrTagL(_LocaleID, tool.LocaleID, /*ifNot*/ -1)
            << attrTagT(_MkTypLibCompatible, tool.MkTypLibCompatible)
            << attrTagS(_OutputDirectory, tool.OutputDirectory)
            << attrTagX(_PreprocessorDefinitions, tool.PreprocessorDefinitions, ";")
            << attrTagS(_ProxyFileName, tool.ProxyFileName)
            << attrTagS(_RedirectOutputAndErrors, tool.RedirectOutputAndErrors)
            << attrTagS(_ServerStubFile, tool.ServerStubFile)
            << attrTagS(_StructMemberAlignment, toString(tool.StructMemberAlignment))
            << attrTagT(_SuppressCompilerWarnings, tool.SuppressCompilerWarnings)
            << attrTagT(_SuppressStartupBanner, tool.SuppressStartupBanner)
            << attrTagS(_TargetEnvironment, toString(tool.TargetEnvironment))
            << attrTagS(_TypeLibFormat, tool.TypeLibFormat)
            << attrTagS(_TypeLibraryName, tool.TypeLibraryName)
            << attrTagX(_UndefinePreprocessorDefinitions, tool.UndefinePreprocessorDefinitions, ";")
            << attrTagT(_ValidateAllParameters, tool.ValidateAllParameters)
            << attrTagT(_WarnAsError, tool.WarnAsError)
            << attrTagS(_WarningLevel, toString(tool.WarningLevel))
        << closetag(_Midl);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCCustomBuildTool &tool)
{
    const QString condition = generateCondition(*tool.config);

    if ( !tool.AdditionalDependencies.isEmpty() )
    {
        xml << tag("AdditionalInputs")
            << attrTag("Condition", condition)
            << valueTagDefX(tool.AdditionalDependencies, "AdditionalInputs", ";");
    }

    if( !tool.CommandLine.isEmpty() )
    {
        xml << tag("Command")
            << attrTag("Condition", condition)
            << valueTag(tool.CommandLine.join(vcxCommandSeparator()));
    }

    if ( !tool.Description.isEmpty() )
    {
        xml << tag("Message")
            << attrTag("Condition", condition)
            << valueTag(tool.Description);
    }

    if ( !tool.Outputs.isEmpty() )
    {
        xml << tag("Outputs")
            << attrTag("Condition", condition)
            << valueTagDefX(tool.Outputs, "Outputs", ";");
    }
}

void VCXProjectWriter::write(XmlOutput &xml, const VCLibrarianTool &tool)
{
    xml
        << tag(_Link)
            << attrTagX(_AdditionalDependencies, tool.AdditionalDependencies, ";")
            << attrTagX(_AdditionalLibraryDirectories, tool.AdditionalLibraryDirectories, ";")
            << attrTagX(_AdditionalOptions, tool.AdditionalOptions, " ")
//unused    << attrTagS(_DisplayLibrary, tool.DisplayLibrary)
//unused    << attrTagS(_ErrorReporting, tool.ErrorReporting)
            << attrTagX(_ExportNamedFunctions, tool.ExportNamedFunctions, ";")
            << attrTagX(_ForceSymbolReferences, tool.ForceSymbolReferences, ";")
            << attrTagT(_IgnoreAllDefaultLibraries, tool.IgnoreAllDefaultLibraries)
            << attrTagX(_IgnoreSpecificDefaultLibraries, tool.IgnoreDefaultLibraryNames, ";")
//unused    << attrTagT(_LinkTimeCodeGeneration, tool.LinkTimeCodeGeneration)
            << attrTagS(_ModuleDefinitionFile, tool.ModuleDefinitionFile)
//unused    << attrTagS(_Name, tool.Name)
            << attrTagS(_OutputFile, tool.OutputFile)
//unused    << attrTagX(_RemoveObjects, tool.RemoveObjects, ";")
//unused    << attrTagS(_SubSystem, tool.SubSystem)
            << attrTagT(_SuppressStartupBanner, tool.SuppressStartupBanner)
//unused    << attrTagS(_TargetMachine, tool.TargetMachine)
//unused    << attrTagT(_TreatLibWarningAsErrors, tool.TreatLibWarningAsErrors)
//unused    << attrTagT(_Verbose, tool.Verbose)
        << closetag(_Link);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCResourceCompilerTool &tool)
{
    xml
        << tag(_ResourceCompile)
            << attrTagX(_AdditionalIncludeDirectories, tool.AdditionalIncludeDirectories, ";")
            << attrTagX(_AdditionalOptions, tool.AdditionalOptions, " ")
            << attrTagS(_Culture, toString(tool.Culture))
            << attrTagT(_IgnoreStandardIncludePath, tool.IgnoreStandardIncludePath)
//unused    << attrTagT(_NullTerminateStrings, tool.NullTerminateStrings)
            << attrTagX(_PreprocessorDefinitions, tool.PreprocessorDefinitions, ";")
            << attrTagS(_ResourceOutputFileName, tool.ResourceOutputFileName)
            << attrTagT(_ShowProgress, toTriState(tool.ShowProgress))
            << attrTagT(_SuppressStartupBanner, tool.SuppressStartupBanner)
//unused    << attrTagS(_TrackerLogDirectory, tool.TrackerLogDirectory)
//unused    << attrTagS(_UndefinePreprocessorDefinitions, tool.UndefinePreprocessorDefinitions)
        << closetag(_ResourceCompile);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCEventTool &tool)
{
    xml
        << tag(tool.EventName)
            << tag(_Command) << valueTag(tool.CommandLine.join(vcxCommandSeparator()))
            << tag(_Message) << valueTag(tool.Description)
        << closetag(tool.EventName);
}

void VCXProjectWriter::write(XmlOutput &xml, const VCDeploymentTool &tool)
{
    Q_UNUSED(xml);
    Q_UNUSED(tool);
    // SmartDevice deployment not supported in VS 2010
}

void VCXProjectWriter::write(XmlOutput &xml, const VCConfiguration &tool)
{
        xml << tag("PropertyGroup")
            << attrTag("Condition", generateCondition(tool))
            << attrTag("Label", "Configuration")
            << attrTagS(_OutputDirectory, tool.OutputDirectory)
            << attrTagT(_ATLMinimizesCRunTimeLibraryUsage, tool.ATLMinimizesCRunTimeLibraryUsage)
            << attrTagT(_BuildBrowserInformation, tool.BuildBrowserInformation)
            << attrTagS(_CharacterSet, toString(tool.CharacterSet))
            << attrTagS(_ConfigurationType, toString(tool.ConfigurationType))
            << attrTagS(_DeleteExtensionsOnClean, tool.DeleteExtensionsOnClean)
            << attrTagS(_ImportLibrary, tool.ImportLibrary)
            << attrTagS(_IntermediateDirectory, tool.IntermediateDirectory)
            << attrTagS(_PrimaryOutput, tool.PrimaryOutput)
            << attrTagS(_ProgramDatabase, tool.ProgramDatabase)
            << attrTagT(_RegisterOutput, tool.RegisterOutput)
            << attrTagS(_UseOfATL, toString(tool.UseOfATL))
            << attrTagS(_UseOfMfc, toString(tool.UseOfMfc))
            << attrTagT(_WholeProgramOptimization, tool.WholeProgramOptimization)
        << closetag();
}

void VCXProjectWriter::write(XmlOutput &xml, VCFilter &tool)
{
    Q_UNUSED(xml);
    Q_UNUSED(tool);
    // unused in this generator
}

void VCXProjectWriter::addFilters(VCProject &project, XmlOutput &xmlFilter, const QString &filtername)
{
    bool added = false;

    for (int i = 0; i < project.SingleProjects.count(); ++i) {
        VCFilter filter;
        const VCProjectSingleConfig &singleCfg = project.SingleProjects.at(i);
        if (filtername == "Root Files") {
            filter = singleCfg.RootFiles;
        } else if (filtername == "Source Files") {
            filter = singleCfg.SourceFiles;
        } else if (filtername == "Header Files") {
            filter = singleCfg.HeaderFiles;
        } else if (filtername == "Generated Files") {
            filter = singleCfg.GeneratedFiles;
        } else if (filtername == "LexYacc Files") {
            filter = singleCfg.LexYaccFiles;
        } else if (filtername == "Translation Files") {
            filter = singleCfg.TranslationFiles;
        } else if (filtername == "Form Files") {
            filter = singleCfg.FormFiles;
        } else if (filtername == "Resource Files") {
            filter = singleCfg.ResourceFiles;
        } else {
            // ExtraCompilers
            filter = project.SingleProjects[i].filterForExtraCompiler(filtername);
        }

        if(!filter.Files.isEmpty() && !added) {
            xmlFilter << tag("Filter")
                      << attrTag("Include", filtername)
                      << attrTagS("UniqueIdentifier", filter.Guid)
                      << attrTagS("Extensions", filter.Filter)
                      << attrTagT("ParseFiles", filter.ParseFiles)
                      << closetag();
        }
    }
}

// outputs a given filter for all existing configurations of a project
void VCXProjectWriter::outputFilter(VCProject &project, XmlOutput &xml, XmlOutput &xmlFilter, const QString &filtername)
{
    XNode *root;
    if (project.SingleProjects.at(0).flat_files)
        root = new XFlatNode;
    else
        root = new XTreeNode;

    QString name, extfilter;
    triState parse;

    for (int i = 0; i < project.SingleProjects.count(); ++i) {
        VCFilter filter;
        const VCProjectSingleConfig &singleCfg = project.SingleProjects.at(i);
        if (filtername == "Root Files") {
            filter = singleCfg.RootFiles;
        } else if (filtername == "Source Files") {
            filter = singleCfg.SourceFiles;
        } else if (filtername == "Header Files") {
            filter = singleCfg.HeaderFiles;
        } else if (filtername == "Generated Files") {
            filter = singleCfg.GeneratedFiles;
        } else if (filtername == "LexYacc Files") {
            filter = singleCfg.LexYaccFiles;
        } else if (filtername == "Translation Files") {
            filter = singleCfg.TranslationFiles;
        } else if (filtername == "Form Files") {
            filter = singleCfg.FormFiles;
        } else if (filtername == "Resource Files") {
            filter = singleCfg.ResourceFiles;
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
        }
    }

    if (!root->hasElements())
        return;

    root->generateXML(xml, xmlFilter, "", project, filtername); // output root tree
}

// Output all configurations (by filtername) for a file (by info)
// A filters config output is in VCFilter.outputFileConfig()
void VCXProjectWriter::outputFileConfigs(VCProject &project, XmlOutput &xml, XmlOutput &xmlFilter, const VCFilterFile &info, const QString &filtername)
{
    // We need to check if the file has any custom build step.
    // If there is one then it has to be included with "CustomBuild Include"
    bool fileAdded = false;

    for (int i = 0; i < project.SingleProjects.count(); ++i) {
        VCFilter filter;
        const VCProjectSingleConfig &singleCfg = project.SingleProjects.at(i);
        if (filtername.startsWith("Root Files")) {
            filter = singleCfg.RootFiles;
        } else if (filtername.startsWith("Source Files")) {
            filter = singleCfg.SourceFiles;
        } else if (filtername.startsWith("Header Files")) {
            filter = singleCfg.HeaderFiles;
        } else if (filtername.startsWith("Generated Files")) {
            filter = singleCfg.GeneratedFiles;
        } else if (filtername.startsWith("LexYacc Files")) {
            filter = singleCfg.LexYaccFiles;
        } else if (filtername.startsWith("Translation Files")) {
            filter = singleCfg.TranslationFiles;
        } else if (filtername.startsWith("Form Files")) {
            filter = singleCfg.FormFiles;
        } else if (filtername.startsWith("Resource Files")) {
            filter = singleCfg.ResourceFiles;
        } else {
            // ExtraCompilers
            filter = project.SingleProjects[i].filterForExtraCompiler(filtername);
        }

        if (filter.Config) // only if the filter is not empty
            if (outputFileConfig(filter, xml, xmlFilter, info.file, filtername, fileAdded)) // only add it once.
                fileAdded = true;
    }

    if ( !fileAdded )
    {
        if (filtername.startsWith("Source Files")) {

            xmlFilter << tag("ClCompile")
                      << attrTag("Include",Option::fixPathToLocalOS(info.file))
                      << attrTagS("Filter", filtername);

            xml << tag("ClCompile")
                << attrTag("Include",Option::fixPathToLocalOS(info.file));

        } else if(filtername.startsWith("Header Files")) {

            xmlFilter << tag("ClInclude")
                << attrTag("Include",Option::fixPathToLocalOS(info.file))
                << attrTagS("Filter", filtername);

            xml << tag("ClInclude")
                << attrTag("Include",Option::fixPathToLocalOS(info.file));
        } else if(filtername.startsWith("Generated Files") || filtername.startsWith("Form Files")) {

            if (info.file.endsWith(".h")) {

                xmlFilter << tag("ClInclude")
                          << attrTag("Include",Option::fixPathToLocalOS(info.file))
                          << attrTagS("Filter", filtername);

                xml << tag("ClInclude")
                    << attrTag("Include",Option::fixPathToLocalOS(info.file));
            } else if(info.file.endsWith(".cpp")) {

                xmlFilter << tag("ClCompile")
                          << attrTag("Include",Option::fixPathToLocalOS(info.file))
                          << attrTagS("Filter", filtername);

                xml << tag("ClCompile")
                    << attrTag("Include",Option::fixPathToLocalOS(info.file));
            } else if(info.file.endsWith(".res")) {

                    xmlFilter << tag("CustomBuild")
                              << attrTag("Include",Option::fixPathToLocalOS(info.file))
                              << attrTagS("Filter", filtername);

                    xml << tag("CustomBuild")
                        << attrTag("Include",Option::fixPathToLocalOS(info.file));
            } else {

                xmlFilter << tag("CustomBuild")
                          << attrTag("Include",Option::fixPathToLocalOS(info.file))
                          << attrTagS("Filter", filtername);

                xml << tag("CustomBuild")
                    << attrTag("Include",Option::fixPathToLocalOS(info.file));
            }

        } else if(filtername.startsWith("Root Files")) {

            if (info.file.endsWith(".rc")) {

                xmlFilter << tag("ResourceCompile")
                          << attrTag("Include",Option::fixPathToLocalOS(info.file));

                xml << tag("ResourceCompile")
                    << attrTag("Include",Option::fixPathToLocalOS(info.file));
            }
        } else {

            xmlFilter << tag("None")
                      << attrTag("Include",Option::fixPathToLocalOS(info.file))
                      << attrTagS("Filter", filtername);

                xml << tag("None")
                    << attrTag("Include",Option::fixPathToLocalOS(info.file));
        }
    }

    xml << closetag();
    xmlFilter << closetag();
}

bool VCXProjectWriter::outputFileConfig(VCFilter &filter, XmlOutput &xml, XmlOutput &xmlFilter, const QString &filename, const QString &filtername, bool fileAllreadyAdded)
{
    bool fileAdded = false;

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
    filter.CompilerTool.ProgramDataBaseFileName.clear();
    filter.CompilerTool.RuntimeLibrary = rtUnknown;
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
        if (filter.Project->usePCH)
            filter.modifyPCHstage(info.file);
    } else {
        // Excluded files uses an empty compiler stage
        if(info.excludeFromBuild)
            filter.useCompilerTool = true;
    }

    // Actual XML output ----------------------------------
    if (filter.useCustomBuildTool || filter.useCompilerTool || !inBuild) {

        if (filter.useCustomBuildTool)
        {
            if ( !fileAllreadyAdded ) {

                fileAdded = true;

                xmlFilter << tag("CustomBuild")
                    << attrTag("Include",Option::fixPathToLocalOS(filename))
                    << attrTagS("Filter", filtername);

                xml << tag("CustomBuild")
                    << attrTag("Include",Option::fixPathToLocalOS(filename));

                if ( filtername.startsWith("Form Files") || filtername.startsWith("Generated Files") || filtername.startsWith("Resource Files") )
                    xml << attrTagS("FileType", "Document");
            }

            filter.Project->projectWriter->write(xml, filter.CustomBuildTool);
        }

        if ( !fileAdded && !fileAllreadyAdded )
        {
            fileAdded = true;

            if (filtername.startsWith("Source Files")) {

                xmlFilter << tag("ClCompile")
                          << attrTag("Include",Option::fixPathToLocalOS(filename))
                          << attrTagS("Filter", filtername);

                xml << tag("ClCompile")
                    << attrTag("Include",Option::fixPathToLocalOS(filename));

            } else if(filtername.startsWith("Header Files")) {

                xmlFilter << tag("ClInclude")
                    << attrTag("Include",Option::fixPathToLocalOS(filename))
                    << attrTagS("Filter", filtername);

                xml << tag("ClInclude")
                    << attrTag("Include",Option::fixPathToLocalOS(filename));
            } else if(filtername.startsWith("Generated Files") || filtername.startsWith("Form Files")) {

                if (filename.endsWith(".h")) {

                    xmlFilter << tag("ClInclude")
                              << attrTag("Include",Option::fixPathToLocalOS(filename))
                              << attrTagS("Filter", filtername);

                    xml << tag("ClInclude")
                        << attrTag("Include",Option::fixPathToLocalOS(filename));
                } else if(filename.endsWith(".cpp")) {

                    xmlFilter << tag("ClCompile")
                              << attrTag("Include",Option::fixPathToLocalOS(filename))
                              << attrTagS("Filter", filtername);

                    xml << tag("ClCompile")
                        << attrTag("Include",Option::fixPathToLocalOS(filename));
                } else if(filename.endsWith(".res")) {

                    xmlFilter << tag("CustomBuild")
                              << attrTag("Include",Option::fixPathToLocalOS(filename))
                              << attrTagS("Filter", filtername);

                    xml << tag("CustomBuild")
                        << attrTag("Include",Option::fixPathToLocalOS(filename));
                } else {

                    xmlFilter << tag("CustomBuild")
                              << attrTag("Include",Option::fixPathToLocalOS(filename))
                              << attrTagS("Filter", filtername);

                    xml << tag("CustomBuild")
                        << attrTag("Include",Option::fixPathToLocalOS(filename));
                }
            } else if(filtername.startsWith("Root Files")) {

                if (filename.endsWith(".rc")) {

                    xmlFilter << tag("ResourceCompile")
                              << attrTag("Include",Option::fixPathToLocalOS(filename));

                    xml << tag("ResourceCompile")
                        << attrTag("Include",Option::fixPathToLocalOS(filename));
                }
            }
        }

        const QString condition = generateCondition(*filter.Config);
        if(!inBuild) {
            xml << tag("ExcludedFromBuild")
                << attrTag("Condition", condition)
                << valueTag("true");
        }

        if (filter.useCompilerTool) {

            if ( !filter.CompilerTool.ForcedIncludeFiles.isEmpty() ) {
                xml << tag("ForcedIncludeFiles")
                    << attrTag("Condition", condition)
                    << valueTagX(filter.CompilerTool.ForcedIncludeFiles);
            }

            if ( !filter.CompilerTool.PrecompiledHeaderThrough.isEmpty() ) {

                xml << tag("PrecompiledHeaderFile")
                    << attrTag("Condition", condition)
                    << valueTag(filter.CompilerTool.PrecompiledHeaderThrough)
                    << tag("PrecompiledHeader")
                    << attrTag("Condition", condition)
                    << valueTag(toString(filter.CompilerTool.UsePrecompiledHeader));
            }
        }
    }

    return fileAdded;
}

QString VCXProjectWriter::generateCondition(const VCConfiguration &config)
{
    return QLatin1String("'$(Configuration)|$(Platform)'=='") + config.Name + QLatin1Char('\'');
}

QT_END_NAMESPACE
