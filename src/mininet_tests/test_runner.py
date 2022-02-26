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


def create_directory(path_to_parent, dir_name):
  new_dir_path = os.path.join(path_to_parent, dir_name)
  if (not os.path.exists(new_dir_path)):
    os.mkdir(new_dir_path)
  return new_dir_path

def test_already_run(path_to_parent, dir_name):
  dir_path = os.path.join(path_to_parent, dir_name)
  if os.path.exists(dir_path):
    filenames = os.listdir(dir_path)
    if "Failure" in filenames or "Success" in filenames:
      print(filenames)
      return True
    else:
      shutil.rmtree(dir_path)
      return False
  else:
    return False
  
    


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

    # Create results director if it does not already exist.
    if (not os.path.exists(results_path)):
      os.mkdir(results_path)

    # For each iteration, test each possible param combination. Repeat for specified number of iters. 
    # Before the parameters and logging path are passed to the test loop, we must create the directory structure which will hold the results.
    for implementation in impl_config['implementations'].values():
      stream_server_command = implementation['stream_server']
      stream_client_command = implementation['stream_client']
      ct_command            = implementation['ct_client']
      run_time              = implementation['run_time']
      protocol_name         = implementation['name']
      log_level             = implementation['logging']
      implementation_results_path = create_directory(results_path, protocol_name)

      test_params = {}
      for delay in param_config['parameters']['delay']:
        delay_experiment_descriptor = "Delay_"+str(delay)+"ms"
        delay_results_path = create_directory(implementation_results_path, delay_experiment_descriptor)
        test_params["delay"] = delay

        for loss in param_config['parameters']['loss']:
          loss_experiment_descriptor = "Loss_"+str(loss)+"%"
          loss_results_path = create_directory(delay_results_path, loss_experiment_descriptor)
          test_params["loss"] = loss

          for jitter in param_config['parameters']['jitter']:
            jitter_experiment_descriptor = "Jitter_"+str(jitter)+"ms"
            jitter_results_path = create_directory(loss_results_path, jitter_experiment_descriptor)
            test_params["jitter"] = jitter

            for buffer_delay in param_config['parameters']['buffer_delay']:
              buffer_delay_experiment_descriptor = "Buffer_Delay_"+str(buffer_delay)+"ms"
              buffer_delay_results_path = create_directory(jitter_results_path, buffer_delay_experiment_descriptor)
              test_params["buffer_delay"] = buffer_delay

              for bandwidth in param_config['parameters']['bandwidth']:
                bandwidth_experiment_descriptor = "Bandwidth_"+str(bandwidth)+"Mbit_per_sec"
                bandwidth_results_path = create_directory(buffer_delay_results_path, bandwidth_experiment_descriptor)
                test_params["bandwidth"] = bandwidth

                for cross_traffic in param_config['parameters']['cross_traffic']:
                  if cross_traffic:
                    cross_traffic_experiment_descriptor = "Cross_Traffic_Enabled"
                  else:
                    cross_traffic_experiment_descriptor = "Cross_Traffic_Disabled"

                  print ("cross_traffic_experiment_descriptor:" + cross_traffic_experiment_descriptor)

                  cross_traffic_results_path = create_directory(bandwidth_results_path, cross_traffic_experiment_descriptor)
                  test_params["cross_traffic"] = cross_traffic

                  for iteration in range(iterations):
                    iteration_descriptor = "Iteration"+str(iteration)
                    
                    if test_already_run(cross_traffic_results_path, iteration_descriptor):
                      print(os.path.join(cross_traffic_results_path, iteration_descriptor) + " has already run. Skipping...")
                      continue
                    log_path = create_directory(cross_traffic_results_path, iteration_descriptor)

                    # Run test (We set timeout to our streams runtime + 5 seconds to account for possibility of loss causing increased run_time)
                    test_loop.run_test(test_params, stream_server_command, stream_client_command, ct_command, int(run_time)+5, protocol_name, log_path, log_level)
            
    # Mark test run as complete:
    done_file_marker = os.path.join(results_path, "DONE")
    open(done_file_marker, 'a').close()

    # call analyser to analyse raw results
    return analyser.analyse(results_path)


if __name__ == "__main__":
    main()
