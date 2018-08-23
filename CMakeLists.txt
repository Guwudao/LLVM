# If we don't need RTTI or EH, there's no reason to export anything
# from the plugin.
if( NOT MSVC ) # MSVC mangles symbols differently
    if( NOT LLVM_REQUIRES_RTTI )
        if( NOT LLVM_REQUIRES_EH )
            set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/JJPlugins.exports)
        endif()
    endif()
endif()

add_llvm_loadable_module(JJPlugin JJPlugin.cpp)

if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
    target_link_libraries(MyPlugin ${cmake_2_8_12_PRIVATE}
    clangAST
    clangBasic
    clangFrontend
    LLVMSupport
)
endif()

