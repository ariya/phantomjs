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

#ifndef MSVC_OBJECTMODEL_H
#define MSVC_OBJECTMODEL_H

#include "project.h"
#include "xmloutput.h"
#include <qatomic.h>
#include <qlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

enum DotNET {
    NETUnknown = 0,
    NET2002 = 0x70,
    NET2003 = 0x71,
    NET2005 = 0x80,
    NET2008 = 0x90,
    NET2010 = 0xa0
};

/*
    This Object model is of course VERY simplyfied,
    and does not actually follow the original MSVC
    object model. However, it fulfilles the basic
    needs for qmake
*/

/*
    If a triState value is 'unset' then the
    corresponding property is not in the output,
    forcing the tool to utilize default values.
    False/True values will be in the output...
*/
enum customBuildCheck {
    none,
    mocSrc,
    mocHdr,
    lexyacc
};
enum triState {
    unset = -1,
    _False = 0,
    _True = 1
};

triState operator!(const triState &rhs);

enum addressAwarenessType {
    addrAwareDefault,
    addrAwareNoLarge,
    addrAwareLarge
};
enum asmListingOption {
    asmListingNone,
    asmListingAssemblyOnly,
    asmListingAsmMachineSrc,
    asmListingAsmMachine,
    asmListingAsmSrc
};
enum basicRuntimeCheckOption {
    runtimeBasicCheckNone,
    runtimeCheckStackFrame,
    runtimeCheckUninitVariables,
    runtimeBasicCheckAll
};
enum browseInfoOption {
    brInfoNone,
    brAllInfo,
    brNoLocalSymbols
};
enum callingConventionOption {
    callConventionDefault = -1,
    callConventionCDecl,
    callConventionFastCall,
    callConventionStdCall
};
enum charSet {
    charSetNotSet,
    charSetUnicode,
    charSetMBCS
};
enum compileAsManagedOptions {
    managedDefault           = -1, // Was: noAssembly
    managedAssembly          = 1,
    managedAssemblyPure      = 2,  // Old was: Assembly
    managedAssemblySafe      = 3,
    managedAssemblyOldSyntax = 4
};
enum CompileAsOptions{
    compileAsDefault,
    compileAsC,
    compileAsCPlusPlus
};
enum ConfigurationTypes {
    typeUnknown        = 0,
    typeApplication    = 1,
    typeDynamicLibrary = 2,
    typeStaticLibrary  = 4,
    typeGeneric        = 10
};
enum debugOption {
    debugUnknown = -1,
    debugDisabled,
    debugOldStyleInfo,
    debugLineInfoOnly,
    debugEnabled,
    debugEditAndContinue
};
enum eAppProtectionOption {
    eAppProtectUnchanged,
    eAppProtectLow,
    eAppProtectMedium,
    eAppProtectHigh
};
enum enhancedInstructionSetOption {
    archNotSet = 0,
    archSSE = 1,
    archSSE2 = 2
};
enum exceptionHandling {
    ehDefault = -1,
    ehNone    = 0,
    ehNoSEH   = 1,
    ehSEH     = 2
};
enum enumResourceLangID {
    rcUseDefault                 = 0,
    rcAfrikaans                  = 1078,
    rcAlbanian                   = 1052,
    rcArabicAlgeria              = 5121,
    rcArabicBahrain              = 15361,
    rcArabicEgypt                = 3073,
    rcArabicIraq                 = 2049,
    rcArabicJordan               = 11265,
    rcArabicKuwait               = 13313,
    rcArabicLebanon              = 12289,
    rcArabicLibya                = 4097,
    rcArabicMorocco              = 6145,
    rcArabicOman                 = 8193,
    rcArabicQatar                = 16385,
    rcArabicSaudi                = 1025,
    rcArabicSyria                = 10241,
    rcArabicTunisia              = 7169,
    rcArabicUnitedArabEmirates   = 14337,
    rcArabicYemen                = 9217,
    rcBasque                     = 1069,
    rcBulgarian                  = 1026,
    rcByelorussian               = 1059,
    rcCatalan                    = 1027,
    rcChineseHongKong            = 3076,
    rcChinesePRC                 = 2052,
    rcChineseSingapore           = 4100,
    rcChineseTaiwan              = 1028,
    rcCroatian                   = 1050,
    rcCzech                      = 1029,
    rcDanish                     = 1030,
    rcDutchBelgium               = 2067,
    rcDutchStandard              = 1043,
    rcEnglishAustralia           = 3081,
    rcEnglishBritain             = 2057,
    rcEnglishCanada              = 4105,
    RcEnglishCaribbean           = 9225,
    rcEnglishIreland             = 6153,
    rcEnglishJamaica             = 8201,
    rcEnglishNewZealand          = 5129,
    rcEnglishSouthAfrica         = 7177,
    rcEnglishUS                  = 1033,
    rcEstonian                   = 1061,
    rcFarsi                      = 1065,
    rcFinnish                    = 1035,
    rcFrenchBelgium              = 2060,
    rcFrenchCanada               = 3084,
    rcFrenchLuxembourg           = 5132,
    rcFrenchStandard             = 1036,
    rcFrenchSwitzerland          = 4108,
    rcGermanAustria              = 3079,
    rcGermanLichtenstein         = 5127,
    rcGermanLuxembourg           = 4103,
    rcGermanStandard             = 1031,
    rcGermanSwitzerland          = 2055,
    rcGreek                      = 1032,
    rcHebrew                     = 1037,
    rcHungarian                  = 1038,
    rcIcelandic                  = 1039,
    rcIndonesian                 = 1057,
    rcItalianStandard            = 1040,
    rcItalianSwitzerland         = 2064,
    rcJapanese                   = 1041,
    rcKorean                     = 1042,
    rcKoreanJohab                = 2066,
    rcLatvian                    = 1062,
    rcLithuanian                 = 1063,
    rcNorwegianBokmal            = 1044,
    rcNorwegianNynorsk           = 2068,
    rcPolish                     = 1045,
    rcPortugueseBrazilian        = 1046,
    rcPortugueseStandard         = 2070,
    rcRomanian                   = 1048,
    rcRussian                    = 1049,
    rcSerbian                    = 2074,
    rcSlovak                     = 1051,
    rcSpanishArgentina           = 11274,
    rcSpanishBolivia             = 16394,
    rcSpanishChile               = 13322,
    rcSpanishColombia            = 9226,
    rcSpanishCostaRica           = 5130,
    rcSpanishDominicanRepublic   = 7178,
    rcSpanishEcuador             = 12298,
    rcSpanishGuatemala           = 4106,
    rcSpanishMexico              = 2058,
    rcSpanishModern              = 3082,
    rcSpanishPanama              = 6154,
    rcSpanishParaguay            = 15370,
    rcSpanishPeru                = 10250,
    rcSpanishTraditional         = 1034,
    rcSpanishUruguay             = 14346,
    rcSpanishVenezuela           = 8202,
    rcSwedish                    = 1053,
    rcThai                       = 1054,
    rcTurkish                    = 1055,
    rcUkrainian                  = 1058,
    rcUrdu                       = 1056
};
enum enumSccEvent {
    eProjectInScc,
    ePreDirtyNotification
};
enum favorSizeOrSpeedOption {
    favorNone,
    favorSpeed,
    favorSize
};
enum floatingPointModel {
    floatingPointNotSet = -1,
    floatingPointPrecise,
    floatingPointStrict,
    floatingPointFast
};
enum genProxyLanguage {
    genProxyNative,
    genProxyManaged
};
enum inlineExpansionOption {
    expandDisable,
    expandOnlyInline,
    expandAnySuitable,
    expandDefault // Not useful number, but stops the output
};
enum linkIncrementalType {
    linkIncrementalDefault,
    linkIncrementalNo,
    linkIncrementalYes
};
enum linkProgressOption {
    linkProgressNotSet,
    linkProgressAll,
    linkProgressLibs
};
enum machineTypeOption {
    machineNotSet,
    machineX86,
    machineX64 = 17
};
enum midlCharOption {
    midlCharUnsigned,
    midlCharSigned,
    midlCharAscii7
};
enum midlErrorCheckOption {
    midlEnableCustom,
    midlDisableAll,
    midlEnableAll
};
enum midlStructMemberAlignOption {
    midlAlignNotSet,
    midlAlignSingleByte,
    midlAlignTwoBytes,
    midlAlignFourBytes,
    midlAlignEightBytes,
    midlAlignSixteenBytes
};
enum midlTargetEnvironment {
    midlTargetNotSet,
    midlTargetWin32,
    midlTargetWin64
};
enum midlWarningLevelOption {
    midlWarningLevel_0,
    midlWarningLevel_1,
    midlWarningLevel_2,
    midlWarningLevel_3,
    midlWarningLevel_4
};
enum optFoldingType {
    optFoldingDefault,
    optNoFolding,
    optFolding
};
enum optimizeOption {
    optimizeDisabled,
    optimizeMinSpace,
    optimizeMaxSpeed,
    optimizeFull,
    optimizeCustom,
    optimizeDefault // Not useful number, but stops the output
};
enum optRefType {
    optReferencesDefault,
    optNoReferences,
    optReferences
};
enum optWin98Type {
    optWin98Default,
    optWin98No,
    optWin98Yes
};
enum optLinkTimeCodeGenType {
    optLTCGDefault,
    optLTCGEnabled,
    optLTCGInstrument,
    optLTCGOptimize,
    optLTCGUpdate
};
enum pchOption {
    pchUnset = -1,
    pchNone,
    pchCreateUsingSpecific,
    pchGenerateAuto,
    pchUseUsingSpecific
};
enum preprocessOption {
    preprocessUnknown = -1,
    preprocessNo,
    preprocessYes,
    preprocessNoLineNumbers
};
enum ProcessorOptimizeOption {
    procOptimizeBlended,                //GB
    procOptimizePentium,                //G5
    procOptimizePentiumProAndAbove,     //G6
    procOptimizePentium4AndAbove        //G7
};
enum RegisterDeployOption {
    registerNo = 0,
    registerYes
};
enum RemoteDebuggerType {
    DbgLocal,
    DbgRemote,
    DbgRemoteTCPIP
};
enum runtimeLibraryOption {
    rtUnknown = -1,
    rtMultiThreaded,
    rtMultiThreadedDebug,
    rtMultiThreadedDLL,
    rtMultiThreadedDebugDLL,
    rtSingleThreaded,
    rtSingleThreadedDebug
};
enum structMemberAlignOption {
    alignNotSet,
    alignSingleByte,
    alignTwoBytes,
    alignFourBytes,
    alignEightBytes,
    alignSixteenBytes
};
enum subSystemOption {
    subSystemNotSet,
    subSystemConsole,
    subSystemWindows
};
enum termSvrAwarenessType {
    termSvrAwareDefault,
    termSvrAwareNo,
    termSvrAwareYes
};
enum toolSetType {
    toolSetUtility,
    toolSetMakefile,
    toolSetLinker,
    toolSetLibrarian,
    toolSetAll
};
enum TypeOfDebugger {
    DbgNativeOnly,
    DbgManagedOnly,
    DbgMixed,
    DbgAuto
};
enum useOfATL {
    useATLNotSet,
    useATLStatic,
    useATLDynamic
};
enum useOfMfc {
    useMfcStdWin,
    useMfcStatic,
    useMfcDynamic
};
enum useOfArchitecture {
    archUnknown = -1,
    archArmv4,
    archArmv5,
    archArmv4T,
    archArmv5T,
    archMips1 = 0,
    archMips2 = 1,
    archMips3 = 2,
    archMips4 = 3,
    archMips5 = 4,
    archMips16 = 5,
    archMips32 = 6,
    archMips64 = 7
};
enum warningLevelOption {
    warningLevelUnknown = -1,
    warningLevel_0,
    warningLevel_1,
    warningLevel_2,
    warningLevel_3,
    warningLevel_4
};


