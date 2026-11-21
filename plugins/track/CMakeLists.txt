juce_add_plugin(MixwaveTrack
    COMPANY_NAME "Mixwave"
    PLUGIN_MANUFACTURER_CODE Mxwv
    PLUGIN_CODE Mxtk
    FORMATS VST3
    PRODUCT_NAME "MIXWAVE AI Track"
    PLUGIN_NAME "MIXWAVE AI Track"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD FALSE)   # installing to Program Files needs admin: use scripts/install.ps1

target_sources(MixwaveTrack PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    ${MIXWAVE_CORE_SOURCES})

target_include_directories(MixwaveTrack PRIVATE
    ${MIXWAVE_CORE_DIR}
    ${CMAKE_SOURCE_DIR}/plugins/common)

target_compile_definitions(MixwaveTrack PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixwaveTrack
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
