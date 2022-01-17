#!/usr/bin/python

import os

def analyse(results_path):
  try:
    os.path.exists(results_path)
    print("Analysis successful!")
    return 0
  except:
    print("Analysis failed! Path to results" + results_path + " does not exist")
    return 1