class VCToolBase {
protected:
    // Functions
    VCToolBase(){}
    virtual ~VCToolBase(){}
    virtual bool parseOption(const char* option) = 0;
public:
    void parseOptions(QStringList& options) {
        for (QStringList::ConstIterator it=options.begin(); (it!=options.end()); it++)
            parseOption((*it).toLatin1());
    }
    static QStringList fixCommandLine(const QString &input);
};

class VCConfiguration;
class VCProject;

class VCCLCompilerTool : public VCToolBase
{
public:
    // Functions
    VCCLCompilerTool();
    virtual ~VCCLCompilerTool(){}
    bool parseOption(const char* option);

    // Variables
    QStringList             AdditionalIncludeDirectories;
    QStringList             AdditionalOptions;
    QStringList             AdditionalUsingDirectories;
    QString                 AssemblerListingLocation;
    asmListingOption        AssemblerOutput;
    basicRuntimeCheckOption BasicRuntimeChecks;
    browseInfoOption        BrowseInformation;
    QString                 BrowseInformationFile;
    triState                BufferSecurityCheck;
    callingConventionOption CallingConvention;
    CompileAsOptions        CompileAs;
    compileAsManagedOptions CompileAsManaged;
    triState                CompileOnly;
    debugOption             DebugInformationFormat;
    triState                DefaultCharIsUnsigned;
    triState                Detect64BitPortabilityProblems;
    triState                DisableLanguageExtensions;
    QStringList             DisableSpecificWarnings;
    enhancedInstructionSetOption  EnableEnhancedInstructionSet;
    triState                EnableFiberSafeOptimizations;
    triState                EnableFunctionLevelLinking;
    triState                EnableIntrinsicFunctions;
    exceptionHandling       ExceptionHandling;
    triState                ExpandAttributedSource;
    favorSizeOrSpeedOption  FavorSizeOrSpeed;
    floatingPointModel      FloatingPointModel;
    triState                FloatingPointExceptions;
    triState                ForceConformanceInForLoopScope;
    QStringList             ForcedIncludeFiles;
    QStringList             ForcedUsingFiles;
    preprocessOption        GeneratePreprocessedFile;
    triState                PreprocessSuppressLineNumbers;
    triState                GlobalOptimizations;
    triState                IgnoreStandardIncludePath;
    triState                ImproveFloatingPointConsistency;
    inlineExpansionOption   InlineFunctionExpansion;
    triState                KeepComments;
    triState                MinimalRebuild;
    QString                 ObjectFile;
    triState                OmitDefaultLibName;
    triState                OmitFramePointers;
    triState                OpenMP;
    optimizeOption          Optimization;
    ProcessorOptimizeOption OptimizeForProcessor;
    triState                OptimizeForWindowsApplication;
    QString                 OutputFile;
    QString                 PrecompiledHeaderFile;
    QString                 PrecompiledHeaderThrough;
    QStringList             PreprocessorDefinitions;
    QString                 ProgramDataBaseFileName;
    runtimeLibraryOption    RuntimeLibrary;
    triState                RuntimeTypeInfo;
    triState                ShowIncludes;
    triState                SmallerTypeCheck;
    triState                StringPooling;
    structMemberAlignOption StructMemberAlignment;
    triState                SuppressStartupBanner;
    triState                TreatWChar_tAsBuiltInType;
    triState                TurnOffAssemblyGeneration;
    triState                UndefineAllPreprocessorDefinitions;
    QStringList             UndefinePreprocessorDefinitions;
    pchOption               UsePrecompiledHeader;
    triState                UseUnicodeForAssemblerListing;
    triState                WarnAsError;
    warningLevelOption      WarningLevel;
    triState                WholeProgramOptimization;
    useOfArchitecture       CompileForArchitecture;
    triState                InterworkCalls;

