cmake_minimum_required(VERSION 3.22)
project(VoxLane VERSION 0.5.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT WIN32)
    message(FATAL_ERROR "VoxLane 0.5 supports Windows only")
endif()
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_gui_app(VoxLane PRODUCT_NAME "VoxLane 0.5" VERSION "0.5.0"
    COMPANY_NAME "Reekksio" BUNDLE_ID "com.vstmichost.app")
juce_generate_juce_header(VoxLane)
target_sources(VoxLane PRIVATE
    Source/main.cpp Source/maincomponent.cpp Source/maincomponent.h
    Source/VstPanel.cpp Source/VstPanel.h
    Source/AudioUtilityProcessor.cpp Source/AudioUtilityProcessor.h
    Source/CrashLog.cpp)
target_compile_definitions(VoxLane PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST=0 JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    VSTMIC_JUCE_COMMIT="8.0.8" VSTMIC_JUCE_PATCH_PRESENT=false
    VSTMIC_BUILD_TYPE="$<CONFIG>")
target_link_libraries(VoxLane PRIVATE
    juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
    juce::juce_audio_processors juce::juce_audio_utils juce::juce_core
    juce::juce_data_structures juce::juce_dsp juce::juce_events
    juce::juce_graphics juce::juce_gui_basics
    juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
juce_add_console_app(VSTScanner PRODUCT_NAME "VSTScanner")
juce_generate_juce_header(VSTScanner)
target_sources(VSTScanner PRIVATE Scanner/ScannerMain.cpp)
target_compile_definitions(VSTScanner PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0 JUCE_PLUGINHOST_AU=0)
target_link_libraries(VSTScanner PRIVATE
    juce::juce_audio_basics juce::juce_audio_processors juce::juce_core
    juce::juce_data_structures juce::juce_events juce::juce_recommended_config_flags)
