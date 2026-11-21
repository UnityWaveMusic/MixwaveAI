juce_add_plugin(MixwaveHub
    COMPANY_NAME "Mixwave"
    PLUGIN_MANUFACTURER_CODE Mxwv
    PLUGIN_CODE Mxhb
    FORMATS VST3
    PRODUCT_NAME "MIXWAVE AI Hub"
    PLUGIN_NAME "MIXWAVE AI Hub"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(MixwaveHub PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    ${MIXWAVE_CORE_SOURCES})

target_include_directories(MixwaveHub PRIVATE
    ${MIXWAVE_CORE_DIR}
    ${CMAKE_SOURCE_DIR}/plugins/common)

target_compile_definitions(MixwaveHub PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixwaveHub
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
