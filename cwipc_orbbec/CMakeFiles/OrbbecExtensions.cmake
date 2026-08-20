macro(get_orbbec_extension_location varname)
    get_target_property(runtime ob::OrbbecSDK IMPORTED_LOCATION_RELEASE)
    message("xxxjack runtime is: " ${runtime} ", okay?")
    cmake_path(GET runtime PARENT_PATH runtime_dir)
    message("xxxjack runtime_dir is: " ${runtime_dir} ", okay?")
    cmake_path(APPEND runtime_dir "extensions")
    message("xxxjack extensions_dir is: " ${runtime_dir} ", okay?")
    set(${varname} ${runtime_dir})
endmacro()

macro(install_orbbec_extensions destdir)
    message("xxxjack installing Orbbec extensions to " ${destdir} ", okay?")
    get_orbbec_extension_location(extensions_dir)
    message("xxxjack extensions_dir is: " ${extensions_dir} ", okay?")
    set(source_path "${extensions_dir}")
    set(destination_path "${destdir}")
    cmake_path(APPEND destination_path "extensions")
    install(DIRECTORY "${source_path}/"
        DESTINATION "${destination_path}"
        # COMMENT "Installing Orbbec extensions from ${source_path} to ${destination_path}"
    )
    message("xxxjack will install Orbbec extensions from ${source_path} to ${destination_path}")
endmacro()

macro(copy_orbbec_extensions destdir)
    get_orbbec_extension_location(extension_dir)
    set(source_path "${extension_dir}")
    set(destination_path "${destdir}")
    cmake_path(APPEND destination_path "extensions")
    add_custom_command(TARGET cwipc_orbbec POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${source_path}"
        "${destination_path}"
        COMMENT "Copying Orbbec extensions from ${source_path} to ${destination_path}"
    )
    message("xxxjack will copy Orbbec extensions from ${source_path} to ${destination_path}")
endmacro()