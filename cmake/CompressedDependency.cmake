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
    if (IS_DIRECTORY "${_dependency_file}-patches")
        find_program(PATCH patch REQUIRED)
        file(GLOB patch_files "${_dependency_file}-patches/*.patch")
        list(SORT patch_files)

        foreach (patch_file ${patch_files})
            get_filename_component(patch_name ${patch_file} NAME)
            set(patch_flag "${_dependency_dir}/.patch.${patch_name}")
            if (NOT EXISTS ${patch_flag})
                message(STATUS "Applying patch: ${dependency} ${patch_name}")
                execute_process(
                        COMMAND ${PATCH} -p1 --batch --forward -i ${patch_file}
                        WORKING_DIRECTORY ${_dependency_dir}
                        RESULT_VARIABLE patch_result
                        OUTPUT_VARIABLE patch_output
                        ERROR_VARIABLE patch_error
                )

                if (NOT patch_result EQUAL 0)
                    message(FATAL_ERROR "Failed to apply patch ${patch_name} (exit code ${patch_result}):\n${patch_error}\n${patch_output}")
                else ()
                    message(STATUS "Successfully applied patch: ${patch_name}")
                endif ()
                file(TOUCH "${patch_flag}")
            else ()
                message(STATUS "Skipping ${dependency} ${patch_name}, already applied")
            endif ()
        endforeach ()

    endif ()
endfunction()