    // VS2010
    triState                EnablePREfast;
    triState                DisplayFullPaths;
    triState                MultiProcessorCompilation;
    QString                 MultiProcessorCompilationProcessorCount;
    triState                GenerateXMLDocumentationFiles;
    QString                 XMLDocumentationFileName;
    QString                 ErrorReporting;
    triState                CreateHotpatchableImage;
    QString                 PreprocessOutputPath;

    VCConfiguration*        config;
};

class VCLinkerTool : public VCToolBase
{
public:
    // Functions
    VCLinkerTool();
    virtual ~VCLinkerTool(){}
    bool parseOption(const char* option);

    // Variables
    QStringList             AdditionalDependencies;
    QStringList             AdditionalLibraryDirectories;
    QStringList             AdditionalOptions;
    QStringList             AddModuleNamesToAssembly;
    QString                 BaseAddress;
    triState                DataExecutionPrevention;
    QStringList             DelayLoadDLLs;
    optFoldingType          EnableCOMDATFolding;
    QString                 EntryPointSymbol;
    QStringList             ForceSymbolReferences;
    QString                 FunctionOrder;
    triState                GenerateDebugInformation;
    triState                GenerateMapFile;
    qlonglong               HeapCommitSize;
    qlonglong               HeapReserveSize;
    triState                IgnoreAllDefaultLibraries;
    QStringList             IgnoreDefaultLibraryNames;
    triState                IgnoreEmbeddedIDL;
    triState                IgnoreImportLibrary;
    QString                 ImportLibrary;
    addressAwarenessType    LargeAddressAware;
    triState                LinkDLL;
    linkIncrementalType     LinkIncremental;
    optLinkTimeCodeGenType  LinkTimeCodeGeneration;
    QString                 LinkToManagedResourceFile;
    triState                MapExports;
    QString                 MapFileName;
    triState                MapLines;
    QString                 MergedIDLBaseFileName;
    QString                 MergeSections;          // Should be list?
    QString                 MidlCommandFile;
    QString                 ModuleDefinitionFile;   // Should be list?
    optWin98Type            OptimizeForWindows98;
    optRefType              OptimizeReferences;
    QString                 OutputFile;
    QString                 ProgramDatabaseFile;
    triState                RandomizedBaseAddress;
    triState                RegisterOutput;
    triState                ResourceOnlyDLL;
    triState                SetChecksum;
    linkProgressOption      ShowProgress;
    qlonglong               StackCommitSize;
    qlonglong               StackReserveSize;
    QString                 StripPrivateSymbols;    // Should be list?
    subSystemOption         SubSystem;
    triState                SupportUnloadOfDelayLoadedDLL;
    triState                SuppressStartupBanner;
    triState                SwapRunFromCD;
    triState                SwapRunFromNet;
    machineTypeOption       TargetMachine;
    termSvrAwarenessType    TerminalServerAware;
    triState                TreatWarningsAsErrors;
    triState                TurnOffAssemblyGeneration;
    QString                 TypeLibraryFile;
    qlonglong               TypeLibraryResourceID;
    QString                 Version;

