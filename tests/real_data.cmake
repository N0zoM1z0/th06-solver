file(MAKE_DIRECTORY "${WORK}")
execute_process(COMMAND "${HEADLESS}" --data "${DATA}" --audit
  RESULT_VARIABLE result OUTPUT_VARIABLE audit ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0 OR NOT audit MATCHES "\"verified\":true")
  message(FATAL_ERROR "DAT audit failed: ${audit} ${diagnostic}")
endif()
foreach(run RANGE 1 2)
  execute_process(COMMAND "${HEADLESS}" --data "${DATA}" --frames 1200 --trace "${WORK}/baseline${run}.tsv"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
  if(NOT result EQUAL 2 OR NOT output MATCHES "\"hit\":true" OR NOT output MATCHES "\"deaths\":1")
    message(FATAL_ERROR "stationary negative control failed: ${output} ${diagnostic}")
  endif()
endforeach()
file(SHA256 "${WORK}/baseline1.tsv" first)
file(SHA256 "${WORK}/baseline2.tsv" second)
if(NOT first STREQUAL second)
  message(FATAL_ERROR "fresh native traces differ")
endif()
execute_process(COMMAND "${PLAN}" --data "${DATA}" --frames 600 --horizon 48 --commit 8
  --output "${WORK}/prefix.actions" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 2 OR NOT output MATCHES "\"frames\":600" OR NOT output MATCHES "\"deaths\":0")
  message(FATAL_ERROR "native planner prefix failed: ${output} ${diagnostic}")
endif()
execute_process(COMMAND "${CHECK}" --data "${DATA}" --tape "${WORK}/prefix.actions"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0 OR NOT output MATCHES "\"verified\":true")
  message(FATAL_ERROR "fresh replay failed: ${output} ${diagnostic}")
endif()
execute_process(COMMAND "${CHECK}" --data "${DATA}" --tape "${WORK}/prefix.actions" --require-complete
  RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
if(NOT result EQUAL 2)
  message(FATAL_ERROR "incomplete prefix certified as complete")
endif()
file(READ "${WORK}/prefix.actions" tape)
string(REGEX REPLACE "1 ([0-9]+) [0-9]+\n2 " "1 \\1 0\n2 " damaged "${tape}")
if(damaged STREQUAL tape)
  message(FATAL_ERROR "corruption fixture did not change tape")
endif()
file(WRITE "${WORK}/damaged.actions" "${damaged}")
execute_process(COMMAND "${PROBE}" --data "${DATA}" --tape "${WORK}/prefix.actions"
  --origin 200 --lead 8 --horizon 32 --budget-ms 1000 --proposals 1
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0 OR NOT output MATCHES "\"literal_checked\":true" OR
   NOT output MATCHES "\"certified\":40")
  message(FATAL_ERROR "checked shadow probe failed: ${output} ${diagnostic}")
endif()
foreach(origin IN ITEMS 1 0)
  execute_process(COMMAND "${PROBE}" --data "${DATA}" --tape "${WORK}/damaged.actions"
    --origin ${origin} --lead 8 --horizon 32 --budget-ms 1000 --proposals 1
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
  if(NOT result EQUAL 1 OR NOT diagnostic MATCHES "mismatch/hit at 1")
    message(FATAL_ERROR "shadow probe accepted corrupt prefix/lead: ${diagnostic}")
  endif()
endforeach()
execute_process(COMMAND "${PROBE}" --data "${DATA}" --tape "${WORK}/prefix.actions"
  --origin 599 --lead 8 RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 1 OR NOT diagnostic MATCHES "entire immutable lead")
  message(FATAL_ERROR "shadow probe accepted missing lead: ${diagnostic}")
endif()
execute_process(COMMAND "${CHECK}" --data "${DATA}" --tape "${WORK}/damaged.actions"
  RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 1 OR NOT diagnostic MATCHES "replay divergence at frame 1")
  message(FATAL_ERROR "damaged replay was not rejected: ${diagnostic}")
endif()
execute_process(COMMAND "${PLAN}" --data "${DATA}" --prefix "${WORK}/prefix.actions"
  --prefix-frames 200 --exclude-first 261 --frames 600 --horizon 48 --commit 8
  --output "${WORK}/continued.actions" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 2 OR NOT output MATCHES "\"replayed_prefix_frames\":200")
  message(FATAL_ERROR "verified prefix continuation failed: ${output} ${diagnostic}")
endif()
execute_process(COMMAND "${CHECK}" --data "${DATA}" --tape "${WORK}/continued.actions"
  RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "continued route did not replay: ${diagnostic}")
endif()
execute_process(COMMAND "${PLAN}" --data "${DATA}" --prefix "${WORK}/prefix.actions"
  --prefix-frames 200 --frames 600 --horizon 48 --commit 8 --reactive
  --output "${WORK}/reactive.actions" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 2 OR NOT output MATCHES "\"reactive\":true")
  message(FATAL_ERROR "reactive native prefix failed: ${output} ${diagnostic}")
endif()
string(JSON reactive_branches GET "${output}" reactive_branches)
if(reactive_branches LESS 1)
  message(FATAL_ERROR "reactive continuation was not exercised")
endif()
execute_process(COMMAND "${CHECK}" --data "${DATA}" --tape "${WORK}/reactive.actions"
  RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "reactive route did not replay: ${diagnostic}")
endif()
execute_process(COMMAND "${PLAN}" --data "${DATA}" --prefix "${WORK}/damaged.actions"
  --prefix-frames 1 --frames 10 --output "${WORK}/invalid-prefix.actions"
  RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 1 OR NOT diagnostic MATCHES "prefix replay divergence at frame 1")
  message(FATAL_ERROR "damaged prefix was not rejected: ${diagnostic}")
endif()
