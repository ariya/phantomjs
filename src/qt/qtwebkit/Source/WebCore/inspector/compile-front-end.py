#!/usr/bin/env python
# Copyright (c) 2012 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import os.path
import generate_protocol_externs
import shutil
import sys
import tempfile

inspector_path = "Source/WebCore/inspector"
inspector_frontend_path = inspector_path + "/front-end"
protocol_externs_path = inspector_frontend_path + "/protocol-externs.js"

generate_protocol_externs.generate_protocol_externs(protocol_externs_path, inspector_path + "/Inspector.json")

jsmodule_name_prefix = "jsmodule_"
modules = [
    {
        "name": "common",
        "dependencies": [],
        "sources": [
            "Color.js",
            "DOMExtension.js",
            "Object.js",
            "ParsedURL.js",
            "Progress.js",
            "Settings.js",
            "UIString.js",
            "UserMetrics.js",
            "utilities.js",
        ]
    },
    {
        "name": "sdk",
        "dependencies": ["common"],
        "sources": [
            "ApplicationCacheModel.js",
            "CompilerScriptMapping.js",
            "ConsoleModel.js",
            "ContentProvider.js",
            "ContentProviderBasedProjectDelegate.js",
            "ContentProviders.js",
            "CookieParser.js",
            "CSSMetadata.js",
            "CSSStyleModel.js",
            "BreakpointManager.js",
            "Database.js",
            "DOMAgent.js",
            "DOMStorage.js",
            "DebuggerModel.js",
            "DebuggerScriptMapping.js",
            "FileManager.js",
            "FileMapping.js",
            "FileSystemMapping.js",
            "FileSystemModel.js",
            "FileSystemProjectDelegate.js",
            "FileUtils.js",
            "HAREntry.js",
            "IndexedDBModel.js",
            "InspectorBackend.js",
            "IsolatedFileSystemManager.js",
            "IsolatedFileSystem.js",
            "Linkifier.js",
            "NetworkLog.js",
            "NetworkUISourceCodeProvider.js",
            "PresentationConsoleMessageHelper.js",
            "RuntimeModel.js",
            "SASSSourceMapping.js",
            "Script.js",
            "ScriptFormatter.js",
            "ScriptSnippetModel.js",
            "SimpleWorkspaceProvider.js",
            "SnippetStorage.js",
            "SourceMapping.js",
            "StylesSourceMapping.js",
            "TimelineManager.js",
            "RemoteObject.js",
            "Resource.js",
            "DefaultScriptMapping.js",
            "ResourceScriptMapping.js",
            "LiveEditSupport.js",
            "ResourceTreeModel.js",
            "ResourceType.js",
            "ResourceUtils.js",
            "SourceMap.js",
            "NetworkManager.js",
            "NetworkRequest.js",
            "UISourceCode.js",
            "UserAgentSupport.js",
            "Workspace.js",
            "protocol-externs.js",
        ]
    },
    {
        "name": "ui",
        "dependencies": ["common"],
        "sources": [
            "Checkbox.js",
            "ContextMenu.js",
            "DOMSyntaxHighlighter.js",
            "DataGrid.js",
            "DefaultTextEditor.js",
            "Dialog.js",
            "DockController.js",
            "Drawer.js",
            "EmptyView.js",
            "GoToLineDialog.js",
            "HelpScreen.js",
            "InspectorView.js",
            "KeyboardShortcut.js",
            "OverviewGrid.js",
            "Panel.js",
            "PanelEnablerView.js",
            "Placard.js",
            "Popover.js",
            "ProgressIndicator.js",
            "PropertiesSection.js",
            "SearchController.js",
            "Section.js",
            "SidebarPane.js",
            "SidebarTreeElement.js",
            "ShortcutsScreen.js",
            "ShowMoreDataGridNode.js",
            "SidebarOverlay.js",
            "SoftContextMenu.js",
            "SourceTokenizer.js",
            "Spectrum.js",
            "SplitView.js",
            "SidebarView.js",
            "StatusBarButton.js",
            "SuggestBox.js",
            "TabbedPane.js",
            "TextEditor.js",
            "TextEditorHighlighter.js",
            "TextEditorModel.js",
            "TextPrompt.js",
            "TextUtils.js",
            "TimelineGrid.js",
            "Toolbar.js",
            "UIUtils.js",
            "View.js",
            "ViewportControl.js",
            "treeoutline.js",
        ]
    },
    {
        "name": "components",
        "dependencies": ["sdk", "ui"],
        "sources": [
            "AdvancedSearchController.js",
            "HandlerRegistry.js",
            "ConsoleMessage.js",
            "CookiesTable.js",
            "DOMBreakpointsSidebarPane.js",
            "DOMPresentationUtils.js",
            "ElementsTreeOutline.js",
            "FontView.js",
            "ImageView.js",
            "NativeBreakpointsSidebarPane.js",
            "InspectElementModeController.js",
            "ObjectPopoverHelper.js",
            "ObjectPropertiesSection.js",
            "SourceFrame.js",
            "ResourceView.js",
        ]
    },
    {
        "name": "elements",
        "dependencies": ["components"],
        "sources": [
            "CSSNamedFlowCollectionsView.js",
            "CSSNamedFlowView.js",
            "ElementsPanel.js",
            "ElementsPanelDescriptor.js",
            "EventListenersSidebarPane.js",
            "MetricsSidebarPane.js",
            "PropertiesSidebarPane.js",
            "StylesSidebarPane.js",
        ]
    },
    {
        "name": "network",
        "dependencies": ["components"],
        "sources": [
            "NetworkItemView.js",
            "RequestCookiesView.js",
            "RequestHeadersView.js",
            "RequestHTMLView.js",
            "RequestJSONView.js",
            "RequestPreviewView.js",
            "RequestResponseView.js",
            "RequestTimingView.js",
            "RequestView.js",
            "ResourceWebSocketFrameView.js",
            "NetworkPanel.js",
            "NetworkPanelDescriptor.js",
        ]
    },
    {
        "name": "resources",
        "dependencies": ["components"],
        "sources": [
            "ApplicationCacheItemsView.js",
            "CookieItemsView.js",
            "DatabaseQueryView.js",
            "DatabaseTableView.js",
            "DirectoryContentView.js",
            "DOMStorageItemsView.js",
            "FileContentView.js",
            "FileSystemView.js",
            "IndexedDBViews.js",
            "ResourcesPanel.js",
        ]
    },
    {
        "name": "workers",
        "dependencies": ["components"],
        "sources": [
            "WorkerManager.js",
        ]
    },
    {
        "name": "scripts",
        "dependencies": ["components", "workers"],
        "sources": [
            "BreakpointsSidebarPane.js",
            "CallStackSidebarPane.js",
            "FilteredItemSelectionDialog.js",
            "JavaScriptSourceFrame.js",
            "NavigatorOverlayController.js",
            "NavigatorView.js",
            "RevisionHistoryView.js",
            "ScopeChainSidebarPane.js",
            "ScriptsNavigator.js",
            "ScriptsPanel.js",
            "ScriptsPanelDescriptor.js",
            "ScriptsSearchScope.js",
            "SnippetJavaScriptSourceFrame.js",
            "StyleSheetOutlineDialog.js",
            "TabbedEditorContainer.js",
            "UISourceCodeFrame.js",
            "WatchExpressionsSidebarPane.js",
            "WorkersSidebarPane.js",
        ]
    },
    {
        "name": "console",
        "dependencies": ["components"],
        "sources": [
            "ConsoleView.js",
            "ConsolePanel.js",
        ]
    },
    {
        "name": "timeline",
        "dependencies": ["components"],
        "sources": [
            "DOMCountersGraph.js",
            "MemoryStatistics.js",
            "NativeMemoryGraph.js",
            "TimelineModel.js",
            "TimelineOverviewPane.js",
            "TimelinePanel.js",
            "TimelinePanelDescriptor.js",
            "TimelinePresentationModel.js",
            "TimelineFrameController.js"
        ]
    },
    {
        "name": "audits",
        "dependencies": ["components"],
        "sources": [
            "AuditCategories.js",
            "AuditController.js",
            "AuditFormatters.js",
            "AuditLauncherView.js",
            "AuditResultView.js",
            "AuditRules.js",
            "AuditsPanel.js",
        ]
    },
    {
        "name": "extensions",
        "dependencies": ["components"],
        "sources": [
            "ExtensionAPI.js",
            "ExtensionAuditCategory.js",
            "ExtensionPanel.js",
            "ExtensionRegistryStub.js",
            "ExtensionServer.js",
            "ExtensionView.js",
        ]
    },
    {
        "name": "settings",
        "dependencies": ["components", "extensions"],
        "sources": [
            "SettingsScreen.js",
            "OverridesView.js",
        ]
    },
    {
        "name": "tests",
        "dependencies": ["components"],
        "sources": [
            "TestController.js",
        ]
    },
    {
        "name": "profiler",
        "dependencies": ["components", "workers"],
        "sources": [
            "BottomUpProfileDataGridTree.js",
            "CPUProfileView.js",
            "CSSSelectorProfileView.js",
            "FlameChart.js",
            "HeapSnapshot.js",
            "HeapSnapshotDataGrids.js",
            "HeapSnapshotGridNodes.js",
            "HeapSnapshotLoader.js",
            "HeapSnapshotProxy.js",
            "HeapSnapshotView.js",
            "HeapSnapshotWorker.js",
            "HeapSnapshotWorkerDispatcher.js",
            "JSHeapSnapshot.js",
            "NativeHeapSnapshot.js",
            "ProfileDataGridTree.js",
            "ProfilesPanel.js",
            "ProfilesPanelDescriptor.js",
            "ProfileLauncherView.js",
            "TopDownProfileDataGridTree.js",
            "CanvasProfileView.js",
        ]
    },
    {
        "name": "host_stub",
        "dependencies": ["components", "profiler", "timeline"],
        "sources": [
            "InspectorFrontendAPI.js",
            "InspectorFrontendHostStub.js",
        ]
    }
]