    // VS2010
    triState                GenerateManifest;
    QStringList             AdditionalManifestDependencies;
    QString                 ManifestFile;
    triState                EnableUAC;
    QString                 UACExecutionLevel;
    triState                UACUIAccess;
    qlonglong               SectionAlignment;
    triState                PreventDllBinding;
    triState                AllowIsolation;
    triState                AssemblyDebug;
    QStringList             AssemblyLinkResource;
    QString                 CLRImageType;
    QString                 CLRSupportLastError;
    QString                 CLRThreadAttribute;
    triState                CLRUnmanagedCodeCheck;
    triState                DelaySign;
    QString                 KeyContainer;
    QString                 KeyFile;
    QString                 LinkErrorReporting;

    VCConfiguration*        config;
};

class VCMIDLTool : public VCToolBase
{
public:
    // Functions
    VCMIDLTool();
    virtual ~VCMIDLTool(){}
    bool parseOption(const char* option);

    // Variables
    QStringList             AdditionalIncludeDirectories;
    QStringList             AdditionalOptions;
    QStringList             CPreprocessOptions;
    midlCharOption          DefaultCharType;
    QString                 DLLDataFileName;    // Should be list?
    midlErrorCheckOption    EnableErrorChecks;
    triState                ErrorCheckAllocations;
    triState                ErrorCheckBounds;
    triState                ErrorCheckEnumRange;
    triState                ErrorCheckRefPointers;
    triState                ErrorCheckStubData;
    QStringList             FullIncludePath;
    triState                GenerateStublessProxies;
    triState                GenerateTypeLibrary;
    QString                 HeaderFileName;
    triState                IgnoreStandardIncludePath;
    QString                 InterfaceIdentifierFileName;
    triState                MkTypLibCompatible;
    QString                 OutputDirectory;
    QStringList             PreprocessorDefinitions;
    QString                 ProxyFileName;
    QString                 RedirectOutputAndErrors;
    midlStructMemberAlignOption StructMemberAlignment;
    triState                SuppressStartupBanner;
    midlTargetEnvironment   TargetEnvironment;
    QString                 TypeLibraryName;
    QStringList             UndefinePreprocessorDefinitions;
    triState                ValidateParameters;
    triState                WarnAsError;
    midlWarningLevelOption  WarningLevel;

