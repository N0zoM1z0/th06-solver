set(BIN "${WORK}/bin")
set(OUT "${WORK}/output")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${BIN}" "${OUT}")
configure_file("${CAMPAIGN}" "${BIN}/th06_campaign" COPYONLY)
file(WRITE "${BIN}/th06_plan" [=[#!/bin/sh
rm -f "$TH06_SNAPSHOT_TEST_BIN/th06_plan" "$TH06_SNAPSHOT_TEST_BIN/th06_replay_check"
while [ "$#" -gt 0 ]; do
  if [ "$1" = --output ]; then shift; tape_path=$1; fi
  shift
done
cat > "$tape_path" <<'TAPE'
TH06ACTIONS 2 native-i386-v1
1 3 0 0 0 0
123 1 1
1 261 1
TAPE
]=])
file(WRITE "${BIN}/th06_replay_check" "#!/bin/sh\nexit 0\n")
file(CHMOD "${BIN}/th06_campaign" "${BIN}/th06_plan" "${BIN}/th06_replay_check"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "TH06_SNAPSHOT_TEST_BIN=${BIN}"
  "${BIN}/th06_campaign" --data unused-test-data --output "${OUT}" --jobs 1
  --case s1-d3-shot0-seed0-p0 RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
if(NOT result EQUAL 0 OR NOT output MATCHES "\"cleared\":1")
  message(FATAL_ERROR "frozen-runtime regression failed: ${output} ${diagnostic}")
endif()
if(EXISTS "${BIN}/th06_plan" OR EXISTS "${BIN}/th06_replay_check")
  message(FATAL_ERROR "regression did not remove the mutable source binaries")
endif()
file(GLOB receipts "${OUT}/bin/*/runtime.json")
list(LENGTH receipts count)
if(NOT count EQUAL 1)
  message(FATAL_ERROR "runtime identity receipt missing")
endif()
