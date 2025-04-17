# example of how compression can be done
# 7zz a boost_1_88_0 -w boost_1_88_0/.
function(prepare_compressed_dependency dependency)
    set(_dependency_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../deps/${dependency}.7z")
    file(SHA1 "${_dependency_file}" _dependency_file_hash)
    set(_dependency_dir "${CMAKE_BINARY_DIR}/dep-${_dependency_file_hash}")
    set("${dependency}_PATH" "${_dependency_dir}" PARENT_SCOPE)
    if (NOT EXISTS "${_dependency_dir}")
        file(ARCHIVE_EXTRACT INPUT "${_dependency_file}" DESTINATION "${_dependency_dir}")
    endif ()
endfunction()