    // VS 2010
    triState                ApplicationConfigurationMode;
    QString                 GenerateClientFiles;
    QString                 ClientStubFile;
    QString                 TypeLibFormat;
    triState                ValidateAllParameters;
    triState                SuppressCompilerWarnings;
    QString                 GenerateServerFiles;
    QString                 ServerStubFile;
    qlonglong               LocaleID;

    VCConfiguration*        config;
};

class VCLibrarianTool : public VCToolBase
{
public:
    // Functions
    VCLibrarianTool();
    virtual ~VCLibrarianTool(){}
    bool parseOption(const char*){ return false; }

    // Variables
    QStringList             AdditionalDependencies;
    QStringList             AdditionalLibraryDirectories;
    QStringList             AdditionalOptions;
    QStringList             ExportNamedFunctions;
    QStringList             ForceSymbolReferences;
    triState                IgnoreAllDefaultLibraries;
    QStringList             IgnoreDefaultLibraryNames;
    QString                 ModuleDefinitionFile;
    QString                 OutputFile;
    triState                SuppressStartupBanner;
};

class VCCustomBuildTool : public VCToolBase
{
public:
    // Functions
    VCCustomBuildTool();
    virtual ~VCCustomBuildTool(){}
    bool parseOption(const char*){ return false; }

