#!/bin/bash

printf "Test failed: "
cat test_res | grep "Test failed" | wc -l
printf "Test passed: "
cat test_res | grep "Test passed" | wc -l
