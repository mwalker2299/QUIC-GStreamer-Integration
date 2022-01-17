#!/usr/bin/python

import argparse
import os
import json
import shutil
import analyser
import test_loop

def dir_path(string):
    if os.path.isdir(string):
        return string
    else:
        raise NotADirectoryError(string)

def file_path(string):
    if os.path.isfile(string):
        return string
    else:
        raise FileNotFoundError(string)


def init_argparse():

    parser = argparse.ArgumentParser(
        description="Runs a mininet test framework to evaluate the performance of the given GStreamer network transport protocol implementations. Each possible parameter combination will be tested. The tests are repeated based on commandline args (default iters=10)"
    )

    parser.add_argument(
        "-i", "--iters", dest="iters", default=10, type=int,
        help="Sets the number of test iterations. Default is 10"
    )
    parser.add_argument(
        "-o", "--output", dest="output", default=os.getcwd(), type=dir_path,
        help="Path to directory where results dir will be created. Default is the current directory"
    )
    parser.add_argument(
        "-ic", "--impl_config", dest="impl_config", default=os.path.join(os.getcwd(), "implementations.json"), type=file_path,
        help="Path to the implementations json config file"
    )
    parser.add_argument(
        "-pc", "--param_config", dest="param_config", default=os.path.join(os.getcwd(), "parameters.json"), type=file_path,
        help="Path to the link parameters json config file"
    )

    return parser

def main():
    # Setup arg parser and parse cmdline args
    parser = init_argparse()
    args = parser.parse_args()

    results_path = os.path.join(args.output, "mininet_results")
    iterations = args.iters
    impl_config_path = args.impl_config
    param_config_path = args.param_config

    # Retrieve implementation and parameter config dictionaries 
    impl_config_file = open(impl_config_path, "r")
    impl_config = json.load(impl_config_file)

    param_config_file = open(param_config_path, "r")
    param_config = json.load(param_config_file)

    # Create results directory. (We first remove the directory to clear the files if it already exists)
    try:
      shutil.rmtree(results_path)
    except Exception:
      pass

    os.mkdir(results_path)

    # For each iteration, test each possible param combination. Repeat for specified number of iters. 
    # Before the parameters and logging path are passed to the test loop, we must create the directory structure which will hold the results.
    for implementation in impl_config['implementations'].values():
      server_command = implementation['server']
      client_command = implementation['client']
      protocol_name  = implementation['name']
      log_level      = implementation['logging']
      implementation_results_path = os.path.join(results_path, protocol_name)
      os.mkdir(implementation_results_path)
      for delay in param_config['parameters']['delay']:
        for loss in param_config['parameters']['loss']:
          param_combo_results_path = os.path.join(implementation_results_path, "Delay_"+str(delay)+"ms_"+"Loss_"+str(loss))
          os.mkdir(param_combo_results_path)
          for iteration in range(iterations):
            log_path = os.path.join(param_combo_results_path, "Iteration"+str(iteration))
            os.mkdir(log_path)
            test_params = {"delay": delay, 'loss': loss}
            test_loop.run_test(test_params, server_command, client_command, protocol_name, log_path, log_level)
    

    # call analyser to analyse raw results
    return analyser.analyse(results_path)


if __name__ == "__main__":
    main()