    // Variables
    QStringList             AdditionalDependencies;
    QStringList             CommandLine;
    QString                 Description;
    QStringList             Outputs;
    QString                 ToolName;
    QString                 ToolPath;

    VCConfiguration*        config;
};

class VCResourceCompilerTool : public VCToolBase
{
public:
    // Functions
    VCResourceCompilerTool();
    virtual ~VCResourceCompilerTool(){}
    bool parseOption(const char*){ return false; }

    // Variables
    QStringList             AdditionalIncludeDirectories;
    QStringList             AdditionalOptions;
    enumResourceLangID      Culture;
    QStringList             FullIncludePath;
    triState                IgnoreStandardIncludePath;
    QStringList             PreprocessorDefinitions;
    QString                 ResourceOutputFileName;
    linkProgressOption      ShowProgress;
    QString                 ToolPath;
    triState                SuppressStartupBanner;
};

class VCDeploymentTool
{
public:
    // Functions
    VCDeploymentTool();
    virtual ~VCDeploymentTool() {}

    // Variables
    QString                 DeploymentTag;
    QString                 RemoteDirectory;
    RegisterDeployOption    RegisterOutput;
    QString                 AdditionalFiles;
};

class VCEventTool : public VCToolBase
{
protected:
    // Functions
    VCEventTool(const QString &eventName);
    virtual ~VCEventTool(){}
    bool parseOption(const char*){ return false; }

public:
    // Variables
    QStringList             CommandLine;
    QString                 Description;
    triState                ExcludedFromBuild;
    QString                 EventName;
    QString                 ToolName;
    QString                 ToolPath;
};

class VCPostBuildEventTool : public VCEventTool
{
public:
    VCPostBuildEventTool();
    ~VCPostBuildEventTool(){}
};

class VCPreBuildEventTool : public VCEventTool
{
public:
    VCPreBuildEventTool();
    ~VCPreBuildEventTool(){}
};

class VCPreLinkEventTool : public VCEventTool
{
public:
    VCPreLinkEventTool();
    ~VCPreLinkEventTool(){}
};

class VCConfiguration
{
public:
    // Functions
    VCConfiguration();
    ~VCConfiguration(){}

    DotNET                  CompilerVersion;

    // Variables
    triState                ATLMinimizesCRunTimeLibraryUsage;
    triState                BuildBrowserInformation;
    charSet                 CharacterSet;
    ConfigurationTypes      ConfigurationType;
    QString                 DeleteExtensionsOnClean;
    QString                 ImportLibrary;
    QString                 IntermediateDirectory;
    QString                 Name;   // "ConfigurationName|PlatformName"
    QString                 ConfigurationName;
    QString                 OutputDirectory;
    QString                 PrimaryOutput;
    QString                 ProgramDatabase;
    triState                RegisterOutput;
    useOfATL                UseOfATL;
    useOfMfc                UseOfMfc;
    triState                WholeProgramOptimization;