modules_by_name = {}
for module in modules:
    modules_by_name[module["name"]] = module


def dump_module(name, recursively, processed_modules):
    if name in processed_modules:
        return ""
    processed_modules[name] = True
    module = modules_by_name[name]
    command = ""
    if recursively:
        for dependency in module["dependencies"]:
            command += dump_module(dependency, recursively, processed_modules)
    command += " \\\n    --module " + jsmodule_name_prefix + module["name"] + ":"
    command += str(len(module["sources"]))
    firstDependency = True
    for dependency in module["dependencies"]:
        if firstDependency:
            command += ":"
        else:
            command += ","
        firstDependency = False
        command += jsmodule_name_prefix + dependency
    for script in module["sources"]:
        command += " \\\n        --js " + inspector_frontend_path + "/" + script
    return command

modules_dir = tempfile.mkdtemp()
compiler_command = "java -jar ~/closure/compiler.jar --summary_detail_level 3 --compilation_level SIMPLE_OPTIMIZATIONS --warning_level VERBOSE --language_in ECMASCRIPT5 --accept_const_keyword --module_output_path_prefix %s/ \\\n" % modules_dir

process_recursively = len(sys.argv) > 1
if process_recursively:
    module_name = sys.argv[1]
    if module_name != "all":
        modules = []
        for i in range(1, len(sys.argv)):
            modules.append(modules_by_name[sys.argv[i]])
    for module in modules:
        command = compiler_command
        command += "    --externs " + inspector_frontend_path + "/externs.js"
        command += dump_module(module["name"], True, {})
        print "Compiling \"" + module["name"] + "\""
        os.system(command)
