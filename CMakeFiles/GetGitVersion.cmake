macro(get_git_version)
	set(options)
	set(oneValueArgs VERSION_VAR)
	set(multiValueArgs)
	cmake_parse_arguments(MYARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# cwipc-specific: our release tags start with v
	execute_process(COMMAND "git" "describe" "--tags" "--match" "v*" RESULT_VARIABLE status OUTPUT_VARIABLE describe_output OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(status AND NOT status EQUAL 0)
		message(STATUS "GetGitVersion: git describe failed: ${status}, using .cachedgitversion.txt")
		set(describe_version "0.0+unknown")
		file(READ "${CMAKE_SOURCE_DIR}/.cachedgitversion.txt" describe_version)
		set(${MYARGS_VERSION_VAR} ${describe_version})
	else()
		# cwipc-specific: remove the "v" and any "_stable"
		string(REPLACE "_stable" "" describe_output "${describe_output}")
		string(REPLACE "v" "" describe_output "${describe_output}")
		# Now we have either 1.2.3 or 1.2.3-123-g123abc. Get the version bit
		string(REGEX MATCH "[0-9\\.]+" describe_version "${describe_output}")
		string(REGEX MATCH "-g[a-fA-F0-9]+" describe_sha "${describe_output}")
		string(REPLACE "-g" "" describe_sha "${describe_sha}")
		# Use unknown if no version tag found
		if(NOT describe_version)
			set(describe_version "0.0")
		endif()
		if(describe_sha)
			# Append +sha to the version tag
			set(full_version "${describe_version}+${describe_sha}")
		else()
			# Return just the version tag
			set(full_version ${describe_version})
		endif()
		file(WRITE "${CMAKE_SOURCE_DIR}/.cachedgitversion.txt" ${full_version})
		set(${MYARGS_VERSION_VAR} ${full_version})
		endif()
	
endmacro()