    // XML sub-parts
    VCCLCompilerTool        compiler;
    VCLinkerTool            linker;
    VCLibrarianTool         librarian;
    VCCustomBuildTool       custom;
    VCMIDLTool              idl;
    VCPostBuildEventTool    postBuild;
    VCPreBuildEventTool     preBuild;
    VCDeploymentTool        deployment;
    VCPreLinkEventTool      preLink;
    VCResourceCompilerTool  resource;
};

struct VCFilterFile
{
    VCFilterFile()
    { excludeFromBuild = false; }
    VCFilterFile(const QString &filename, bool exclude = false )
    { file = filename; excludeFromBuild = exclude; }
    VCFilterFile(const QString &filename, const QString &additional, bool exclude = false )
    { file = filename; excludeFromBuild = exclude; additionalFile = additional; }
    bool operator==(const VCFilterFile &other){
        return file == other.file
               && additionalFile == other.additionalFile
               && excludeFromBuild == other.excludeFromBuild;
    }

    bool                    excludeFromBuild;
    QString                 file;
    QString                 additionalFile; // For tools like MOC
};

#ifndef QT_NO_DEBUG_OUTPUT
inline QDebug operator<<(QDebug dbg, const VCFilterFile &p)
{
    dbg.nospace() << "VCFilterFile(file(" << p.file
                  << ") additionalFile(" << p.additionalFile
                  << ") excludeFromBuild(" << p.excludeFromBuild << "))" << endl;
    return dbg.space();
}
#endif

class VcprojGenerator;
class VCFilter
{
public:
    // Functions
    VCFilter();
    ~VCFilter(){}

    void addFile(const QString& filename);
    void addFile(const VCFilterFile& fileInfo);
    void addFiles(const QStringList& fileList);
    bool addExtraCompiler(const VCFilterFile &info);
    void modifyPCHstage(QString str);

    // Variables
    QString                 Name;
    QString                 Filter;
    QString                 Guid;
    triState                ParseFiles;
    VcprojGenerator*        Project;
    VCConfiguration*        Config;
    QList<VCFilterFile>     Files;

    customBuildCheck	    CustomBuild;

    bool                    useCustomBuildTool;
    VCCustomBuildTool       CustomBuildTool;

    bool                    useCompilerTool;
    VCCLCompilerTool        CompilerTool;
};

typedef QList<VCFilter> VCFilterList;
class VCProjectSingleConfig
{
public:
    enum FilterTypes {
        None,
        Source,
        Header,
        Generated,
        LexYacc,
        Translation,
        Resources,
        Extras
    };
    // Functions
    VCProjectSingleConfig(){}
    ~VCProjectSingleConfig(){}

    // Variables
    QString                 Name;
    QString                 Version;
    QString                 ProjectGUID;
    QString                 Keyword;
    QString                 SccProjectName;
    QString                 SccLocalPath;
    QString                 PlatformName;

    // XML sub-parts
    VCConfiguration         Configuration;
    VCFilter                RootFiles;
    VCFilter                SourceFiles;
    VCFilter                HeaderFiles;
    VCFilter                GeneratedFiles;
    VCFilter                LexYaccFiles;
    VCFilter                TranslationFiles;
    VCFilter                FormFiles;
    VCFilter                ResourceFiles;
    VCFilterList            ExtraCompilersFiles;

    bool                    flat_files;

    // Accessor for extracompilers
    VCFilter               &filterForExtraCompiler(const QString &compilerName);
};

// Tree & Flat view of files --------------------------------------------------
class VCFilter;
class Node
{
public:
    virtual ~Node() { }
    void addElement(const VCFilterFile &file) {
        addElement(file.file, file);
    }
    virtual void addElement(const QString &filepath, const VCFilterFile &allInfo) = 0;
    virtual void removeElements()= 0;
    virtual void generateXML(XmlOutput &xml, const QString &tagName, VCProject &tool, const QString &filter) = 0;
    virtual bool hasElements() = 0;
};