else:
    command = compiler_command
    command += "    --externs " + inspector_frontend_path + "/externs.js"
    for module in modules:
        command += dump_module(module["name"], False, {})
    os.system(command)

if not process_recursively:
    print "Compiling InjectedScriptSource.js..."
    os.system("echo \"var injectedScriptValue = \" > " + inspector_path + "/" + "InjectedScriptSourceTmp.js")
    os.system("cat  " + inspector_path + "/" + "InjectedScriptSource.js" + " >> " + inspector_path + "/" + "InjectedScriptSourceTmp.js")
    command = compiler_command
    command += "    --externs " + inspector_path + "/" + "InjectedScriptExterns.js" + " \\\n"
    command += "    --externs " + protocol_externs_path + " \\\n"
    command += "    --module " + jsmodule_name_prefix + "injected_script" + ":" + "1" + " \\\n"
    command += "        --js " + inspector_path + "/" + "InjectedScriptSourceTmp.js" + " \\\n"
    command += "\n"
    os.system(command)
    os.system("rm " + inspector_path + "/" + "InjectedScriptSourceTmp.js")

    print "Compiling InjectedScriptCanvasModuleSource.js..."
    os.system("echo \"var injectedScriptCanvasModuleValue = \" > " + inspector_path + "/" + "InjectedScriptCanvasModuleSourceTmp.js")
    os.system("cat  " + inspector_path + "/" + "InjectedScriptCanvasModuleSource.js" + " >> " + inspector_path + "/" + "InjectedScriptCanvasModuleSourceTmp.js")
    command = compiler_command
    command += "    --externs " + inspector_path + "/" + "InjectedScriptExterns.js" + " \\\n"
    command += "    --externs " + protocol_externs_path + " \\\n"
    command += "    --module " + jsmodule_name_prefix + "injected_script" + ":" + "1" + " \\\n"
    command += "        --js " + inspector_path + "/" + "InjectedScriptCanvasModuleSourceTmp.js" + " \\\n"
    command += "\n"
    os.system(command)
    os.system("rm " + inspector_path + "/" + "InjectedScriptCanvasModuleSourceTmp.js")

shutil.rmtree(modules_dir)
#os.system("rm " + protocol_externs_path)
