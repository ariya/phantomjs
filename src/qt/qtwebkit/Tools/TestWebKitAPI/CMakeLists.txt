set(TESTWEBKITAPI_DIR "${TOOLS_DIR}/TestWebKitAPI")

include_directories(${CMAKE_BINARY_DIR}
    ${TESTWEBKITAPI_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${DERIVED_SOURCES_WEBKIT2_DIR}/include
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/API
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${THIRDPARTY_DIR}/gtest/include
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/platform/network
    ${WEBKIT2_DIR}/Shared
    ${WEBKIT2_DIR}/Shared/API/c
    ${WEBKIT2_DIR}/Shared/Plugins
    ${WEBKIT2_DIR}/UIProcess
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/c
    ${WTF_DIR}
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

add_library(TestWebKitAPIInjectedBundle SHARED
    ${bundle_harness_SOURCES}
    ${TESTWEBKITAPI_DIR}/InjectedBundleController.cpp
    ${TESTWEBKITAPI_DIR}/InjectedBundleMain.cpp
    ${TESTWEBKITAPI_DIR}/PlatformUtilities.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/CanHandleRequest_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DidAssociateFormControls_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DOMWindowExtensionBasic_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DOMWindowExtensionNoCache_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DocumentStartUserScriptAlertCrash_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/GetInjectedBundleInitializationUserDataCallback_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/HitTestResultNodeHandle_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleBasic_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleFrameHitTest_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleInitializationUserDataCallbackWins_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/LoadCanceledNoServerRedirectCallback_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/MouseMoveAfterCrash_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutFails_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutForImages_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutFrames_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayout_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ParentFrame_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ResponsivenessTimerDoesntFireEarly_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ShouldGoToBackForwardListItem_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/UserMessage_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/WKConnection_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/WillLoad_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/WillSendSubmitEvent_Bundle.cpp
)

set(TestWebKitAPI_LIBRARIES
    WTF
    WebKit2
)

target_link_libraries(TestWebKitAPIInjectedBundle ${TestWebKitAPI_LIBRARIES})
add_dependencies(TestWebKitAPIInjectedBundle ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})

get_property(TestWebKitAPIInjectedBundle_PATH TARGET TestWebKitAPIInjectedBundle PROPERTY LOCATION)

add_definitions(-DGTEST_LINKED_AS_SHARED_LIBRARY=1
    -DTEST_WEBKIT2_RESOURCES_DIR=\"${TESTWEBKITAPI_DIR}/Tests/WebKit2\"
    -DTEST_INJECTED_BUNDLE_PATH=\"${TestWebKitAPIInjectedBundle_PATH}\"
)

set(test_wtf_LIBRARIES
    WTF
    gtest
)

add_executable(test_wtf
    ${test_main_SOURCES}
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/AtomicString.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/CString.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/CheckedArithmeticOperations.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Functional.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/HashMap.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/IntegerToStringConversion.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/ListHashSet.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MD5.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MathExtras.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MetaAllocator.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/RedBlackTree.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/SHA1.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/SaturatedArithmeticOperations.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringBuilder.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringHasher.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringImpl.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringOperators.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/TemporaryChange.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Vector.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/VectorBasic.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/VectorReverse.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/WTFString.cpp
)

target_link_libraries(test_wtf ${test_wtf_LIBRARIES})
add_dependencies(test_wtf ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})
add_test(test_wtf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_wtf)
set_tests_properties(test_wtf PROPERTIES TIMEOUT 60)

set(test_webcore_LIBRARIES
    gtest
    WTF
    WebCore
)

foreach (testName ${test_webcore_BINARIES})
    add_executable(test_webcore_${testName} ${test_main_SOURCES} ${TESTWEBKITAPI_DIR}/TestsController.cpp ${TESTWEBKITAPI_DIR}/Tests/WebCore/${testName}.cpp)
    add_test(test_webcore_${testName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_webcore_${testName})
    set_tests_properties(test_webcore_${testName} PROPERTIES TIMEOUT 60)
    target_link_libraries(test_webcore_${testName} ${test_webcore_LIBRARIES})
endforeach ()

add_library(TestWebKitAPIBase
    ${test_main_SOURCES}
    ${webkit2_api_harness_SOURCES}
    ${TESTWEBKITAPI_DIR}/JavaScriptTest.cpp
    ${TESTWEBKITAPI_DIR}/PlatformUtilities.cpp
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
)

add_dependencies(TestWebKitAPIBase WebKit2 ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})

set(test_webkit2_api_LIBRARIES
    TestWebKitAPIBase
    WTF
    JavaScriptCore
    WebKit2
    gtest
)

foreach (testName ${test_webkit2_api_BINARIES})
    get_filename_component(testBaseName ${testName} NAME)
    add_executable(test_webkit2_api_${testBaseName} ${TESTWEBKITAPI_DIR}/Tests/WebKit2/${testName}.cpp)
    add_test(test_webkit2_api_${testBaseName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_webkit2_api_${testBaseName})
    set_tests_properties(test_webkit2_api_${testBaseName} PROPERTIES TIMEOUT 60)
    target_link_libraries(test_webkit2_api_${testBaseName} ${test_webkit2_api_LIBRARIES})
endforeach ()

# We don't run tests that are expected to fail. We could use the WILL_FAIL
# property, but it reports failure when the test crashes or timeouts and would
# make the bot red.

foreach (testName ${test_webkit2_api_fail_BINARIES})
    add_executable(test_webkit2_api_fail_${testName} ${TESTWEBKITAPI_DIR}/Tests/WebKit2/${testName}.cpp)
    target_link_libraries(test_webkit2_api_fail_${testName} ${test_webkit2_api_LIBRARIES})
endforeach ()