class TreeNode : public Node
{
    typedef QMap<QString, TreeNode*> ChildrenMap;
    VCFilterFile info;
    ChildrenMap children;

public:
    virtual ~TreeNode() { removeElements(); }

    int pathIndex(const QString &filepath) {
        int Windex = filepath.indexOf("\\");
        int Uindex = filepath.indexOf("/");
        if (Windex != -1 && Uindex != -1)
            return qMin(Windex, Uindex);
        else if (Windex != -1)
            return Windex;
        return Uindex;
    }

    void addElement(const QString &filepath, const VCFilterFile &allInfo){
        QString newNodeName(filepath);

        int index = pathIndex(filepath);
        if (index != -1)
            newNodeName = filepath.left(index);

        TreeNode *n = children.value(newNodeName);
        if (!n) {
            n = new TreeNode;
            n->info = allInfo;
            children.insert(newNodeName, n);
        }
        if (index != -1)
            n->addElement(filepath.mid(index+1), allInfo);
    }

    void removeElements() {
        ChildrenMap::ConstIterator it = children.constBegin();
        ChildrenMap::ConstIterator end = children.constEnd();
        for( ; it != end; it++) {
            (*it)->removeElements();
            delete it.value();
        }
        children.clear();
    }

    void generateXML(XmlOutput &xml, const QString &tagName, VCProject &tool, const QString &filter);
    bool hasElements() {
        return children.size() != 0;
    }
};

class FlatNode : public Node
{
    typedef QMap<QString, VCFilterFile> ChildrenMapFlat;
    ChildrenMapFlat children;

public:
    virtual ~FlatNode() { removeElements(); }

    int pathIndex(const QString &filepath) {
        int Windex = filepath.lastIndexOf("\\");
        int Uindex = filepath.lastIndexOf("/");
        if (Windex != -1 && Uindex != -1)
            return qMax(Windex, Uindex);
        else if (Windex != -1)
            return Windex;
        return Uindex;
    }

    void addElement(const QString &filepath, const VCFilterFile &allInfo){
        QString newKey(filepath);

        int index = pathIndex(filepath);
        if (index != -1)
            newKey = filepath.mid(index+1);

        // Key designed to sort files with same
        // name in different paths correctly
        children.insert(newKey + "\0" + allInfo.file, allInfo);
    }

    void removeElements() {
        children.clear();
    }

    void generateXML(XmlOutput &xml, const QString &tagName, VCProject &proj, const QString &filter);
    bool hasElements() {
        return children.size() != 0;
    }
};
// ----------------------------------------------------------------------------

class VCProject
{
public:
    // Variables
    QString                 Name;
    QString                 Version;
    QString                 ProjectGUID;
    QString                 Keyword;
    QString                 SccProjectName;
    QString                 SccLocalPath;
    QString                 PlatformName;

    // Single projects
    QList<VCProjectSingleConfig>  SingleProjects;

    // List of all extracompilers
    QStringList             ExtraCompilers;
};

class VCProjectWriter
{
public:
    virtual ~VCProjectWriter() {}

    virtual void write(XmlOutput &, VCProjectSingleConfig &);
    virtual void write(XmlOutput &, VCProject &);

    virtual void write(XmlOutput &, const VCCLCompilerTool &);
    virtual void write(XmlOutput &, const VCLinkerTool &);
    virtual void write(XmlOutput &, const VCMIDLTool &);
    virtual void write(XmlOutput &, const VCCustomBuildTool &);
    virtual void write(XmlOutput &, const VCLibrarianTool &);
    virtual void write(XmlOutput &, const VCResourceCompilerTool &);
    virtual void write(XmlOutput &, const VCEventTool &);
    virtual void write(XmlOutput &, const VCDeploymentTool &);
    virtual void write(XmlOutput &, const VCConfiguration &);
    virtual void write(XmlOutput &, VCFilter &);

private:
    static void outputFilter(VCProject &project, XmlOutput &xml, const QString &filtername);
    static void outputFileConfigs(VCProject &project, XmlOutput &xml, const VCFilterFile &info, const QString &filtername);
    static void outputFileConfig(VCFilter &filter, XmlOutput &xml, const QString &filename);

    friend class TreeNode;
    friend class FlatNode;
};

QT_END_NAMESPACE

#endif // MSVC_OBJECTMODEL_H
